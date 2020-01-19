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
#define LED PB7
#define LED_PORT PORTB
#define LED_DDR  DDRB

static inline void gpio_init(void)
{
	/* set output pins */
	LED_DDR |= 1 << LED;
	/* PORTF used by RF sender (PF1) & RF receiver (PF2) */
	DDRF = (1 << PF1) & ~(1 << PF2);

	/* enable pull-up resistors on unconnected pins */
	PORTB = 0xFF & ~(1 << LED);
	PORTD = 0xFF;
	PORTF = 0xFF & ~((1 << PF1) | (1 << PF0));
}

/* LED */
static inline void gpio_led_on(void)
{
	LED_PORT |= 1 << LED;
}

static inline void gpio_led_off(void)
{
	LED_PORT &= ~(1 << LED);
}
static inline void gpio_led_toggle(void)
{
	LED_PORT ^= 1 << LED;
}
#endif
