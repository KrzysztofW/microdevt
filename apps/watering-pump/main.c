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
#include <watchdog.h>
#include <_stdio.h>
#include <common.h>
#include <interrupts.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/utils.h>

#include "gpio.h"

#define PUMP_ON_DURATION  5000000UL
#define CORRECTION_PER_HOUR_DELAY 68 /* seconds */
#define PUMP_OFF_DURATION ((3600 - CORRECTION_PER_HOUR_DELAY) * 1000000UL)
#define PUMP_IDLE_TIME 24 /* hours */

static int counter = 1;
static tim_t pump_timer  = TIMER_INIT(pump_timer);
static tim_t led_timer = TIMER_INIT(led_timer);

static void pump_start_cb(void *arg);

static void pump_stop_cb(void *arg)
{
	gpio_turn_pump_off();
	timer_add(&pump_timer, PUMP_OFF_DURATION - PUMP_ON_DURATION,
		  pump_start_cb, NULL);
}

static void pump_start_cb(void *arg)
{
	if (--counter <= 0) {
		timer_add(&pump_timer, PUMP_ON_DURATION, pump_stop_cb, NULL);
		counter = PUMP_IDLE_TIME;
		gpio_turn_pump_on();
		return;
	}
	timer_add(&pump_timer, PUMP_OFF_DURATION, pump_start_cb, NULL);
}

static void tim_cb(void *arg)
{
	timer_reschedule(&led_timer, 1000000);
	gpio_led_toggle();
}

int main(void)
{
	watchdog_shutdown();
	gpio_init();
	timer_subsystem_init();

	STATIC_ASSERT(PUMP_OFF_DURATION > PUMP_ON_DURATION);
	timer_add(&pump_timer, 0, pump_start_cb, NULL);
	timer_add(&led_timer, 0, tim_cb, NULL);

	irq_enable();
	watchdog_enable(WATCHDOG_TIMEOUT_4S);

	while (1) {
		scheduler_run_task();
		watchdog_reset();
	}
	return 0;
}
