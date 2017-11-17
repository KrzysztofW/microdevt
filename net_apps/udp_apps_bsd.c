#include <log.h>

#include "net_apps.h"
#include "../timer.h"
#include "../sys/errno.h"

static uint16_t port = 777; /* host endian */
//#define UDP_CLIENT
#ifdef UDP_CLIENT
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static uint32_t client_addr = 0x01020101;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static uint32_t client_addr = 0x01010201;
#endif
int udp_fd_client;
tim_t udp_client_timer;
struct sockaddr_in addr_c;
#define UDP_CLIENT_SEND_DELAY 1000000
#endif

int udp_server(void)
{
	int fd;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		DEBUG_LOG("can't create socket\n");
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
		DEBUG_LOG("can't bind\n");
		return -1;
	}
	return fd;
}

#ifdef UDP_CLIENT
void udp_client_cb(void *arg)
{
	sbuf_t sb = SBUF_INITS("blabla\n");
	pkt_t *pkt;
	struct sockaddr_in addr;

	if (socket_put_sbuf(udp_fd_client, &sb, &addr_c) < 0)
		DEBUG_LOG("can't put sbuf to socket\n");
	if (socket_get_pkt(udp_fd_client, &pkt, &addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);
		char *s = (char *)sb.data;

		s[sb.len] = 0;
		DEBUG_LOG("%s", s);
		pkt_free(pkt);
	}
	timer_reschedule(&udp_client_timer, UDP_CLIENT_SEND_DELAY);
}

int udp_client(struct sockaddr_in *sockaddr)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		DEBUG_LOG("can't create socket\n");
		return -1;
	}
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = client_addr;
	sockaddr->sin_port = htons(port + 1);
	timer_add(&udp_client_timer, UDP_CLIENT_SEND_DELAY, udp_client_cb, NULL);

	return fd;
}
#endif

int udp_fd;
int udp_init(void)
{
	udp_fd = udp_server();
#ifdef UDP_CLIENT
	udp_fd_client = udp_client(&addr_c);
	if (udp_fd_client < 0) {
		DEBUG_LOG("can't create UDP client socket\n");
		return -1;
	}
#endif
	if (udp_fd < 0) {
		DEBUG_LOG("can't create UDP server socket\n");
		return -1;
	}
	return 0;
}

void udp_app(void)
{
	pkt_t *pkt;
	struct sockaddr_in addr;

	if (socket_get_pkt(udp_fd, &pkt, &addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		if (socket_put_sbuf(udp_fd, &sb, &addr) < 0)
			DEBUG_LOG("can't put sbuf to socket\n");
		pkt_free(pkt);
	}
}
