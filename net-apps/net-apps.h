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

#ifndef _NET_APPS_H_
#define _NET_APPS_H_
#include "../net/socket.h"
#include "../net/event.h"

#ifdef CONFIG_AVR_MCU
#include <avr/io.h>
#include <avr/pgmspace.h>
#endif

int tcp_app_init(void);
int tcp_server(void);
void tcp_app(void);
int udp_server(void);
#ifdef CONFIG_BSD_COMPAT
int udp_client(struct sockaddr_in *sockaddr);
#endif
int udp_app_init(void);
void udp_app(void);

int dns_resolver_init(void);

#endif
