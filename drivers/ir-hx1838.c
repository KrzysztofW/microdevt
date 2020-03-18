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
#include <sys/scheduler.h>
#include "ir-hx1838.h"

static byte_t byte;
static uint8_t byte_pos;
static uint8_t prev_byte;
static uint32_t ticks;
static void (*ir_cb)(uint8_t cmd, uint8_t is_repeated);

static void ir_task_cb(void *arg)
{
	uint16_t cmd = (uint16_t)(uintptr_t)arg;

	(*ir_cb)((uint8_t)cmd, cmd >> 15);
}

static uint32_t get_timer_tick_diff(uint32_t t)
{
	/* protection against wrapping ticks */
	if (t > timer_ticks)
		return UINT32_MAX - t + timer_ticks;
	return timer_ticks - t;
}

void ir_falling_edge_interrupt_cb(void)
{
	int b;
	uint32_t tick_diff;
	static uint32_t last_byte_ticks;
	static uint8_t repeat_cnt;

	tick_diff = get_timer_tick_diff(ticks);
	ticks = timer_ticks;

	if (tick_diff > 3 * IR_TICK) {
		uint16_t cmd;

		tick_diff = get_timer_tick_diff(last_byte_ticks);

		if (tick_diff > 81 * IR_TICK * 2)
			goto error;

		/* Repeat the last command when receiving bursts
		 * (button not released).
		 */
		repeat_cnt++;
		last_byte_ticks = ticks;

		/* Do not repeat the command immediately to avoid
		 * unexpected duplicates.
		 */
		if (repeat_cnt < 4)
			return;

		cmd = 0x8000 | prev_byte;
		schedule_task(ir_task_cb, (void *)(uintptr_t)cmd);
		return;
	}
	if ((b = byte_add_bit(&byte, tick_diff > IR_TICK)) < 0)
		return;

	byte_pos++;
	/* 1st byte: 0x00
	 * 2nd byte: inv 1st (0xFF)
	 * 3rd byte: data
	 * 4th byte: inv 3rd byte
	 */
	if ((byte_pos == 1 && b == 0) ||
	    (byte_pos == 2 && b == 0xFF))
		return;
	if (byte_pos == 3) {
		prev_byte = b;
		return;
	}
	if (byte_pos == 4 && (uint8_t)~prev_byte == b) {
		last_byte_ticks = ticks;
		repeat_cnt = 0;
		schedule_task(ir_task_cb, (void *)(uintptr_t)prev_byte);
		return;
	}

 error:
	byte_reset(&byte);
	byte_pos = 0;
}

void ir_init(void (*cb)(uint8_t, uint8_t))
{
	ticks = timer_ticks;
	ir_cb = cb;
}
