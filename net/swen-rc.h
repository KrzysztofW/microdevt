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

#ifndef _SWEN_RC_H_
#define _SWEN_RC_H_

#include <crypto/xtea.h>
#include "pkt-mempool.h"
#include "if.h"

#define SWEN_RC_WINDOW 1024
typedef struct swen_rc_ctx {
	uint32_t *local_counter;
	uint32_t *remote_counter;
	const uint32_t *key;
	uint8_t dst;
	const iface_t *iface;
	void (*set_rc_cnt)(uint32_t *counter, uint8_t value);
	list_t list;
} swen_rc_ctx_t;

int swen_rc_sendto(swen_rc_ctx_t *ctx, const sbuf_t *sbuf);
void swen_rc_init(swen_rc_ctx_t *ctx, const iface_t *iface, uint8_t to,
		  uint32_t *local_cnt, uint32_t *remote_cnt,
		  void (*set_rc_cnt)(uint32_t *counter, uint8_t value),
		  const uint32_t *key);
void swen_rc_input(uint8_t from, pkt_t *pkt, const iface_t *iface);

#endif
