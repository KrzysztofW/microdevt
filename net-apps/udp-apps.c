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
#include <sys/timer.h>

#include "net-apps.h"

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

static void udp_get_pkt(sock_info_t *sock_info)
{
	uint32_t src_addr;
	uint16_t src_port;
	pkt_t *pkt;

	if (__socket_get_pkt(sock_info, &pkt, &src_addr, &src_port) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		if (__socket_put_sbuf(sock_info, &sb, src_addr, src_port) < 0)
			DEBUG_LOG("can't put sbuf to udp socket\n");

		src_port = ntohs(src_port);
		DEBUG_LOG("got from 0x%X on port %u: %.*s\n", src_addr,
			  src_port, sb.len, sb.data);
		pkt_free(pkt);
	}
}

#ifdef CONFIG_EVENT
static void ev_udp_server_cb(event_t *ev, uint8_t events)
{
	sock_info_t *sock_info = socket_event_get_sock_info(ev);

	DEBUG_LOG("received udp read event\n");
	udp_get_pkt(sock_info);
}
#endif

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
#ifdef CONFIG_EVENT
	socket_event_register(&sock_info_udp_server, EV_READ, ev_udp_server_cb);
#endif

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

int udp_app_init(void)
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
	udp_get_pkt(&sock_info_udp_server);
}
