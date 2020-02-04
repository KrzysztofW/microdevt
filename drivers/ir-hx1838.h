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

#define IR_TICK_DURATION_US 1200
#if IR_TICK_DURATION_US < CONFIG_TIMER_RESOLUTION_US
#error "Timer resolution too low. Increase CONFIG_TIMER_RESOLUTION_US option."
#endif
#define IR_TICK (IR_TICK_DURATION_US / CONFIG_TIMER_RESOLUTION_US)

/** This function has to be called from the interrupt service routine
 * on the falling edge of the pin.
 * Eg. on atmega328p using :
 *
 * ISR(INT0_vect)
 * {
 *	ir_falling_edge_interrupt_cb();
 * }
 *
 */
void ir_falling_edge_interrupt_cb(void);
void ir_init(void (*cb)(uint8_t));
