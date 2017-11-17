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
uint32_t src_addr_c;
uint16_t src_port_c;
sock_info_t sock_info_udp_client;
static tim_t udp_client_timer;
#define UDP_CLIENT_SEND_DELAY 1000000
#endif

sock_info_t sock_info_udp_server;

int udp_server(void)
{
	if (sock_info_init(&sock_info_udp_server, SOCK_DGRAM) < 0) {
		DEBUG_LOG("can't init udp sock_info\n");
		return -1;
	}

	if (sock_info_bind(&sock_info_udp_server, htons(port)) < 0) {
		DEBUG_LOG("can't start udp server\n");
		return -1;
	}
	return 0;
}

#ifdef UDP_CLIENT
static void udp_client_send_buf_cb(void *arg)
{
	pkt_t *pkt;
	sbuf_t sb = SBUF_INITS("blabla\n");
	uint16_t dst_port;

	if (__socket_put_sbuf(&sock_info_udp_client, &sb, src_addr_c,
			      src_port_c) < 0)
		DEBUG_LOG("can't put sbuf to socket\n");
	if (__socket_get_pkt(&sock_info_udp_client, &pkt, &src_addr_c,
			     &dst_port) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);
		char *s = (char *)sb.data;

		s[sb.len] = '\0';
		DEBUG_LOG("%s", s);
		pkt_free(pkt);
	}
	timer_reschedule(&udp_client_timer, UDP_CLIENT_SEND_DELAY);
}

int udp_client(void)
{
	if (sock_info_init(&sock_info_udp_client, SOCK_DGRAM) < 0) {
		DEBUG_LOG("can't init udp sock_info\n");
		return -1;
	}
	src_addr_c = client_addr;
	src_port_c = htons(port + 1);
	timer_add(&udp_client_timer, UDP_CLIENT_SEND_DELAY,
		  udp_client_send_buf_cb, NULL);

	return 0;
}
#endif

int udp_init(void)
{
	if (udp_server() < 0) {
		DEBUG_LOG("can't create UDP server socket\n");
		return -1;
	}
#ifdef UDP_CLIENT
	if (udp_client() < 0) {
		DEBUG_LOG("can't create UDP client socket\n");
		return -1;
	}
#endif
	return 0;
}

void udp_app(void)
{
	uint32_t src_addr;
	uint16_t src_port;
	pkt_t *pkt;

	if (__socket_get_pkt(&sock_info_udp_server, &pkt, &src_addr,
			     &src_port) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		if (__socket_put_sbuf(&sock_info_udp_server, &sb, src_addr,
				      src_port) < 0)
			DEBUG_LOG("can't put sbuf to udp socket\n");
		pkt_free(pkt);
	}
}
