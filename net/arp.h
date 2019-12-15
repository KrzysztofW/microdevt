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

#ifndef _ARP_H_
#define _ARP_H_

#include "config.h"

struct arp_hdr {
	uint16_t hrd;	/* format of hardware address */
	uint16_t proto;	/* format of protocol address */
	uint8_t  hln;	/* length of hardware address */
	uint8_t  pln;	/* length of protocol address */
	uint16_t op;	/* ARP opcode */
	/* uint8_t  sha[]; /\* sender hardware address *\/ */
	/* uint8_t  spa[]; /\* sender protocol address *\/ */
	/* uint8_t  tha[]; /\* target hardware address *\/ */
	/* uint8_t  tpa[]; /\* target protocol address *\/ */
	uint8_t data[];
} __PACKED__;

typedef struct arp_hdr arp_hdr_t;

typedef struct arp_entry {
	uint32_t ip;
	uint8_t mac[ETHER_ADDR_LEN];
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	iface_t *iface;
#endif
#ifdef CONFIG_ARP_EXPIRY
	uint8_t updated; /* used by arp_timer */
#endif
} arp_entry_t;

#ifndef CONFIG_ARP_TABLE_SIZE
#define CONFIG_ARP_TABLE_SIZE 2
#endif

typedef struct arp_entries {
	arp_entry_t entries[CONFIG_ARP_TABLE_SIZE];
	uint8_t pos;
} arp_entries_t;

#ifdef CONFIG_IPV6
typedef struct arp6_entry {
	uint8_t ip[IP6_ADDR_LEN];
	uint8_t mac[ETHER_ADDR_LEN];
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	iface_t *iface;
#endif
} arp_entry_t;

typedef struct arp6_entries {
	arp6_entry_t entries[CONFIG_ARP_TABLE_SIZE];
	uint8_t pos;
} arp6_entries_t;
#endif

extern uint8_t broadcast_mac[];

/** Arp input
 *
 * @param[in]  pkt    packet
 * @param[in]  iface  interface
 */
void arp_input(pkt_t *pkt, const iface_t *iface);

int arp_find_entry(const uint32_t *ip, const uint8_t **mac,
		   const iface_t **iface);
int arp_output(const iface_t *iface, int op, const uint8_t *tha,
	       const uint8_t *tpa);
void
arp_add_entry(const uint8_t *sha, const uint8_t *spa, const iface_t *iface);
#ifdef CONFIG_IPV6
static void arp6_add_entry(uint8_t *sha, uint8_t *spa, const iface_t *iface);
#endif

void arp_resolve(pkt_t *pkt, const uint32_t *ip_dst, const iface_t *iface);

#ifdef TEST
arp_entries_t *arp_get_entries(void);
#endif

#endif
