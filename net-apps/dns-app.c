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

#include <stdio.h>
#include <log.h>
#include "dns-app.h"
#include "../net/dns.h"
#include "../sys/buf.h"

static void dns_cb(uint32_t ip)
{
	DEBUG_LOG("%s: ip=0x%X\n", __func__, ip);
}

int dns_resolver_init(void)
{
	sbuf_t sb = SBUF_INITS("free.fr");

	/* set google dns nameserver */
	dns_init(0x08080808);

	return dns_resolve(&sb, dns_cb);
}
