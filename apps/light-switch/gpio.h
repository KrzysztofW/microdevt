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

#ifndef _GPIO_H_
#define _GPIO_H_

/* output pins */
#define SWITCH_PIN PB0

#define SWITCH_PORT PORTB
#define SWITCH_DDR  DDRB

static inline void gpio_init(void)
{
	/* set output pins */
	SWITCH_DDR |= 1 << SWITCH_PIN;

	/* enable pull-up resistors on unconnected pins */
	PORTB = 0xFF & ~((1 << SWITCH_PIN));
}

/* SWITCH */
static inline void gpio_switch_on(void)
{
	SWITCH_PORT |= 1 << SWITCH_PIN;
}

static inline void gpio_switch_off(void)
{
	SWITCH_PORT &= ~(1 << SWITCH_PIN);
}

static inline void gpio_switch_toggle(void)
{
	SWITCH_PORT ^= 1 << SWITCH_PIN;
}

static inline uint8_t gpio_is_switch_on(void)
{
	return !!(SWITCH_PORT & (1 << SWITCH_PIN));
}

#endif
