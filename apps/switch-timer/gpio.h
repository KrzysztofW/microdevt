/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2021, Krzysztof Witek
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
#define OUTPUT_PIN PB4
#define OUTPUT_PORT PORTB
#define OUTPUT_DDR DDRB

#define LED_PIN PB2
#define LED_PORT PORTB
#define LED_DDR DDRB

/* input PINs */
#define SWITCH_PIN PB1
#define SWITCH_PORT PORTB
#define SWITCH_IN_PORT PINB
#define SWITCH_DDR DDRB

static inline void gpio_init(void)
{
	/* set output pins */
	OUTPUT_DDR |= (1 << OUTPUT_PIN) | (1 << LED_PIN);

	/* set input pins */
	SWITCH_DDR &= ~(1 << SWITCH_PIN);

	/* turns on PIN change interrupt */
	GIMSK |= (1 << PCIE);
	PCMSK |= (1 << PCINT1);

	/* enable pull-up resistors on unconnected pins */
	PORTB = 0xFF & ~((1 << OUTPUT_PIN)| (1 << SWITCH_PIN) | (1 << LED_PIN));
}

/* OUTPUT */
static inline void gpio_output_on(void)
{
	OUTPUT_PORT |= 1 << OUTPUT_PIN;
}

static inline void gpio_output_off(void)
{
	OUTPUT_PORT &= ~(1 << OUTPUT_PIN);
}

static inline void gpio_output_toggle(void)
{
	OUTPUT_PORT ^= 1 << OUTPUT_PIN;
}

static inline uint8_t gpio_is_output_on(void)
{
	return !!(OUTPUT_PORT & (1 << OUTPUT_PIN));
}

/* system LED */
static inline void gpio_led_on(void)
{
	LED_PORT |= 1 << LED_PIN;
}

static inline void gpio_led_off(void)
{
	LED_PORT &= ~(1 << LED_PIN);
}

static inline void gpio_led_toggle(void)
{
	LED_PORT ^= 1 << LED_PIN;
}

static inline uint8_t gpio_is_led_on(void)
{
	return !!(LED_PORT & (1 << LED_PIN));
}

static inline uint8_t gpio_is_switch_on(void)
{
	return !!(SWITCH_IN_PORT & (1 << SWITCH_PIN));
}

#endif
