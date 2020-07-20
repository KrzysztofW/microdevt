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

#ifndef _GPIO_H_
#define _GPIO_H_
#include <adc.h>

#define ADJ_RESISTOR_PIN 2

static void gpio_pwm_init(void)
{
	/* set as output */
	DDRB |= (1 << PB3);

	TCCR1 = 1 << CTC1 | 1 << PWM1A | 3 << COM1A0 | 7 << CS10;
	GTCCR = 1 << PWM1B | 3 << COM1B0;

	/* interrupts on OC1A match and overflow */
	TIMSK |= 1 << OCIE1A | 1 << TOIE1;
}

static void gpio_init(void)
{
	DDRB &= ~(1 << PB4); /* set as input */
	PORTB |= (1 << PB4); /* pullup resistors */
	gpio_pwm_init();
}

static void gpio_turn_pump_on(void)
{
	TCCR1 = 1 << CTC1 | 1 << PWM1A | 3 << COM1A0 | 7 << CS10;
}

static void gpio_turn_pump_off(void)
{
	TCCR1 = 0;
}

#endif
