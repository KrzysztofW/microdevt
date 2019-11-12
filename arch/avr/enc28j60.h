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

#ifndef _ENC28J60_H_
#define _ENC28J60_H_

#include <sys/buf.h>
#include <net/pkt-mempool.h>
#include <net/if.h>

/* buffer boundaries applied to internal 8K ram
 * entire available packet buffer space is allocated
 */
/* start TX buffer at 0 */
#define TXSTART_INIT	0x0000

/* give TX buffer space for one full ethernet frame (~1500 bytes) */
#define RXSTART_INIT	0x0600

/* receive buffer gets the rest */
#define RXSTOP_INIT	0x1FFF

/* maximum ethernet frame length */
#define	MAX_FRAMELEN	1518

/* Ethernet constants */
#define ETHERNET_MIN_PACKET_LENGTH	0x3C

uint8_t enc28j60_read_op(uint8_t op, uint8_t address);
void enc28j60_write_op(uint8_t op, uint8_t address, uint8_t data);
void enc28j60_read_buffer(uint16_t len, buf_t *buf);
void enc28j60_write_buffer(uint16_t len, uint8_t* data);
uint8_t enc28j60_read(uint8_t address);
void enc28j60_write(uint8_t address, uint8_t data);
uint16_t enc28j60_phy_read(uint8_t address);
void enc28j60_phy_write(uint8_t address, uint16_t data);

void enc28j60_init(uint8_t *macaddr);

/* This function is supposed to be called from BHs. */
/* \param iface		Interface on whitch to send (not supported for now). */
/* \param pkt		Pointer to the packet. */
/* \return		0 (generic return). */
int enc28j60_pkt_send(const iface_t *iface, pkt_t *pkt);

/* \param iface		Pointer to the interface. */
void enc28j60_pkt_recv(const iface_t *iface);

uint8_t enc28j60_get_interrupts(void);
void enc28j60_handle_interrupts(const iface_t *iface);

#endif
