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

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#ifdef DEBUG
void __schedule_task(void (*cb)(void *arg), void *arg,
		     const char *func, int line);
#define schedule_task(cb, arg) __schedule_task(cb, arg, __func__, __LINE__)
#else

/** Schedule task
 *
 * Scheduling tasks is safe from an interrupt handler and from an other task.
 * @param[in] cb  task function to be scheduled
 */
void schedule_task(void (*cb)(void *arg), void *arg);
#endif

/** Run first task in queue
 * This function should be called from the main loop of a user application
 */
void scheduler_run_task(void);


/** Run all tasks in loop
 *
 * This function should be used as a main loop in user application
 */
static inline void scheduler_run_tasks(void)
{
	for (;;)
		scheduler_run_task();
}

#endif
