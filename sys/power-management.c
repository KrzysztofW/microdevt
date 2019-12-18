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

#include "power-management.h"
#include "timer.h"
#include "scheduler.h"

static tim_t timer = TIMER_INIT(timer);
static uint16_t pwr_mgr_sleep_timeout;
static void (*on_sleep_cb)(void *arg);
static void *on_sleep_arg;
uint16_t power_management_inactivity;

#define power_management_pwr_down_set_idle() power_management_inactivity++
#define ONE_SECOND 1000000

static void enter_power_down_mode_cb(void *arg)
{
	if (on_sleep_cb)
		on_sleep_cb(on_sleep_arg);
	power_management_set_mode(PWR_MGR_SLEEP_MODE_PWR_DOWN);
	power_management_disable_bod();
	power_management_sleep();
}

static void timer_cb(void *arg)
{
	timer_reschedule(&timer, ONE_SECOND);
	power_management_pwr_down_set_idle();

	if (power_management_inactivity < pwr_mgr_sleep_timeout)
		return;
	/* interrupts must be enabled when entering sleep mode */
	schedule_task(enter_power_down_mode_cb, NULL);
}

void power_management_power_down_disable(void)
{
	timer_del(&timer);
}

void power_management_power_down_enable(void)
{
	timer_add(&timer, ONE_SECOND, timer_cb, NULL);
}

void
power_management_power_down_init(uint16_t inactivity_timeout,
				 void (*on_sleep)(void *arg), void *arg)
{
	pwr_mgr_sleep_timeout = inactivity_timeout;
	on_sleep_cb = on_sleep;
	on_sleep_arg = arg;
	power_management_power_down_enable();
}
