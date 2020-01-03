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

#ifndef _RF_H_
#define _RF_H_
#include <sys/ring.h>
#include <sys/byte.h>
#include <sys/timer.h>
#include <net/if.h>

typedef struct rf_data {
	byte_t  byte;
	pkt_t  *pkt;
	buf_t   buf;
} rf_data_t;

typedef struct rf_ctx {
	tim_t timer;
#ifdef CONFIG_RF_RECEIVER
	rf_data_t rcv_data;
	struct {
		uint8_t cnt;
		uint8_t prev_val;
		volatile uint8_t receiving;
	} rcv;
#endif
#ifdef CONFIG_RF_SENDER
	rf_data_t snd_data;
#ifdef CONFIG_RF_BURST
	uint8_t   burst;
#endif
	struct {
		uint8_t  frame_pos;
		uint8_t  bit;
		uint8_t  clk;
#ifdef CONFIG_RF_BURST
		uint8_t  burst_cnt;
#endif
	} snd;
#endif
} rf_ctx_t;

void rf_init(iface_t *iface, rf_ctx_t *ctx, uint8_t burst);
void rf_shutdown(const iface_t *iface);
 void rf_input(iface_t *iface);
int rf_output(iface_t *iface, pkt_t *pkt);
int rf_checks(iface_t *iface);
#endif
