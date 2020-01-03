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

#include <stdlib.h>
#include <avr/interrupt.h>
#include <sys/timer.h>
#include "log.h"
#include "common.h"
#include "timer.h"
#include "utils.h"

/* 16-bit timer */
#ifdef ATTINY85
ISR(TIMER0_COMPA_vect)
{
	timer_process();
}
#else
ISR(TIMER1_COMPA_vect)
{
	timer_process();
}
#endif

void __timer_subsystem_init(void)
{
	STATIC_ASSERT(TIM_COUNTER8 <= 255);
	STATIC_ASSERT(TIM_COUNTER16 <= 65535);

#ifdef ATTINY85
	/* 8-bit timer */
	TCNT0 = 0;
	/* CTC mode with 64 prescaler */
	TCCR0A = 1 << WGM01;
	TCCR0B = (1 << CS00) | (1 << CS01);
	OCR0A = TIM_COUNTER8;
	/* enable compare and match interrupt */
	TIMSK |= 1 << OCIE0A;
#else
	/* 16-bit timer */
	TCNT1 = 0;
	TCCR1A = 0;
	/* CTC mode with 8 prescaler */
	TCCR1B = (1 << WGM12) | (1 << CS11);
	OCR1A = TIM_COUNTER16;
	/* enable compare and match interrupt */
	TIMSK1 |= 1 << OCIE1A;
#endif
}
