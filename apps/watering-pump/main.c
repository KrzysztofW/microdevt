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

#define PUMP_ON_DURATION  10000000
#define PUMP_OFF_DURATION (3600*1000000UL)
#define PUMP_IDLE_TIME 24

static int counter = 1;
static tim_t pump_stop_timer  = TIMER_INIT(pump_stop_timer);
static tim_t pump_start_timer = TIMER_INIT(pump_start_timer);
static tim_t timer = TIMER_INIT(timer);

static void pump_stop_cb(void *arg)
{
	gpio_turn_pump_off();
}

static void pump_start_cb(void *arg)
{
	timer_reschedule(&pump_start_timer, PUMP_OFF_DURATION);

	if (--counter <= 0) {
		counter = PUMP_IDLE_TIME;
		timer_add(&pump_stop_timer, PUMP_ON_DURATION, pump_stop_cb, NULL);
		gpio_turn_pump_on();
	}
}

static void tim_cb(void *arg)
{
	timer_reschedule(&timer, 1000000);
	gpio_led_toggle();
	watchdog_reset();
}

int main(void)
{
	watchdog_shutdown();
	gpio_init();
	timer_subsystem_init();

	STATIC_ASSERT(PUMP_OFF_DURATION > PUMP_ON_DURATION);
	timer_add(&pump_start_timer, 0, pump_start_cb, NULL);
	timer_add(&timer, 0, tim_cb, NULL);

	irq_enable();
	watchdog_enable(WATCHDOG_TIMEOUT_4S);

	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
