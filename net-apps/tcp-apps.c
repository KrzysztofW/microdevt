#include <log.h>

#include "net-apps.h"
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
#define TCP_TIMER_RECONNECT 5000000UL /* reconnect every 5 seconds */

static sock_info_t sock_info_client;

static void tcp_app_client_init(void);

#ifndef CONFIG_EVENT
static void tcp_client_send_buf_cb(void *arg);

static void tcp_client_connect_cb(void *arg)
{
	sock_info_t *sock_info = arg;

	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_connect(sock_info, client_addr, htons(port + 1)) >= 0) {
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT,
			  tcp_client_send_buf_cb, sock_info);
		return;
	}
	DEBUG_LOG("can't connect\n");
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT);
}

static void tcp_app_client_init(void)
{
	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_init(&sock_info_client, SOCK_STREAM) < 0) {
		DEBUG_LOG("can't init tcp sock_info\n");
		return;
	}

	timer_add(&tcp_client_timer, 1000000,
		  tcp_client_connect_cb, &sock_info_client);
}

static void tcp_client_send_buf_cb(void *arg)
{
	sbuf_t sb = SBUF_INITS("blabla\n");
	pkt_t *pkt;
	sock_info_t *sock_info = arg;

	printf("%s\n", __func__);
	if (__socket_put_sbuf(sock_info, &sb, 0, 0) < 0) {
		if (sock_info_state(sock_info) != SOCK_CONNECTED) {
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
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT);
}
#else  /* CONFIG_EVENT */

static void tcp_app_client_init(void);
static void tcp_client_connect_cb(void *arg)
{
	tcp_app_client_init();
}

static void ev_connect_cb(sock_info_t *sock_info, uint8_t events)
{
	if (sock_info_state(sock_info) == SOCK_CLOSED)
		goto error;

	if (events & EV_READ) {
		sbuf_t sb = SBUF_INITS("blabla\n");
		pkt_t *pkt;

		if (__socket_put_sbuf(sock_info, &sb, 0, 0) < 0)
			goto error;
		if (__socket_get_pkt(sock_info, &pkt, NULL, NULL) >= 0) {
			sb = PKT2SBUF(pkt);
			DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
			pkt_free(pkt);
		}
	}
	return;
 error:
	DEBUG_LOG("tcp client: closing connection\n");
	sock_info_close(sock_info);
	timer_del(&tcp_client_timer);
	timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT,
		  tcp_client_connect_cb, sock_info);
}


static void tcp_app_client_init(void)
{
	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_init(&sock_info_client, SOCK_STREAM) < 0) {
		DEBUG_LOG("can't init tcp sock_info\n");
		return;
	}
	if (sock_info_connect(&sock_info_client, client_addr, htons(port + 1)) >= 0) {
		socket_ev_set(&sock_info_client, EV_READ, ev_connect_cb);
		return;
	}
	DEBUG_LOG("can't connect\n");
}
#endif	/* CONFIG_EVENT */
#endif	/* CONFIG_TCP_CLIENT */

static sock_info_t sock_info_server;

#ifdef CONFIG_EVENT
static void ev_accept_cb(sock_info_t *sock_info, uint8_t events);
#endif

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

#ifdef CONFIG_EVENT
	socket_ev_set(&sock_info_server, EV_READ, ev_accept_cb);
#endif
	return 0;
}

int tcp_app_init(void)
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
typedef struct sock_info_client_ctx {
	sock_info_t sock_info;
	sbuf_t sb;
	pkt_t *pkt;
} sock_info_client_ctx_t;
static sock_info_client_ctx_t ctx[TCP_CLIENTS];

#ifndef CONFIG_EVENT
void tcp_app(void)
{
	uint32_t src_addr;
	uint16_t src_port;
	int i;

	for (i = 0; i < TCP_CLIENTS; i++) {
		if (ctx[i].sock_info.trq.tcp_conn == NULL) {
			if (sock_info_accept(&sock_info_server, &ctx[i].sock_info,
					     &src_addr, &src_port) >= 0) {
				DEBUG_LOG("accepted connection from:0x%X on port %u\n",
					  (uint32_t)ntohl(src_addr),
					  (uint16_t)ntohs(src_port));
			}
			continue;
		}
		if (ctx[i].sb.len == 0) {
			if (__socket_get_pkt(&ctx[i].sock_info, &ctx[i].pkt,
					     &src_addr, &src_port) >= 0) {
				ctx[i].sb = PKT2SBUF(ctx[i].pkt);
				DEBUG_LOG("[conn:%d]: got (len:%d):%.*s (pkt:%p)\n", i,
					  ctx[i].sb.len, ctx[i].sb.len,
					  ctx[i].sb.data, ctx[i].pkt);
			} else if (sock_info_state(&ctx[i].sock_info) != SOCK_CONNECTED) {
				sock_info_close(&ctx[i].sock_info);
				goto reset_sb;
			}
		}
		if (__socket_put_sbuf(&ctx[i].sock_info, &ctx[i].sb, 0, 0) < 0) {
			DEBUG_LOG("can't put sbuf to socket (len:%d) (from pkt:%p)\n",
				  ctx[i].sb.len, ctx[i].pkt);
			if (sock_info_state(&ctx[i].sock_info) != SOCK_CONNECTED) {
				sock_info_close(&ctx[i].sock_info);
				goto reset_sb;
			}
			continue;
		}
	reset_sb:
		if (ctx[i].sb.len) {
			sbuf_reset(&ctx[i].sb);
			pkt_free(ctx[i].pkt);
		}
	}
}
#else /* CONFIG_EVENT */
static void ev_client_cb(sock_info_t *sock_info, uint8_t events)
{
	sock_info_client_ctx_t *ctx;
	uint32_t src_addr;
	uint16_t src_port;

	ctx = container_of(sock_info, sock_info_client_ctx_t, sock_info);
	if (sock_info_state(sock_info) == SOCK_CLOSED) {
		sock_info_close(&ctx->sock_info);
		goto reset_sb;
	}

	if ((events & EV_READ) == 0)
		return;

	if (ctx->sb.len == 0 && __socket_get_pkt(&ctx->sock_info, &ctx->pkt,
						 &src_addr, &src_port) >= 0) {
		ctx->sb = PKT2SBUF(ctx->pkt);
		DEBUG_LOG("[conn:%p]: got (len:%d):%.*s (pkt:%p)\n", ctx,
			  ctx->sb.len, ctx->sb.len,
			  ctx->sb.data, ctx->pkt);
	}
	if (__socket_put_sbuf(&ctx->sock_info, &ctx->sb, 0, 0) < 0) {
		DEBUG_LOG("can't put sbuf to socket (len:%d) (from pkt:%p)\n",
			  ctx->sb.len, ctx->pkt);
	}

 reset_sb:
	if (ctx->sb.len) {
		sbuf_reset(&ctx->sb);
		pkt_free(ctx->pkt);
	}
}

static void ev_accept_cb(sock_info_t *sock_info, uint8_t events)
{
	uint32_t src_addr;
	uint16_t src_port;
	int i;

	if ((events & EV_READ) == 0)
		return;

	for (i = 0; i < TCP_CLIENTS; i++) {
		if (ctx[i].sock_info.trq.tcp_conn)
			continue;
		if (sock_info_accept(sock_info, &ctx[i].sock_info,
				     &src_addr, &src_port) >= 0) {
			DEBUG_LOG("accepted connection from:0x%X on port %u\n",
				  (uint32_t)ntohl(src_addr),
				  (uint16_t)ntohs(src_port));
			socket_ev_set(&ctx[i].sock_info, EV_READ,
				      ev_client_cb);
		}
	}
}

#endif /* CONFIG_EVENT */
