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

#include "config.h"
#include "pkt-mempool.h"
#ifdef CONFIG_ETHERNET
#include "eth.h"
#endif
#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
#include "swen.h"
#endif
#include <scheduler.h>

static void if_refill_driver_pkt_pool(const iface_t *iface)
{
	pkt_t *pkt;

	while (!ring_is_full(iface->pkt_pool) && (pkt = pkt_alloc()))
		pkt_put(iface->pkt_pool, pkt);
}

void if_init(iface_t *ifce, uint8_t type, ring_t *pkt_pool, ring_t *rx,
	     ring_t *tx, uint8_t is_interrupt_driven)
{
	switch (type) {
#ifdef CONFIG_ETHERNET
	case IF_TYPE_ETHERNET:
		assert(ifce->recv);
		assert(ifce->send);
		ifce->if_output = &eth_output;
		ifce->if_input = &eth_input;
		break;
#endif
#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
	case IF_TYPE_RF:
#ifdef CONFIG_RF_SENDER
		assert(ifce->send);
		ifce->if_output = &swen_output;
#endif
#ifdef CONFIG_RF_RECEIVER
		assert(ifce->recv);
		ifce->if_input = &swen_input;
#endif
		break;
#endif
	default:
		__abort();
	}
	ifce->rx = rx;
	ifce->tx = tx;

	if (is_interrupt_driven) {
		ifce->pkt_pool = pkt_pool;
		if_refill_driver_pkt_pool(ifce);
	}
}

static void if_schedule_receive_cb(void *arg)
{
	iface_t *iface = arg;

	if_refill_driver_pkt_pool(iface);
	iface->if_input(iface);
}

void if_schedule_receive(const iface_t *iface, pkt_t *pkt)
{
	if (ring_is_empty(iface->rx) || ring_is_empty(iface->pkt_pool))
		schedule_task(if_schedule_receive_cb, (iface_t *)iface);
	if (pkt)
		pkt_put(iface->rx, pkt);
}

static void if_pkt_free_cb(void *arg)
{
	pkt_free(arg);
}

void if_schedule_tx_pkt_free(pkt_t *pkt)
{
	schedule_task(if_pkt_free_cb, pkt);
}
