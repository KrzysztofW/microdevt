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

#include <sys/timer.h>
#include <sys/byte.h>
#include "ir-hx1838.h"

static uint32_t ticks;
static byte_t byte;
static uint8_t byte_pos;
static uint8_t prev_byte;
static void (*ir_cb)(uint8_t cmd);

void ir_falling_edge_interrupt_cb(void)
{
	int b;
	uint32_t tick_diff;

	/* protection against wrapping ticks */
	if (ticks > timer_ticks)
		tick_diff = UINT32_MAX - ticks + timer_ticks;
	else
		tick_diff = timer_ticks - ticks;
	ticks = timer_ticks;

	if (tick_diff > IR_TICK * 3)
		goto error;

	if ((b = byte_add_bit(&byte, tick_diff > IR_TICK + 1)) < 0)
		return;

	byte_pos++;
	/* 1st byte: 0x00
	 * 2nd byte: inv 1st (0xFF)
	 * 3rd byte: data
	 * 4th byte: inv 3rd byte
	 */
	if (byte_pos == 1 && b == 0)
		return;
	if (byte_pos == 2 && b == 0xFF)
		return;
	if (byte_pos == 3) {
		prev_byte = b;
		return;
	}
	if (byte_pos == 4 && (uint8_t)~prev_byte == b) {
		(*ir_cb)(prev_byte);
		return;
	}

 error:
	byte_reset(&byte);
	byte_pos = 0;
}

void ir_init(void (*cb)(uint8_t))
{
	ticks = timer_ticks;
	ir_cb = cb;
}
