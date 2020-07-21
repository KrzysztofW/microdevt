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

#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>
#include "common.h"
#include "log.h"

#define ADC_EXTERNAL_AVCC 0
#define ADC_INTERNAL_AVCC 1

extern unsigned reference_voltage_mv;

/* Note that changing reference voltage may lead to incorrect
 * results. The user must discard first results after AREF change
 * as specified in chapter 24.6 p243 of Atmega328p datasheet.
 */
#define ADC_5000MV_REF_VOLTAGE 5000U
#define ADC_1100MV_REF_VOLTAGE 1100U

#define ADC_SET_PRESCALER_64() ADCSRA = (1 << ADPS2) | (1 << ADPS1)
#define ADC_SET_PRESCALER_128()					\
	ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))

#define ADC_SET_REF_VOLTAGE(__ref_voltage)		\
	ADMUX = 0;					\
	reference_voltage_mv = __ref_voltage

#define ADC_SET_REF_VOLTAGE_INTERNAL()			\
	ADMUX = (1 << REFS0) | (1 << REFS1);		\
	reference_voltage_mv = ADC_1100MV_REF_VOLTAGE

#define ADC_SET_REF_VOLTAGE_AVCC()			\
	ADMUX = 1 << REFS0;				\
	reference_voltage_mv = ADC_5000MV_REF_VOLTAGE

#define adc_enable() ADCSRA |= 1 << ADEN

static inline uint16_t adc_read(uint8_t channel)
{
	adc_enable();

	/* select ADC channel */
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

	/* single conversion mode */
	ADCSRA |= 1 << ADSC;

	/* wait until ADC conversion is complete */
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

#define adc_shutdown() ADCSRA &= ~(1 << ADEN)

static inline uint16_t adc_to_millivolt(uint32_t val)
{
	assert(reference_voltage_mv);
	return (val * reference_voltage_mv) / 1023;
}

static inline uint16_t adc_read_mv(uint8_t channel)
{
	return adc_to_millivolt(adc_read(channel));
}

#endif
