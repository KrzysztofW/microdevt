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

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/timer.h>

#include "timer.h"

uint8_t irq_lock;

static inline void process_timers(int signo)
{
	(void)signo;
	if (!irq_lock)
		timer_process();
}

void __timer_subsystem_init(void)
{
	struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &process_timers;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		fprintf(stderr, "\ncan't initialize timers (%m)\n");
		abort();
	}

	/* configure the timer to first expire after 3 seconds */
	timer.it_value.tv_sec = 3;
	timer.it_value.tv_usec = 0;

	if (CONFIG_TIMER_RESOLUTION_US > 1000000) {
		timer.it_interval.tv_sec = CONFIG_TIMER_RESOLUTION_US / 1000000;
		timer.it_interval.tv_usec = CONFIG_TIMER_RESOLUTION_US % 1000000;
	} else {
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = CONFIG_TIMER_RESOLUTION_US;
	}

	if (setitimer(ITIMER_REAL, &timer, NULL) < 0)
		fprintf(stderr, "\n can'\t set itimer (%m)\n");
}

void __timer_subsystem_stop(void)
{
	struct itimerval timer;

	memset(&timer, 0, sizeof(timer));
	if (setitimer(ITIMER_REAL, &timer, NULL) < 0)
		fprintf(stderr, "\n can'\t disable itimer (%m)\n");
}
