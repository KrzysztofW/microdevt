#include <log.h>

#include "net_apps.h"
#include "../timer.h"
#include "../sys/errno.h"


static uint16_t port = 777; /* host endian */

#ifdef CONFIG_TCP_CLIENT
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static uint32_t client_addr = 0x01020101;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static uint32_t client_addr = 0x01010201;
#endif

static tim_t tcp_client_timer;
#define TCP_TIMER_RECONNECT 5	/* reconnect every 5 seconds */
sock_info_t sock_info_client;

static int tcp_app_client_init(void);
static void tcp_client_send_buf_cb(void *arg);

static void tcp_client_connect_cb(void *arg)
{
	sock_info_t *sock_info = arg;

	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_connect(sock_info, client_addr, htons(port + 1)) >= 0) {
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
			  tcp_client_send_buf_cb, sock_info);
		return;
	}
	DEBUG_LOG("can't connect\n");
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000UL);
}

static int tcp_app_client_init(void)
{
	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_init(&sock_info_client, SOCK_STREAM) < 0) {
		DEBUG_LOG("can't init tcp sock_info\n");
		return -1;
	}

	timer_add(&tcp_client_timer, 1000000,
		  tcp_client_connect_cb, &sock_info_client);

	return 0;
}

static void tcp_client_send_buf_cb(void *arg)
{
	sbuf_t sb = SBUF_INITS("blabla\n");
	pkt_t *pkt;
	sock_info_t *sock_info = arg;

	printf("%s\n", __func__);
	if (__socket_put_sbuf(sock_info, &sb, 0, 0) < 0) {
		if (socket_info_state(sock_info) != SOCK_CONNECTED) {
			sock_info_close(sock_info);
			tcp_app_client_init();
			return;
		}
	}
	if (__socket_get_pkt(sock_info, &pkt, NULL, NULL) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		(void)sb;
		DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
		pkt_free(pkt);
		sock_info_close(sock_info);
	}
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000);
}
#endif

sock_info_t sock_info_server;

int tcp_server(void)
{
	if (sock_info_init(&sock_info_server, SOCK_STREAM) < 0) {
		DEBUG_LOG("can't init tcp sock_info\n");
		return -1;
	}
	if (sock_info_listen(&sock_info_server, 5) < 0) {
		DEBUG_LOG("can't listen on tcp sock_info\n");
		return -1;
	}

	if (sock_info_bind(&sock_info_server, htons(port)) < 0) {
		DEBUG_LOG("can't start tcp server\n");
		return -1;
	}

	return 0;
}

int tcp_init(void)
{
	if (tcp_server() < 0) {
		DEBUG_LOG("can't create TCP socket\n");
		return -1;
	}
#ifdef CONFIG_TCP_CLIENT
	tcp_app_client_init();
#endif
	return 0;
}

#define TCP_CLIENTS 5
sock_info_t sock_info_clients[TCP_CLIENTS];
sbuf_t sb[TCP_CLIENTS];
pkt_t *pkts[TCP_CLIENTS];
void tcp_app(void)
{
	uint32_t src_addr;
	uint16_t src_port;
	int i;

	for (i = 0; i < TCP_CLIENTS; i++) {
		if (sock_info_clients[i].trq.tcp_conn == NULL) {
			if (sock_info_accept(&sock_info_server, &sock_info_clients[i],
					     &src_addr, &src_port) >= 0) {
				DEBUG_LOG("accepted connection from:0x%lX on port %u\n",
				       ntohl(src_addr), (uint16_t)ntohs(src_port));
			}
			continue;
		}
		if (sb[i].len == 0) {
		    if (__socket_get_pkt(&sock_info_clients[i], &pkts[i], &src_addr,
				     &src_port) >= 0) {
			sb[i] = PKT2SBUF(pkts[i]);
			DEBUG_LOG("[conn:%d]: got (len:%d):%.*s (pkt:%p)\n", i,
				  sb[i].len, sb[i].len, sb[i].data, pkts[i]);
		    } else if (socket_info_state(&sock_info_clients[i]) != SOCK_CONNECTED) {
			    sock_info_close(&sock_info_clients[i]);
			    goto reset_sb;
		    }
		}
		if (__socket_put_sbuf(&sock_info_clients[i], &sb[i], 0,
				      0) < 0) {
			DEBUG_LOG("can't put sbuf to socket (len:%d) (from pkt:%p)\n",
				  sb[i].len, pkts[i]);
			if (socket_info_state(&sock_info_clients[i]) != SOCK_CONNECTED) {
				sock_info_close(&sock_info_clients[i]);
				goto reset_sb;
			}
			continue;
		}
	reset_sb:
		if (sb[i].len) {
			sbuf_reset(&sb[i]);
			pkt_free(pkts[i]);
		}
	}
}
