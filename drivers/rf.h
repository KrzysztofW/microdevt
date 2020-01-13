/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2020, Krzysztof Witek
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

#ifndef _RF_PIN_INTERRUPT_H_
#define _RF_PIN_INTERRUPT_H_

#include <sys/byte.h>
#include <sys/timer.h>
#include <net/if.h>

/* Timings for the PT2262/PT2272 RF protocol:
 * Reset:   8060us
 * Period:  1040us
 * Zero hi:  260us
 * One: hi:  780us
 */

#ifndef RF_PULSE_WIDTH
#define RF_PULSE_WIDTH 260 /* microseconds */
#endif
#ifndef RF_LOW_TICKS
#define RF_LOW_TICKS 1
#endif
#ifndef RF_HI_TICKS
#define RF_HI_TICKS  3
#endif
#ifndef RF_RESET_TICKS
#define RF_RESET_TICKS 31
#endif

/* Time after reception in which the peer transmitter is still transmitting
 * so the local transmission should be postponed.
*/
#ifndef RF_FINISH_DELAY
#define RF_FINISH_DELAY 300000
#endif

/* #define RF_DELAY_BETWEEN_SENDS RF_FINISH_DELAY */

#ifdef CONFIG_RF_SENDER
#if (defined(RF_FINISH_DELAY) && defined(CONFIG_RF_RECEIVER))	\
	|| defined(RF_DELAY_BETWEEN_SENDS)
#define RF_WAIT_BEFORE_SENDING
#endif
#endif

typedef struct rf_data {
	byte_t  byte;
	pkt_t  *pkt;
} rf_data_t;

typedef enum rf_flags {
	RF_FLAG_NONE,
	RF_FLAG_SENDING_HI = 1 << 0,
} rf_flags_t;

typedef struct rf_ctx {
	tim_t timer;
#ifdef CONFIG_RF_RECEIVER
	rf_data_t recv_data;
	uint8_t prev_val;
#endif
	uint8_t cnt;
#ifdef CONFIG_RF_SENDER
#ifdef RF_WAIT_BEFORE_SENDING
	tim_t wait_before_sending_timer;
#endif
	uint8_t flags;
	rf_data_t snd_data;
	buf_t snd_buf;
#endif
} rf_ctx_t;

static inline void rf_input(iface_t *iface) {}
void
rf_init(iface_t *iface, rf_ctx_t *ctx);
void rf_shutdown(const iface_t *iface);
int rf_output(iface_t *iface, pkt_t *pkt);
void rf_recv_pin_interrupt(iface_t *iface, uint8_t val);
void rf_snd(void *arg);

#endif
