/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2024, Krzysztof Witek
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

static inline void gpio_init(void)
{
	/* PD2 output
	   PD5 input
	*/
	DDRD = (0xFF & ~(1 << PD5)) | (1 << PD2);

	/* PORTD used by RF receiver (PD5) */
	PORTD = 0xFF & ~(1 << PD5);
}

static inline void gpio_led_toggle(void)
{
	PORTD ^= 1 << PD2;
}
static inline void gpio_led_on(void)
{
	PORTD |= 1 << PD2;
}
static inline void gpio_led_off(void)
{
	PORTD &= ~(1 << PD2);
}
static inline uint8_t gpio_led_is_set(void)
{
	return !!(PORTD & (1 << PD2));
}
#endif
