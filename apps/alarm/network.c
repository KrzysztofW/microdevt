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

#include <sys/scheduler.h>
#include <enc28j60.h>
#include <net/config.h>
#include <net/eth.h>
#include <net/route.h>
#include <net/udp.h>
#include <net/socket.h>
#include <net-apps/net-apps.h>
#include <sys/timer.h>
#include <drivers/rf.h>

/* #define ENC28J60_INT */
/* #define NETWORK_WD_RESET */

static uint8_t ip[] = { 192, 168, 0, 99 };
static uint8_t ip_mask[] = { 255, 255, 255, 0 };
static uint8_t mac[] = { 0x62, 0x5F, 0x70, 0x72, 0x61, 0x79 };

iface_t eth_iface = {
	.flags = IF_UP|IF_RUNNING,
	.hw_addr = mac,
	.ip4_addr = ip,
	.ip4_mask = ip_mask,
	.send = &enc28j60_pkt_send,
	.recv = &enc28j60_pkt_recv,
};

static struct iface_queues {
	RING_DECL_IN_STRUCT(pkt_pool, CONFIG_PKT_DRIVER_NB_MAX);
	RING_DECL_IN_STRUCT(rx, CONFIG_PKT_NB_MAX);
	RING_DECL_IN_STRUCT(tx, CONFIG_PKT_NB_MAX);
} iface_queues = {
	.pkt_pool = RING_INIT(iface_queues.pkt_pool),
	.rx = RING_INIT(iface_queues.rx),
	.tx = RING_INIT(iface_queues.tx),
};

#ifdef NETWORK_WD_RESET
static uint8_t net_wd;
static tim_t timer_wd;

static void tim_cb_wd(void *arg)
{
	DEBUG_LOG("bip\n");
	if (net_wd > 1)
		DEBUG_LOG("resetting net device\n");
	net_wd++;
	timer_reschedule(&timer_wd, 10000000UL);
}
#endif

static int apps_init(void)
{
#ifdef CONFIG_UDP
	if (udp_app_init() < 0)
		return -1;
#endif
#ifdef CONFIG_TCP
	if (tcp_app_init() < 0)
		return -1;
#endif
#ifdef CONFIG_DNS
	if (dns_resolver_init() < 0)
		return -1;
#endif
#ifdef NETWORK_WD_RESET
#endif
	return 0;
}

#ifndef CONFIG_EVENT
static void apps_loop(void)
{
#if defined(CONFIG_UDP) && !defined(CONFIG_EVENT)
	udp_app();
#endif
#if defined(CONFIG_TCP) && !defined(CONFIG_EVENT)
	tcp_app();
#endif
}
#endif

#ifdef ENC28J60_INT
ISR(PCINT0_vect)
{
	enc28j60_handle_interrupts(&eth_iface);
}
#endif

void alarm_network_init(void)
{
#ifdef NETWORK_WD_RESET
	timer_add(&timer_wd, 5000000UL, tim_cb_wd, NULL);
#endif
	if_init(&eth_iface, IF_TYPE_ETHERNET, &iface_queues.pkt_pool,
		&iface_queues.rx, &iface_queues.tx, 0);

	dft_route.iface = &eth_iface;
	dft_route.ip = 0x0b00a8c0;
#if defined(CONFIG_UDP) || defined(CONFIG_TCP)
	socket_init();
#endif

#ifdef ENC28J60_INT
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT5);
#endif
	enc28j60_init(eth_iface.hw_addr);

	if (apps_init() < 0)
		__abort();
}

void alarm_network_loop(void)
{
	pkt_t *pkt;

	eth_iface.recv(&eth_iface);
	while ((pkt = pkt_get(eth_iface.tx)) != NULL) {
#ifdef NETWORK_WD_RESET
		net_wd = 0;
#endif
		eth_iface.send(&eth_iface, pkt);
	}

#ifdef ENC28J60_INT
	/* We must sleep if EIE_PKTIE is set and pkts are received here
	 * (not in the interrupt handler). */
	/* delay_us(10); */
#endif
#ifndef CONFIG_EVENT
	apps_loop();
#endif
}
