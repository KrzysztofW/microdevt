#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef CONFIG_USE_CAPABILITIES
#include <sys/capability.h>
#define STRING(e) #e
#endif

#include <timer.h>
#include <scheduler.h>
#include <net/eth.h>
#include <net/route.h>
#define SOCKLEN_DEFINED
#include <net/socket.h>
#undef SOCKLEN_DEFINED
#include <net-apps/net-apps.h>
#include <net/pkt-mempool.h>
#include "tun.h"

static uint8_t ip[] = { 1, 1, 2, 2 };
static uint8_t ip_mask[] = { 255, 255, 255, 0 };
static uint8_t mac[] = { 0x54, 0x52, 0x00, 0x02, 0x00, 0x41 };
static struct pollfd tun_fds[1];

static int send(const iface_t *iface, pkt_t *pkt);
static void recv(const iface_t *iface) {}

static iface_t iface = {
	.flags = IF_UP|IF_RUNNING,
	.hw_addr = mac,
	.ip4_addr = ip,
	.ip4_mask = ip_mask,
	.send = &send,
	.recv = &recv,
};

static int send(const iface_t *iface, pkt_t *pkt)
{
	ssize_t nwrite;

	nwrite = write(tun_fds[0].fd, buf_data(&pkt->buf), pkt->buf.len);
	pkt_free(pkt);
	if (nwrite < 0) {
		if (errno != EAGAIN) {
			fprintf(stderr, "tun device is not up (%m)\n");
			exit(EXIT_FAILURE);
		}
		return -1;
	}
	return 0;
}

static int tun_receive_pkt(const iface_t *iface)
{
	pkt_t *pkt;
	uint8_t buf[2048];
	ssize_t nread;

	if (poll(tun_fds, 1, -1) < 0) {
		if (errno == EINTR)
			return -1;
		fprintf(stderr, "cannot poll on tun fd (%m (%d))\n", errno);
		return -1;
	}
	if ((tun_fds[0].revents & POLLIN) == 0)
		return -1;

	nread = read(tun_fds[0].fd, buf, sizeof(buf));
	if (nread < 0 && errno == EAGAIN) {
		return -1;
	}

	if (nread < 0) {
		if (errno == EAGAIN)
			return -1;
		fprintf(stderr, "%s: reading tun device failed (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
	if (nread == 0)
		return -1;
#if 0
	printf("read: %ld\n", nread);
	for (int i = 0; i < nread; i++)
		printf(" 0x%X", buf[i]);
	puts("");
#endif
	/* Writes have higher priority.
	 * Don't allocate new packets if there are queued TX pkts.
	 */

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "cannot alloc packet\n");
		return -1;
	}

	if (buf_add(&pkt->buf, buf, nread) < 0) {
		pkt_free(pkt);
		return -1;
	}
	pkt_put(iface->rx, pkt);
	return 0;
}

int main(int argc, char *argv[])
{
	char dev[256];
	char *dev_name;
	char cmd[512];
#ifdef CONFIG_USE_CAPABILITIES
	cap_t caps;
	cap_value_t cap = CAP_NET_ADMIN;
	const char *capname = STRING(CAP_NET_ADMIN);
	cap_flag_value_t cap_effective;
	cap_flag_value_t cap_inheritable;
	cap_flag_value_t cap_permitted;
#endif
	memset(dev, 0, sizeof(dev));
	if (argc > 1)
		strncpy(dev, argv[1], sizeof(dev) - 1);

#ifdef CONFIG_USE_CAPABILITIES
	if ((caps = cap_get_proc()) == NULL) {
		fprintf(stderr, "%s: cannot get capabilities (%m)\n", __func__);
		exit(EXIT_FAILURE);
	}

	/* Check that we have the required capabilities */
	/* At this point we only require CAP_NET_ADMIN to be permitted, */
	/* not effective as we will be enabling it later. */
	if (cap_get_flag(caps, cap, CAP_PERMITTED, &cap_permitted) < 0) {
		fprintf(stderr,
			"%s: cannot get capabilities PERMITTED flag (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}

	if (cap_get_flag(caps, cap, CAP_EFFECTIVE, &cap_effective) < 0) {
		fprintf(stderr,
			"%s: cannot get capabilities EFFECTIVE flag (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
	if (cap_get_flag(caps, cap, CAP_INHERITABLE, &cap_inheritable) < 0) {
		fprintf(stderr,
			"%s: cannot get capabilities INHERITABLE flag (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
	if (!cap_permitted) {
		fprintf(stderr, "%s not permitted, exiting\n", capname);
		exit(0);
	}

	/* And retain only what we require */
	if (cap_clear(caps) < 0) {
		fprintf(stderr, "%s: cannot clear capabilities (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
	/* We must leave it permitted */
	if (cap_set_flag(caps, CAP_PERMITTED, 1, &cap, CAP_SET) < 0) {
		fprintf(stderr,
			"%s: cannot set capabilities PERMITTED flag (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
	/* but also make it effective */
	if (cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, CAP_SET) < 0) {
		fprintf(stderr,
			"%s: cannot set capabilities EFFECTIVE flag (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
	if (cap_set_proc(caps) < 0) {
		fprintf(stderr,
			"%s: cannot set capabilities (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}

#endif
	tun_alloc(dev, tun_fds);
	if (tun_fds[0].fd < 0)
		exit(0);

	dev_name = argc ? dev : "tap0";
	snprintf(cmd, 512, "sudo ip link set up dev %s && "
		 "sudo ip a a 1.1.2.1/24 dev %s",  dev_name, dev_name);
	if (system(cmd) < 0)
		fprintf(stderr, "failed to run command: `%s'\n", cmd);
	printf("the system is available on IP address 1.1.2.2 (interface: %s)\n",
	       dev_name);

#ifdef CONFIG_USE_CAPABILITIES
	/* And before anything else, clear all our capabilities */
	if (cap_clear(caps) < 0 || cap_set_proc(caps) < 0 || cap_free(caps) < 0) {
		fprintf(stderr, "%s: cannot free capabilities (%m)\n",
			__func__);
		exit(EXIT_FAILURE);
	}
#endif
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	if_init(&iface, IF_TYPE_ETHERNET, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 0);

	if (fcntl(tun_fds[0].fd, F_SETFL, O_NONBLOCK) < 0) {
		fprintf(stderr, "cannot set non blocking tcp socket (%m)\n");
		return -1;
	}

	timer_subsystem_init();

#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	scheduler_init();

	socket_init();
	dft_route.iface = &iface;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	dft_route.ip = 0x01020101;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	dft_route.ip = 0x01010201;
#endif

#ifdef CONFIG_UDP
	if (udp_app_init() < 0) {
		fprintf(stderr, "cannot init udp sockets\n");
		return -1;
	}
#endif
#ifdef CONFIG_TCP
	if (tcp_app_init() < 0) {
		fprintf(stderr, "cannot init tcp sockets\n");
		return -1;
	}
#endif
#ifdef CONFIG_DNS
	if (dns_resolver_init() < 0) {
		fprintf(stderr, "cannot init dns resolver\n");
		return -1;
	}
#endif

	while (1) {
		if (tun_receive_pkt(&iface) >= 0)
			iface.if_input(&iface);

		scheduler_run_tasks();
#if defined(CONFIG_TCP) && !defined(CONFIG_EVENT)
		udp_app();
#endif
#if defined(CONFIG_TCP) && !defined(CONFIG_EVENT)
		tcp_app();
#endif
	}
	return 0;
}
