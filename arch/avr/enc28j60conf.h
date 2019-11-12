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

#ifndef _ENC28J60CONF_H_
#define _ENC28J60CONF_H_

/* ENC28J60 SPI port */
#define ENC28J60_SPI_PORT	PORTB
#define ENC28J60_SPI_DDR	DDRB
#define ENC28J60_SPI_SCK	5
#define ENC28J60_SPI_MOSI	3
#define ENC28J60_SPI_MISO	4
#define ENC28J60_SPI_SS		2

/* ENC28J60 control port */
#define ENC28J60_CONTROL_PORT	PORTB
#define ENC28J60_CONTROL_DDR	DDRB
#define ENC28J60_CONTROL_CS	2

/* MAC address for this interface */
#ifdef ETHADDR0
#define ENC28J60_MAC0 ETHADDR0
#define ENC28J60_MAC1 ETHADDR1
#define ENC28J60_MAC2 ETHADDR2
#define ENC28J60_MAC3 ETHADDR3
#define ENC28J60_MAC4 ETHADDR4
#define ENC28J60_MAC5 ETHADDR5
#else
#define ENC28J60_MAC0 'A'
#define ENC28J60_MAC1 'B'
#define ENC28J60_MAC2 'C'
#define ENC28J60_MAC3 'D'
#define ENC28J60_MAC4 'E'
#define ENC28J60_MAC5 'F'
#endif

#endif
