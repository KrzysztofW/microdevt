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

#include <log.h>
#include <_stdio.h>
#include <watchdog.h>
#include <sys/buf.h>
#include <sys/ring.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <interrupts.h>
#include <drivers/sensors.h>
#include <power-management.h>
#include "gpio.h"

#define WD_DELAY 8 /* seconds */
//#define DELAY_ON ((3600 / WD_DELAY) / 2) /* 30 minutes */
//#define DELAY_OFF ((3600 / WD_DELAY) * 24 - DELAY_ON) /* 24 hours */

#define LIGHT_ON_DURATION (1800 * 1000000UL)
#define CORRECTION_PER_HOUR_DELAY 68 /* seconds */
#define LIGHT_OFF_DURATION ((3600 - CORRECTION_PER_HOUR_DELAY) * 1000000UL)
#define LIGHT_IDLE_TIME 24 /* hours */

static tim_t light_timer  = TIMER_INIT(light_timer);
static uint16_t counter = 12;

static void light_start_cb(void *arg);

static void light_stop_cb(void *arg)
{
	gpio_switch_off();
	timer_add(&light_timer, LIGHT_OFF_DURATION - LIGHT_ON_DURATION,
		  light_start_cb, NULL);
}

static void light_start_cb(void *arg)
{
	if (--counter <= 0) {
		timer_add(&light_timer, LIGHT_ON_DURATION, light_stop_cb, NULL);
		counter = LIGHT_IDLE_TIME;
		gpio_switch_on();
		return;
	}
	timer_add(&light_timer, LIGHT_OFF_DURATION, light_start_cb, NULL);
}

int main(void)
{
	watchdog_shutdown();
	gpio_init();
	timer_subsystem_init();

	STATIC_ASSERT(LIGHT_OFF_DURATION > LIGHT_ON_DURATION);
	timer_add(&light_timer, 0, light_start_cb, NULL);

	irq_enable();
	watchdog_enable(WATCHDOG_TIMEOUT_4S);

	while (1) {
		scheduler_run_task();
		watchdog_reset();
	}
	return 0;
}
