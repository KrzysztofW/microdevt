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

#include "net-apps.h"
#include "../timer.h"
#include "../sys/errno.h"

static uint16_t port = 777; /* host endian */
struct sockaddr_in addr_c;

int tcp_server(void)
{
	int fd = 0;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
	if (listen(fd, 5) < 0) {
		DEBUG_LOG("can't listen\n");
		return -1;
	}

	return fd;
}

#ifdef CONFIG_TCP_CLIENT
static int tcp_client_fd = 1;
static tim_t tcp_client_timer;
static struct sockaddr_in tcp_client_sockaddr;
#define TCP_TIMER_RECONNECT 5	/* reconnect every 5 seconds */

static int tcp_app_client_init(void);
static void tcp_client_send_buf_cb(void *arg);

static void tcp_client_connect_cb(void *arg)
{
	(void)arg;

	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (connect(tcp_client_fd, (struct sockaddr *)&tcp_client_sockaddr,
		    sizeof(struct sockaddr_in)) >= 0) {
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
			  tcp_client_send_buf_cb, NULL);
		return;
	}
	DEBUG_LOG("can't connect errno:%d\n", errno);
	if (errno != EAGAIN) {
		close(tcp_client_fd);
		tcp_app_client_init();
		return;
	}

	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000UL);
}

static int tcp_app_client_init(void)
{
	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if ((tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_LOG("TCP client: can't create socket\n");
		return -1;
	}
	tcp_client_sockaddr.sin_family = AF_INET;
	tcp_client_sockaddr.sin_addr.s_addr = 0x01020101;
	tcp_client_sockaddr.sin_port = htons(port + 1);

	timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
		  tcp_client_connect_cb, NULL);

	return 0;
}

static void tcp_client_send_buf_cb(void *arg)
{
	sbuf_t sb = SBUF_INITS("blabla\n");
	pkt_t *pkt;

	(void)arg;
	if (socket_put_sbuf(tcp_client_fd, &sb, &tcp_client_sockaddr) < 0) {
		if (errno != EAGAIN) {
			close(tcp_client_fd);
			tcp_app_client_init();
			return;
		}
		goto reschedule;
	}
	if (socket_get_pkt(tcp_client_fd, &pkt, &tcp_client_sockaddr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		(void)sb;
		DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
		pkt_free(pkt);
	}
 reschedule:
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000);
}
#endif

static int tcp_fd;
int tcp_app_init(void)
{
	tcp_fd = tcp_server();
	if (tcp_fd < 0) {
		DEBUG_LOG("can't create TCP socket\n");
		return -1;
	}
#ifdef CONFIG_TCP_CLIENT
	tcp_app_client_init();
#endif
	return 0;
}

void tcp_app(void)
{
	pkt_t *pkt;
	struct sockaddr_in addr;
	socklen_t addr_len;
	static int client_fd = -1;

	if (client_fd < 0) {
		if ((client_fd = accept(tcp_fd, (struct sockaddr *)&addr, &addr_len)) >= 0)
			DEBUG_LOG("accepted connection from:0x%X on port %u\n",
			       ntohl(addr.sin_addr.s_addr),
			       (uint16_t)ntohs(addr.sin_port));
	}
	if (socket_get_pkt(client_fd, &pkt, &addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		DEBUG_LOG("got:%.*s\n", sb.len, sb.data);
		if (socket_put_sbuf(client_fd, &sb, &addr) < 0)
			DEBUG_LOG("can't put sbuf to socket\n");
		pkt_free(pkt);
		return;
	}
	if (errno == EBADF) {
		close(client_fd);
		client_fd = -1;
	}
}
