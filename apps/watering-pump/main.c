/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2020, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 3, as published by the Free Software Foundation.
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

#include <avr/interrupt.h>
#include <adc.h>
#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <interrupts.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/utils.h>

#include "gpio.h"

/* ADC based on https://www.marcelpost.com/wiki/index.php/ATtiny85_ADC */
/* PWM based on http://www.technoblogy.com/show?LE0 */

#define PUMP_ON_DURATION  10000000
#define PUMP_OFF_DURATION (3600*1000000UL)
#define PUMP_IDLE_TIME 24
#define PWM_PUMP_DELAY 1200

static unsigned counter = PUMP_IDLE_TIME;
static uint8_t analog_val;
static tim_t pump_timer = TIMER_INIT(pump_timer);
static tim_t pwm_pump_timer = TIMER_INIT(pwm_pump_timer);

ISR(TIMER1_OVF_vect) {
	/* don't turn the pump off when the adjustable resistance
	 * is at its near minimum  */
	if (analog_val > 5)
		PORTB &= ~(1 << PB3);
}

ISR(TIMER1_COMPA_vect) {
	if (!((TIFR & (1 << TOV1))))
		PORTB |= (1 << PB3);
}

static void pump_start_cb(void *arg);

static void pump_stop_cb(void *arg)
{
	gpio_turn_pump_off();
	PORTB &= ~(1 << PB3);

	if (counter) {
		timer_add(&pump_timer, PUMP_OFF_DURATION, pump_stop_cb, NULL);
		counter--;
	} else {
		counter = PUMP_IDLE_TIME;
		timer_add(&pump_timer, 0, pump_start_cb, NULL);
	}
}

static void pump_start_cb(void *arg)
{
	gpio_turn_pump_on();
	timer_add(&pump_timer, PUMP_ON_DURATION, pump_stop_cb, NULL);
}

static void pwm_pump_cb(void *arg)
{
	if (PORTB | (1 << PB3)) {
		analog_val = adc_read8(ADJ_RESISTOR_PIN);
		OCR1A = analog_val;
	}
	timer_reschedule(&pwm_pump_timer, PWM_PUMP_DELAY);
}

static void pwm_pump_tim_cb(void *arg)
{
	schedule_task(pwm_pump_cb, NULL);
}

int main(void)
{
	gpio_init();
	ADC_SET_PRESCALER_64();
	timer_subsystem_init();

	timer_add(&pump_timer, 0, pump_start_cb, NULL);
	timer_add(&pwm_pump_timer, 0, pwm_pump_tim_cb, NULL);

	irq_enable();

	while (1) {
		scheduler_run_tasks();
	}
	return 0;
}
