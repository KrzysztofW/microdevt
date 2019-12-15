/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#include <log.h>
#include <errno.h>

#include "net-apps.h"
#include "../timer.h"

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
#ifdef CONFIG_EVENT
static void ev_connecting_cb(event_t *ev, uint8_t events);
#endif
static void tcp_client_connect_cb(void *sock_info)
{
	if (sock_info_init(sock_info, SOCK_STREAM) < 0) {
		DEBUG_LOG("cannot init tcp sock_info\n");
		return;
	}
	if (sock_info_connect(sock_info, client_addr, htons(port + 1)) >= 0) {
#ifdef CONFIG_EVENT
		socket_event_register(&sock_info_client, EV_WRITE,
				      ev_connecting_cb);
#endif
		return;
	}

	DEBUG_LOG("cannot connect\n");
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT);
}

static void tcp_app_client_init(void)
{
	DEBUG_LOG("%s\n", __func__);
	timer_init(&tcp_client_timer);
	timer_add(&tcp_client_timer, 0, tcp_client_connect_cb,
		  &sock_info_client);
}

#ifdef CONFIG_EVENT
static void tcp_client_set_write_mask(void *sock_info)
{
	socket_event_set_mask(sock_info, EV_READ | EV_WRITE);
}

static void ev_client_cb(event_t *ev, uint8_t events)
{
	sock_info_t *sock_info = socket_event_get_sock_info(ev);
	pkt_t *pkt;

	if (events & EV_READ && __socket_get_pkt(sock_info, &pkt, NULL,
						 NULL) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		(void)sb;
		DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
		pkt_free(pkt);
	}

	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("client disconnected\n");
		socket_event_unregister(sock_info);
		sock_info_close(sock_info);
		timer_del(&tcp_client_timer);
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT,
			  tcp_client_connect_cb, sock_info);
		return;
	}

	if (events & EV_WRITE) {
		sbuf_t sb = SBUF_INITS("blabla\n");
		pkt_t *pkt;

		if (__socket_put_sbuf(sock_info, &sb, 0, 0) < 0)
			return;
		if (__socket_get_pkt(sock_info, &pkt, NULL, NULL) >= 0) {
			sb = PKT2SBUF(pkt);
			DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
			pkt_free(pkt);
		}
		socket_event_set_mask(sock_info, EV_READ);
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT,
			  tcp_client_set_write_mask, sock_info);
	}
}

static void ev_connecting_cb(event_t *ev, uint8_t events)
{
	sock_info_t *sock_info = socket_event_get_sock_info(ev);

	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("failed to connect\n");
		socket_event_unregister(sock_info);
		sock_info_close(sock_info);
		timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT);
		return;
	}
	if (events & EV_WRITE) {
		DEBUG_LOG("%s: succeeded\n", __func__);
		socket_event_register(sock_info, EV_READ | EV_WRITE,
				      ev_client_cb);
	}
}

#endif	/* CONFIG_EVENT */
#endif	/* CONFIG_TCP_CLIENT */

static sock_info_t sock_info_server;

#ifdef CONFIG_EVENT
static void ev_accept_cb(event_t *ev, uint8_t events);
#endif

int tcp_server(void)
{
	if (sock_info_init(&sock_info_server, SOCK_STREAM) < 0) {
		DEBUG_LOG("cannot init tcp sock_info\n");
		return -1;
	}
	if (sock_info_listen(&sock_info_server, 5) < 0) {
		DEBUG_LOG("cannot listen on tcp sock_info\n");
		return -1;
	}

#ifdef CONFIG_EVENT
	socket_event_register(&sock_info_server, EV_READ, ev_accept_cb);
#endif
	if (sock_info_bind(&sock_info_server, htons(port)) < 0) {
		DEBUG_LOG("cannot start tcp server\n");
		return -1;
	}
	return 0;
}

int tcp_app_init(void)
{
	if (tcp_server() < 0) {
		DEBUG_LOG("cannot create TCP socket\n");
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
			DEBUG_LOG("cannot put sbuf to socket (len:%d) (from pkt:%p)\n",
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
static void ev_clients_cb(event_t *ev, uint8_t events)
{
	sock_info_t *sock_info = socket_event_get_sock_info(ev);
	sock_info_client_ctx_t *ctx;
	uint32_t src_addr;
	uint16_t src_port;

	ctx = container_of(sock_info, sock_info_client_ctx_t, sock_info);

	if (events & EV_READ) {
		if (ctx->sb.len != 0) {
			socket_event_set_mask(&ctx->sock_info, EV_WRITE);
			return;
		}
		if (__socket_get_pkt(&ctx->sock_info, &ctx->pkt,
				     &src_addr, &src_port) >= 0) {
			ctx->sb = PKT2SBUF(ctx->pkt);
			DEBUG_LOG("[conn:%p]: got (len:%d):%.*s (pkt:%p)\n",
				  ctx, ctx->sb.len, ctx->sb.len, ctx->sb.data,
				  ctx->pkt);
			socket_event_set_mask(&ctx->sock_info,
					      EV_READ | EV_WRITE);
		}
	}

	if (events & (EV_ERROR | EV_HUNGUP)) {
		LOG("[conn:%p] disconnected\n", ctx);
		goto error;
	}

	if (events & EV_WRITE) {
		if (__socket_put_sbuf(&ctx->sock_info, &ctx->sb, 0,
				      0) < 0) {
			LOG("%s:%d write failed\n", __func__, __LINE__);
			return;
		}
		pkt_free(ctx->pkt);
		sbuf_reset(&ctx->sb);
		socket_event_set_mask(&ctx->sock_info, EV_READ);
	}
	return;
 error:
	socket_event_unregister(sock_info);
	sock_info_close(sock_info);
	if (ctx->sb.len) {
		sbuf_reset(&ctx->sb);
		pkt_free(ctx->pkt);
	}
}

static void ev_accept_cb(event_t *ev, uint8_t events)
{
	sock_info_t *sock_info = socket_event_get_sock_info(ev);
	uint32_t src_addr;
	uint16_t src_port;
	int i;

	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("connect closed\n");
		socket_event_unregister(sock_info);
		socket_event_register(&sock_info_server, EV_READ, ev_accept_cb);
		return;
	}

	if ((events & EV_READ) == 0)
		return;

	for (i = 0; i < TCP_CLIENTS; i++) {
		if (sock_info_state(&ctx[i].sock_info) != SOCK_CLOSED)
			continue;
		if (sock_info_accept(sock_info, &ctx[i].sock_info,
				     &src_addr, &src_port) >= 0) {
			DEBUG_LOG("accepted connection [%p] from:0x%X on port %u\n",
				  &ctx[i],
				  (uint32_t)ntohl(src_addr),
				  (uint16_t)ntohs(src_port));
			socket_event_register(&ctx[i].sock_info, EV_READ,
					      ev_clients_cb);
		}
	}
}

#endif /* CONFIG_EVENT */
