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
#define LED PB7
#define LED_PORT PORTB
#define LED_DDR  DDRB

#define SIREN PB0
#define SIREN_PORT PORTB
#define SIREN_DDR  DDRB

#define TEMPERATURE_ANALOG_PIN 3

/* input pins */
#define PIR PC2
#define PIR_PORT PORTC
#define PIR_IN_PORT PINC
#define PIR_DDR DDRC

#define PWR_SENSOR PC1
#define PWR_SENSOR_PORT PORTC
#define PWR_SENSOR_IN_PORT PINC
#define PWR_SENSOR_DDR DDRC

static inline void gpio_init(void)
{
	/* set output pins */
	LED_DDR |= 1 << LED;
	SIREN_DDR |= 1 << SIREN;

	/* set input pins */
	PWR_SENSOR_DDR &= ~(1 << PWR_SENSOR);
	PIR_DDR &= ~(1 << PIR);

	/* enable pull-up resistors on unconnected pins */
	LED_PORT = 0xFF & ~(1 << LED);
	SIREN_PORT = 0xFF & ~(1 << SIREN);

	/* PORTC used by RF receiver (PC0) */
	/* analog: PC3 - temp sensor */
	PORTC = 0xFF & ~((1 << PC0) | (1 << PC2) | (1 << PC1) | (1 << PC3));

	/* PORTF used by RF sender (PF1) */
	PORTF = 0xFF & ~(1 << PF1);
	DDRF = 1 << PF1;

	/* analog input pins */
	ADC_SET_PRESCALER_64();
	ADC_SET_REF_VOLTAGE_INTERNAL();
	DDRC = 0xFF & ~((1 << PC0) | (1 << PC1) | (1 << PC3));

#ifndef CONFIG_AVR_SIMU
	/* PCINT18 enabled (PIR) */
	PCMSK1 = 1 << PCINT10;

	/* PCINT0 enabled (PWR_SENSOR) */
	PCMSK0 = 1 << PCINT0;

	PCICR = (1 << PCIE1) | (1 << PCIE0);
#endif
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

/* SIREN */
static inline void gpio_siren_on(void)
{
	SIREN_PORT |= 1 << SIREN;
}

static inline void gpio_siren_off(void)
{
	SIREN_PORT &= ~(1 << SIREN);
}

static inline uint8_t gpio_is_siren_on(void)
{
	return !!(SIREN_PORT & (1 << SIREN));
}

/* PWR sensor */
static inline uint8_t gpio_is_main_pwr_on(void)
{
	/*
	 * The pin value must be inverted as the used NPN transistor
	 * gives 1 => power disconnected, 0 => power connected
	 */
	return !(PWR_SENSOR_IN_PORT & (1 << PWR_SENSOR));
}

/* PIR sensor */
static inline uint8_t gpio_is_pir_on(void)
{
	return !!(PIR_IN_PORT & (1 << PIR));
}

#endif
