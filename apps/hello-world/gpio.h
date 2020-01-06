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
#include <adc.h>

/* output pins */
#define LED PD7
#define LED_PORT PORTD
#define LED_DDR  DDRD

#define HUMIDITY_ANALOG_PIN 1
#define TEMPERATURE_ANALOG_PIN 3

/* input pins */
#define PIR PD2
#define PIR_PORT PORTD
#define PIR_IN_PORT PIND
#define PIR_DDR DDRD

static inline void pir_int_enable(void)
{
	/* PCINT18 enabled (PIR) */
	PCMSK2 = 1 << PCINT18;
}

static inline void pir_int_disable(void)
{
	/* PCINT18 disabled (PIR) */
	PCMSK2 = 0;
}

static inline void gpio_init(void)
{
	/* set output pins */
	LED_DDR |= 1 << LED;

	/* set input pins */
	PIR_DDR &= ~(1 << PIR);

	/* enable pull-up resistors on unconnected pins */
	PORTB = 0xFF;
	PORTD = 0xFF & ~((1 << LED) | (1 << PIR));

	/* analog: PC1 - HIH humidity sensor, PC3 - temperature sensor */
	PORTC = 0xFF & ~((1 << PC1) | (1 << PC3));

	/* analog input pins */
	ADC_SET_PRESCALER_64();
	ADC_SET_REF_VOLTAGE_AVCC();
	DDRC = 0xFF & ~((1 << PC1) | (1 << PC3));

	pir_int_enable();
	PCICR = (1 << PCIE2) | (1 << PCIE0);
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

/* PIR sensor */
static inline uint8_t gpio_is_pir_on(void)
{
	return !!(PIR_IN_PORT & (1 << PIR));
}

#endif
