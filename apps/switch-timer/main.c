/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2021, Krzysztof Witek
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

#define ONE_SECOND 1000000
#define WD_DELAY 8 /* seconds */
#define DELAY_ON ((3600 / WD_DELAY) * 5) /* 5 hours */

/* #define DELAY_ON (16 / WD_DELAY) */

static uint16_t counter = DELAY_ON;

#ifdef CONFIG_POWER_MANAGEMENT
static void watchdog_on_wakeup(void *arg)
{
	gpio_led_toggle();

	if (!gpio_is_output_on() || --counter > 0)
		return;

	counter = DELAY_ON;
	gpio_output_off();
}

static void pwr_mgr_on_sleep(void *arg)
{
	/* use the watchdog interrupt to wake the uC up */
	watchdog_enable_interrupt(watchdog_on_wakeup, arg);
}
#endif

static uint8_t switch_prev_val;
ISR(PCINT0_vect)
{
	uint8_t switch_val = gpio_is_switch_on();

	if (switch_val != switch_prev_val) {
		gpio_output_toggle();
		switch_prev_val = switch_val;
		counter = DELAY_ON;
	}
}

int main(void)
{
	timer_subsystem_init();
	irq_enable();
	gpio_init();

#ifdef CONFIG_POWER_MANAGEMENT
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
	power_management_power_down_init(0, pwr_mgr_on_sleep, NULL);
#endif
	gpio_output_off();

	/* interruptible functions */
	while (1) {
		scheduler_run_task();
		watchdog_reset();
	}
	return 0;
}
