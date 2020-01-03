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

#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"

static inline void __eth_input(pkt_t *pkt, iface_t *iface)
{
	eth_hdr_t *eh;

	if ((iface->flags & IF_UP) == 0)
		goto unsupported;

	eh = btod(pkt);
#ifdef CONFIG_PROMISC
	if (iface->flags & IFF_PROMISC) {
		/* check mac address */
	}
#endif
	pkt_adj(pkt, sizeof(eth_hdr_t));
	switch (eh->type) {
	case ETHERTYPE_ARP:
		arp_input(pkt, iface);
		return;

	case ETHERTYPE_IP:
		ip_input(pkt, iface);
		return;
#ifdef CONFIG_IPV6
	case ETHERTYPE_IPV6:
		ip6_input(pkt, iface);
		return;
#endif
	default:
		/* unsupported ethertype */
		break;
	}
 unsupported:
	pkt_free(pkt);
}

void eth_input(iface_t *iface)
{
	pkt_t *pkt;

	while ((pkt = pkt_get(iface->rx)))
		__eth_input(pkt, iface);
}

int
eth_output(pkt_t *out, iface_t *iface, uint8_t type, const void *dst)
{
	eth_hdr_t *eh;
	int i;
	const uint8_t *mac_dst;
	uint16_t l3_proto;

	if ((iface->flags & IF_UP) == 0)
		goto end;

	switch (type) {
	case L3_PROTO_IP:
		if (arp_find_entry(dst, &mac_dst, &iface) < 0) {
			arp_resolve(out, dst, iface);
			return 0;
		}
		l3_proto = ETHERTYPE_IP;
		break;
	case L3_PROTO_ARP:
		mac_dst = dst;
		l3_proto = ETHERTYPE_ARP;
		break;
	default:
		/* unsupported */
		goto end;
	}

	pkt_adj(out, -(int)sizeof(eth_hdr_t));
	eh = btod(out);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		eh->dst[i] = mac_dst[i];
		eh->src[i] = iface->hw_addr[i];
	}
	eh->type = l3_proto;
	return iface->send(iface, out);
 end:
	pkt_free(out);
	/* TODO update iface stats */
	return -1;
}
