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

#define FAN PD3
#define FAN_PORT PORTD
#define FAN_DDR  DDRD

#define SIREN PB1
#define SIREN_PORT PORTB
#define SIREN_DDR  DDRB

#define HUMIDITY_ANALOG_PIN 1
#define TEMPERATURE_ANALOG_PIN 3

/* input pins */
#define PIR PD2
#define PIR_PORT PORTD
#define PIR_IN_PORT PIND
#define PIR_DDR DDRD

#define PWR_SENSOR PB0
#define PWR_SENSOR_PORT PORTB
#define PWR_SENSOR_IN_PORT PINB
#define PWR_SENSOR_DDR DDRB

static inline void pir_int_enable(void)
{
#ifndef CONFIG_AVR_SIMU
	/* PCINT18 enabled (PIR) */
	PCMSK2 = 1 << PCINT18;
#endif
}

static inline void pir_int_disable(void)
{
	/* PCINT18 disabled (PIR) */
	PCMSK2 = 0;
}

static inline uint8_t pir_read_bit(void)
{
	return !!(PIR_IN_PORT & (1 << PIR));
}

static inline void gpio_init(void)
{
	/* set output pins */
	LED_DDR |= 1 << LED;
	SIREN_DDR |= 1 << SIREN;

	/* set input pins */
	PWR_SENSOR_DDR &= ~(1 << PWR_SENSOR);
	PIR_DDR &= ~(1 << PIR);

	/* enable pull-up resistors on unconnected pins */
	PORTB = 0xFF & ~((1 << SIREN) | (1 << PWR_SENSOR));
	PORTD = 0xFF & ~((1 << LED) | (1 << FAN) | (1 << PIR));

	/* PORTC used by RF sender (PC2) & receiver (PC0) */
	/* analog: PC1 - HIH, PC3 - temp sensor */
	PORTC = 0xFF & ~((1 << PC0) | (1 << PC2) | (1 << PC1) | (1 << PC3));

	/* input pins */
	ADC_SET_PRESCALER_64();
	ADC_SET_REF_VOLTAGE_INTERNAL();
	DDRC = 0xFF & ~((1 << PC0) | (1 << PC1) | (1 << PC3));

#ifndef CONFIG_AVR_SIMU
	pir_int_enable();

	/* PCINT0 enabled (PWR_SENSOR) */
	PCMSK0 = 1 << PCINT0;

	PCICR = (1 << PCIE2) | (1 << PCIE0);
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

/* FAN */
static inline void gpio_fan_on(void)
{
	FAN_PORT |= 1 << FAN;
}

static inline void gpio_fan_off(void)
{
	FAN_PORT &= ~(1 << FAN);
}

static inline uint8_t gpio_is_fan_on(void)
{
	return !!(FAN_PORT & (1 << FAN));
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
