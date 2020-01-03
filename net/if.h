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

#ifndef _IF_H_
#define _IF_H_
#include <sys/buf.h>
#include <sys/list.h>

#include "config.h"

#define IF_UP      (1 << 0)
#define IF_RUNNING (1 << 1)
#define IF_PROMISC (1 << 2)
#define IF_NOARP   (1 << 3)
/*      IF_LAST    (1 << 7) */

typedef enum if_type {
	IF_TYPE_ETHERNET,
	IF_TYPE_RF,
} if_type_t;

struct iface {
	uint8_t flags;
	uint8_t type;
	uint8_t *hw_addr;

	/* only one ip address allowed (network endianess) */
#ifdef CONFIG_IP
	uint8_t *ip4_addr;
	uint8_t *ip4_mask;
#endif
#ifdef CONFIG_IPV6
	uint8_t *ip6_addr;
#endif
	/* low level driver functions used in interrupt handlers */
	/* asynchronious sending if possible */
	int (*send)(struct iface *iface, pkt_t *pkt);
	void (*recv)(struct iface *iface);

	/* private interface handle */
	void *priv;

	/* level 2 functions used in bottom halves */
	/* points to eth_output() or swen_output() */
	int (*if_output)(pkt_t *out, struct iface *iface, uint8_t type,
			 const void *dst);
	/* points to eth_input() or swen_input() */
	void (*if_input)(struct iface *iface);

#ifdef CONFIG_IFACE_STATS
#ifdef CONFIG_RF_RECEIVER
	uint16_t rx_packets;
	uint16_t rx_errors;
	uint16_t rx_dropped;
#endif
#ifdef CONFIG_RF_SENDER
	uint16_t tx_packets;
	uint16_t tx_errors;
	uint16_t tx_dropped;
#endif
#endif
	ring_t *rx;
	ring_t *tx;
	/* interrupt handler's pkt ring */
	ring_t *pkt_pool;
} __PACKED__;
typedef struct iface iface_t;

/** Initialize an interface.
 *
 * @param[in]  iface    interface to initialize
 * @param[in]  type     type of interface (eg: IF_TYPE_ETHERNET)
 * @param[in]  pkt_pool driver's free packet pool
 * @param[in]  rx       inbound packet bucket
 * @param[in]  tx       outbound packet bucket
 * @param[in]  is_interrupt_driven  indicates if the driver handle
 *             packet reception/sending on hardware interrupt
 */
void if_init(iface_t *iface, uint8_t type, ring_t *pkt_pool, ring_t *rx,
	     ring_t *tx, uint8_t is_interrupt_driven);

/* functions supposed to be called from an interrupt handler */

/** Schedule packet reception
 *
 * @param[in]     iface   network interface
 * @param[inout]  pkt     packet to be scheduled
 *                        The packet is set to null.
 */
void if_schedule_receive(iface_t *iface, pkt_t **pkt);

/** Free transmitted packet
 *
 * @param[in]  pkt  packet to be freed asynchronously
 */
void if_schedule_tx_pkt_free(pkt_t **pkt);

/** Dump interface statistics
 *
 * @param[in]  iface  interface
 */
void if_dump_stats(const iface_t *iface);

#endif
