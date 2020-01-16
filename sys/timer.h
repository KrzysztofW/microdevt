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

#ifndef _TIMER_H_
#define _TIMER_H_

/* #define DEBUG_TIMERS */

#include <timer.h>
#include <stdint.h>
#include "sys/list.h"

struct timer {
	struct list_head list;
	void (*cb)(void *);
	void *arg;
#ifdef DEBUG_TIMERS
	const char *func;
	unsigned line;
#endif
	uint32_t ticks;
} __PACKED__;
typedef struct timer tim_t;

/** Initialize a timer at compile time
 */
#define TIMER_INIT(__tim) { .list = LIST_HEAD_INIT((__tim).list) }

/** Initialize timer subsystem
 *
 * This function should be called from architecture dependant timer
 * interrupt handler.
 */
void timer_subsystem_init(void);

/** Stop architecture dependant timer interrupt
 */
static inline void timer_subsystem_stop(void)
{
	__timer_subsystem_stop();
}

static inline void timer_subsystem_reset(void)
{
	__timer_subsystem_reset();
}

/** Initialize timer
 *
 * @param[in] timer  timer
 */
void timer_init(tim_t *timer);

#ifdef DEBUG_TIMERS
void timer_dump(void);
void __timer_add(const char *func, unsigned line, tim_t *timer, uint32_t expiry,
		 void (*cb)(void *), void *arg);
#define timer_add(timer, expiry, cb, arg)			\
	__timer_add(__func__, __LINE__, timer, expiry, cb, arg)
void __timer_reschedule(const char *func, unsigned line, tim_t *timer,
			uint32_t expiry);
#define timer_reschedule(timer, expiry)				\
	__timer_reschedule(__func__, __LINE__, timer, expiry)
#else

/** Schedule timer
 *
 * @param[in] timer  timer
 * @param[in] expiry expiry in microseconds
 * @param[in] cb     callback function
 */
void timer_add(tim_t *timer, uint32_t expiry, void (*cb)(void *), void *arg);

/** Reschedule timer
 *
 * This function is not re-entrant, a timer cannot be added multiple times.
 * @param[in] timer  timer
 * @param[in] expiry expiry in microseconds
 */
void timer_reschedule(tim_t *timer, uint32_t expiry);

#endif

/** Remove timer
 *
 * This function is re-entrant, it is safe to remove a timer that is
 * not scheduled.
 * @param[in] timer  timer
 */
void timer_del(tim_t *timer);

/** Process scheduled timers
 *
 * This function should only be used in architecture dependant timer interrupt.
 */
void timer_process(void);

/** Check if timer is pending
 *
 * @param[in] timer  timer
 * @return 0 if timer if not pending, 1 otherwise
 */
static inline uint8_t timer_is_pending(tim_t *timer)
{
	return !list_empty(&timer->list);
}

void timer_checks(void);
#endif
