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

#ifndef _AVR_TIMER_H_
#define _AVR_TIMER_H_

#include <avr/interrupt.h>

/* Timer overflow:
 * 8-bit timer 300us resolution
 * 255-0.0003/(1/(16000000/64.)) = 180
 * where 255 is the 8-bit counter max value
 *       0.0003 = 300us
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       64 = prescaler value
 *
 * 16-bit timer 150us resolution
 * 65535-0.00015/(1/(16000000/8.)) = 65235
 * where 65535 is the 16-bit counter max value
 *       0.00015 = 150us
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       8 = prescaler value
 *       <=> (65535 - (150*(16000000/8))/1000000)
 *
 * Timer compare and match:
 * 16-bit timer 150us resolution
 * 0.00015/(1/(16000000/8.)) = 300
 *       0.00015 = 150us
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       8 = prescaler value
 *       <=> ((150*(16000000/8))/1000000)
 */

#define TIMER_DEVIDER 1000000

#define TIM_COUNTER16							\
	((CONFIG_TIMER_RESOLUTION_US*(CONFIG_AVR_F_CPU/8ULL))/TIMER_DEVIDER)
#define TIM_COUNTER8							\
	((CONFIG_TIMER_RESOLUTION_US*(CONFIG_AVR_F_CPU/64))/TIMER_DEVIDER)

void __timer_subsystem_init(void);


static inline void timer_interrupt_enable(void)
{
#ifdef ATTINY85
	TIMSK |= 1 << OCIE0A;
#else
	TIMSK1 |= 1 << OCIE1A;
#endif
}

static inline void __timer_subsystem_start(void)
{
#ifdef ATTINY85
	OCR0A = TIM_COUNTER8;
#else
	OCR1A = TIM_COUNTER16;
#endif
	timer_interrupt_enable();
}

static inline void timer_interrupt_disable(void)
{
#ifdef ATTINY85
	TIMSK &= ~(1 << OCIE0A);
#else
	TIMSK1 &= ~(1 << OCIE1A);
#endif
}

static inline void __timer_subsystem_stop(void)
{
	timer_interrupt_disable();
}

static inline void __timer_subsystem_reset(void)
{
#ifdef ATTINY85
	OCR0A = TIM_COUNTER8;
#else
	OCR1A = TIM_COUNTER16;
#endif
}

static inline uint8_t __timer_subsystem_is_runing(void)
{
#ifdef ATTINY85
	return TIMSK & (1 << OCIE0A);
#else
	return TIMSK1 & (1 << OCIE1A);
#endif
}

#endif
