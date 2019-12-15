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

#ifndef _ROUTE_H_
#define _ROUTE_H_

#include "proto-defs.h"
#include "config.h"

struct route {
	uint32_t ip;
	iface_t *iface;
} __PACKED__;
typedef struct route route_t;

extern route_t dft_route;

#ifdef CONFIG_IPV6
struct route6 {
	uint8_t ip[IP6_ADDR_LEN];
	/* iface_t *iface; */
} __PACKED__;
typedef struct route6 route6_t;

extern route6_t dft_route6;
#endif

#endif
