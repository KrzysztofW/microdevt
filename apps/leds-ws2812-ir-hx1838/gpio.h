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
#include <ws2812_cfg.h>

#ifndef _GPIO_H_
#define _GPIO_H_

/* input pins */
#ifdef ATTINY85
#define IR_PIN PB2 /* ext. INT0 */
#define IR_DDR DDRB
#define IR_PORT PORTB
#else

#define LED_PIN PB5
#define LED_PORT PORTB
#define LED_DDR  DDRB

#define IR_PIN PD2 /* ext. INT0 */
#define IR_DDR DDRD
#define IR_PORT PORTD
#endif

static inline void gpio_init(void)
{
	/* set output pins */
	WS2812_DDR |= 1 << WS2812_PIN;
#ifndef ATTINY85
	LED_DDR |= 1 << LED_PIN;
#endif
	/* set input pins */
	IR_DDR &= ~(1 << IR_PIN);

	/* enable pull-up resistors on unconnected pins */
	IR_PORT &= ~(1 << IR_PIN);
	WS2812_PORT &= ~(1 << WS2812_PIN);

#ifdef ATTINY85
	/* set INT0 to trigger on falling edge */
	MCUCR |= (1 << ISC01);

	/* Turns on INT0 */
	GIMSK |= 1 << INT0;
#else
	EICRA |= (1 << ISC01);
	EIMSK |= 1 << INT0;
#endif
}

#ifndef ATTINY85
/* LED */
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
#endif

#endif
