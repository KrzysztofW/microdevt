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

#ifndef _IP_H_
#define _IP_H_

#include "config.h"

struct ip_hdr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t hl:4;		/* header length */
	uint8_t  v:4;		/* version */
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t   v:4;		/* version */
	uint8_t  hl:4;		/* header length */
#endif
	uint8_t  tos;		/* type of service */
	uint16_t len;		/* total length */
	uint16_t id;		/* identification */
	uint16_t off;		/* fragment offset field */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define	IP_RF 0x8000		/* reserved fragment flag */
#define	IP_EF 0x8000		/* evil flag, per RFC 3514 */
#define	IP_DF 0x4000		/* dont fragment flag */
#define	IP_MF 0x2000		/* more fragments flag */
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define	IP_RF 0x80		/* reserved fragment flag */
#define	IP_EF 0x80		/* evil flag, per RFC 3514 */
#define	IP_DF 0x40		/* dont fragment flag */
#define	IP_MF 0x20		/* more fragments flag */
#endif
#define	IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	uint8_t  ttl;		/* time to live */
	uint8_t  p;		/* protocol */
	uint16_t chksum;	/* checksum */
	uint32_t src, dst;	/* source and dest address */
} __attribute__((__packed__));
typedef struct ip_hdr ip_hdr_t;

#define IP_MIN_HDR_LEN 5
#define IP_MAX_HDR_LEN 15

#ifndef CONFIG_IP_TTL
#define CONFIG_IP_TTL 0x38
#endif

void ip_input(pkt_t *pkt, const iface_t *iface);
int ip_output(pkt_t *out, const iface_t *iface, uint16_t flags);

#endif
