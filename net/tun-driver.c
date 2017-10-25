/*
  sudo ip link set up dev tap0 && sudo ip a a 1.1.2.1/24 dev tap0 && sudo ip link set tap0 address 54:52:00:02:00:40 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "../timer.h"
#include "config.h"
#include "tests.h"
#include "arp.h"
#include "eth.h"
#include "udp.h"
#include "route.h"
#define SOCKLEN_DEFINED
#include "socket.h"
#undef SOCKLEN_DEFINED
#include "../net_apps.h"
#include "pkt-mempool.h"

#include <linux/if.h>
#include <linux/if_tun.h>

#define CHECKAUX(e,s)						\
	((e)?							\
	 (void)0:						\
	 (fprintf(stderr, "'%s' failed at %s:%d - %s\n",	\
		  s, __FILE__, __LINE__,strerror(errno)),	\
	  exit(0)))
#define CHECK(e) (CHECKAUX(e,#e))
#define CHECKSYS(e) (CHECKAUX((e)==0,#e))
#define CHECKFD(e) (CHECKAUX((e)>=0,#e))

#define STRING(e) #e

int tun_fd;

uint16_t recv(buf_t *in)
{
	(void)in;
	return 0;
}

uint16_t send(const buf_t *out)
{
	ssize_t nwrite;

	if (out->len == 0)
		return 0;
	nwrite = write(tun_fd, out->data, out->len);
	CHECK(nwrite == out->len);

	return nwrite;
}

iface_t iface = {
	.flags = IF_UP|IF_RUNNING,
	.mac_addr = { 0x54, 0x52, 0x00, 0x02, 0x00, 0x41 },
	.ip4_addr = { 1,1,2,2 },
	.send = &send,
};

#define USE_CAPABILITIES
#if defined USE_CAPABILITIES
#include <sys/capability.h>
#endif

static int tun_alloc(char *dev)
{
	assert(dev != NULL);
	int tun_fd = open("/dev/net/tun", O_RDWR);
	CHECKFD(tun_fd);

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	CHECKSYS(ioctl(tun_fd, TUNSETIFF, (void *) &ifr));
	strncpy(dev, ifr.ifr_name, IFNAMSIZ);
	return tun_fd;
}

static void tun_send_pkt(void)
{
	pkt_t *pkt;

	while ((pkt = pkt_get(&iface.tx)) != NULL) {
		send(&pkt->buf);
		pkt_free(pkt);
	}
}

static pkt_t *tun_receive_pkt(void)
{
	pkt_t *pkt;
	uint8_t buf[2048];
	ssize_t nread = read(tun_fd, buf, sizeof(buf));

	if (nread < 0 && errno == EAGAIN) {
		sleep(1);
		return NULL;
	}

	CHECK(nread >= 0);
	if (nread == 0)
		return NULL;
#if 0
	printf("read: %ld\n", nread);
	for (i = 0; i < nread; i++) {
		printf(" 0x%X", buf[i]);
	}
	puts("");
#endif
	/* Writes have higher priority.
	 * Don't allocate new packets if there are queued TX pkts.
	 */
	tun_send_pkt();

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "can't alloc a packet\n");
		return NULL;
	}

	if (buf_add(&pkt->buf, buf, nread) < 0) {
		pkt_free(pkt);
		return NULL;
	}
	return pkt;
}

int main(int argc, char *argv[])
{
	char dev[IFNAMSIZ+1];

	memset(dev, 0, sizeof(dev));
	if (argc > 1)
		strncpy(dev, argv[1], sizeof(dev) - 1);

#if defined USE_CAPABILITIES
	cap_t caps = cap_get_proc();
	CHECK(caps != NULL);

	cap_value_t cap = CAP_NET_ADMIN;
	const char *capname = STRING(CAP_NET_ADMIN);
	cap_flag_value_t cap_effective;
	cap_flag_value_t cap_inheritable;

	/* Check that we have the required capabilities */
	/* At this point we only require CAP_NET_ADMIN to be permitted, */
	/* not effective as we will be enabling it later. */
	cap_flag_value_t cap_permitted;
	CHECKSYS(cap_get_flag(caps, cap, CAP_PERMITTED, &cap_permitted));

	CHECKSYS(cap_get_flag(caps, cap, CAP_EFFECTIVE, &cap_effective));
	CHECKSYS(cap_get_flag(caps, cap, CAP_INHERITABLE, &cap_inheritable));
	if (!cap_permitted) {
		fprintf(stderr, "%s not permitted, exiting\n", capname);
		exit(0);
	}

	/* And retain only what we require */
	CHECKSYS(cap_clear(caps));
	/* We must leave it permitted */
	CHECKSYS(cap_set_flag(caps, CAP_PERMITTED, 1, &cap, CAP_SET));
	/* but also make it effective */
	CHECKSYS(cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, CAP_SET));
	CHECKSYS(cap_set_proc(caps));
#endif
	tun_fd = tun_alloc(dev);
	if (tun_fd < 0) exit(0);

#if defined USE_CAPABILITIES
	/* And before anything else, clear all our capabilities */
	CHECKSYS(cap_clear(caps));
	CHECKSYS(cap_set_proc(caps));
	CHECKSYS(cap_free(caps));
#endif
	if (if_init(&iface, &send, &recv) < 0) {
		fprintf(stderr, "can't init interface\n");
		return -1;
	}
	if (pkt_mempool_init() < 0) {
		fprintf(stderr, "can't initialize pkt pool\n");
		return -1;
	}

	timer_subsystem_init(1000000);

	socket_init();
	dft_route.iface = &iface;
	if (udp_init() < 0) {
		fprintf(stderr, "can't init udp server\n");
		return -1;
	}

	if (tcp_init() < 0) {
		fprintf(stderr, "can't init tcp server\n");
		return -1;
	}

	if (fcntl(tun_fd, F_SETFL, O_NONBLOCK) < 0) {
		fprintf(stderr, "can't set non blocking tcp socket (%m)\n");
		return -1;
	}
	while (1) {
		pkt_t *pkt = tun_receive_pkt();

		if (pkt == NULL)
			continue;

		eth_input(pkt, &iface);
		udp_app();
		tcp_app();

		tun_send_pkt();
	}
	return 0;
}
