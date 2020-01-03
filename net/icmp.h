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

#ifndef _ICMP_H_
#define _ICMP_H_

#include "config.h"

struct icmp {
	uint8_t   type;  /* type of message */
	uint8_t   code;  /* type sub code */
	uint16_t  cksum; /* ones complement cksum of struct */
	uint16_t  id;
	uint16_t  seq;
	uint8_t	  id_data[];
} __PACKED__;

typedef struct icmp icmp_hdr_t;

#define MAX_ICMP_DATA_SIZE (int)(CONFIG_PKT_SIZE - sizeof(eth_hdr_t) \
				 - sizeof(ip_hdr_t) - sizeof(icmp_hdr_t))

void icmp_input(pkt_t *pkt, iface_t *iface);
int
icmp_output(pkt_t *out, iface_t *iface, int type, int code,
	    uint16_t id, uint16_t seq, const buf_t *id_data, uint16_t ip_flags);

#endif
