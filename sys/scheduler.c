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

#include <interrupts.h>
#include "power-management.h"
#include "ring.h"
#include "scheduler.h"

typedef struct __PACKED__ task {
	void (*cb)(void *arg);
	void *arg;
} task_t;

#ifndef CONFIG_SCHEDULER_MAX_TASKS
#define CONFIG_SCHEDULER_MAX_TASKS 16
#endif
#ifndef CONFIG_SCHEDULER_TASK_WATER_MARK
#define CONFIG_SCHEDULER_TASK_WATER_MARK 8
#endif
#define SCHEDULER_TASK_WATER_MARK				\
	(CONFIG_SCHEDULER_TASK_WATER_MARK * sizeof(task_t))

#define RING_SIZE (CONFIG_SCHEDULER_MAX_TASKS * sizeof(task_t))

STATIC_RING_DECL(ring, ROUNDUP_PWR2(RING_SIZE));
STATIC_RING_DECL(ring_irq, ROUNDUP_PWR2(RING_SIZE));

#ifdef CONFIG_POWER_MANAGEMENT
static uint8_t idle;
#endif

static void __scheduler_run_task(ring_t *r)
{
	task_t task;
	buf_t buf = BUF_INIT(&task, sizeof(task_t));
#ifdef DEBUG
	int rlen = ring_len(r);

	assert(rlen > 0);
	assert(rlen % sizeof(task_t) == 0);
#endif
	__ring_get_buf(r, &buf);
	task.cb(task.arg);
#ifdef CONFIG_POWER_MANAGEMENT
	idle = 0;
#endif
}

#ifdef DEBUG
void __schedule_task(void (*cb)(void *arg), void *arg,
		     const char *func, int line)
#else
void schedule_task(void (*cb)(void *arg), void *arg)
#endif
{
	task_t task = {
		.cb = cb,
		.arg = arg,
	};
	ring_t *r = IRQ_CHECK() ? ring : ring_irq;

	if (ring_add(r, &task, sizeof(task_t)) >= 0)
		return;
	DEBUG_LOG("cannot schedule task %p from %s:%d\n", cb, func, line);
}

void scheduler_run_task(void)
{
	int irq_rlen = ring_len(ring_irq);

#ifdef CONFIG_POWER_MANAGEMENT
	idle = 1;
#endif
	if (irq_rlen) {
		if (irq_rlen >= SCHEDULER_TASK_WATER_MARK)
			irq_disable();
		else
			irq_enable();
		__scheduler_run_task(ring_irq);
	}

	if (ring_len(ring))
		__scheduler_run_task(ring);

#ifdef CONFIG_POWER_MANAGEMENT
	if (idle) {
		power_management_set_mode(PWR_MGR_SLEEP_MODE_IDLE);
		power_management_sleep();
	}
#endif
}
