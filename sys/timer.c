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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <interrupts.h>

#include <log.h>
#include <common.h>
#include "timer.h"

#define TIMER_TABLE_SIZE 16
#define TIMER_TABLE_MASK (TIMER_TABLE_SIZE - 1)

static uint8_t cur_idx;
static list_t timer_list[TIMER_TABLE_SIZE];
static uint16_t timer_count;

#ifdef DEBUG_TIMERS
static void timer_dump_list(list_t *head)
{
	tim_t *timer;
	uint8_t first = 1;

	list_for_each_entry(timer, head, list) {
		if (first) {
			LOG("head:%p (p:%p n:%p) ", head,
			    head->prev, head->next);
			first = 0;
		}
		LOG("tim:%p (prev:%p next:%p) in %s:%u", timer,
		    timer->list.prev, timer->list.next, timer->func,
		    timer->line);
	}
	if (!first)
		LOG("\n");
}

void timer_dump(void)
{
	uint8_t i;
	uint8_t flags;

	irq_save(flags);
	for (i = 0; i < TIMER_TABLE_SIZE; i++)
		timer_dump_list(&timer_list[i]);
	irq_restore(flags);
}
#endif

static uint8_t ticks_to_idx(uint32_t *delta_ticks)
{
	uint8_t idx;

	if (*delta_ticks >= TIMER_TABLE_SIZE) {
		idx = cur_idx - 1;
		*delta_ticks -= TIMER_TABLE_MASK;
	} else {
		idx = cur_idx + *delta_ticks;
		*delta_ticks = 0;
	}
	idx &= TIMER_TABLE_MASK;
	return idx;
}

void timer_process(void)
{
	cur_idx = (cur_idx + 1) & TIMER_TABLE_MASK;

	while (!list_empty(&timer_list[cur_idx])) {
		tim_t *timer = list_first_entry(&timer_list[cur_idx], tim_t,
						list);

		if (timer->ticks > 0) {
			uint8_t i = ticks_to_idx(&timer->ticks);

			list_move_tail(&timer->list, &timer_list[i]);
			continue;
		}
		list_del_init(&timer->list);
		timer_count--;
		(*timer->cb)(timer->arg);
	}

	if (timer_count == 0)
		__timer_subsystem_stop();
}

void timer_subsystem_init(void)
{
	int i;

	STATIC_ASSERT(TIMER_TABLE_SIZE < MAX_VALUE_UNSIGNED(cur_idx));
	for (i = 0; i < TIMER_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&timer_list[i]);
	__timer_subsystem_init();
}

void timer_init(tim_t *timer)
{
	INIT_LIST_HEAD(&timer->list);
}

#ifdef DEBUG_TIMERS
void __timer_add(const char *func, unsigned line, tim_t *timer, uint32_t expiry,
		 void (*cb)(void *), void *arg)
#else
void timer_add(tim_t *timer, uint32_t expiry, void (*cb)(void *), void *arg)

#endif
{
	uint8_t idx;
	uint8_t flags;

	if (timer_is_pending(timer)) {
#ifdef DEBUG_TIMERS
		DEBUG_LOG("%s:%d (%p) in %s:%u\n", func, line, timer,
			  timer->func, timer->line);
#endif
		__abort();
	}
#ifdef DEBUG_TIMERS
	timer->func = func;
	timer->line = line;
#endif
	timer->ticks = expiry / CONFIG_TIMER_RESOLUTION_US;

	/* don't schedule at current idx */
	if (timer->ticks == 0)
		timer->ticks = 1;

	timer->cb = cb;
	timer->arg = arg;

	irq_save(flags);
	idx = ticks_to_idx(&timer->ticks);
	list_add_tail(&timer->list, &timer_list[idx]);
	timer_count++;
	irq_restore(flags);

	if (!__timer_subsystem_is_runing())
		__timer_subsystem_start();
}

void timer_del(tim_t *timer)
{
	uint8_t flags;

	if (!timer_is_pending(timer))
		return;

	irq_save(flags);
	list_del_init(&timer->list);
	timer_count--;
	irq_restore(flags);
}

#ifdef DEBUG_TIMERS
void __timer_reschedule(const char *func, unsigned line, tim_t *timer,
			uint32_t expiry)
{
	__timer_add(func, line, timer, expiry, timer->cb, timer->arg);
}
#else
void timer_reschedule(tim_t *timer, uint32_t expiry)
{
	timer_add(timer, expiry, timer->cb, timer->arg);
}
#endif

#ifdef CONFIG_TIMER_CHECKS
#define TIM_CNT 64U
static tim_t timers[TIM_CNT];
static uint32_t timer_iters;
static uint8_t is_random;

static void timer_cb(void *arg)
{
	tim_t *tim = arg;
	uint16_t expiry = is_random ? rand() : 0;

	if (timer_iters < 1000 * TIM_CNT)
		timer_reschedule(tim, expiry);
	timer_iters++;
}

static void arm_timers(void)
{
	int i;
	uint16_t expiry = is_random ? rand() : 0;

	timer_iters = 0;
	for (i = 0; i < TIM_CNT; i++)
		timer_add(&timers[i], expiry, timer_cb, &timers[i]);
}

static void wait_to_finish(void)
{
	while (1) {
		int i, cnt = 0;

		for (i = 0; i < TIM_CNT; i++) {
			if (timer_is_pending(&timers[i]))
				break;
			cnt++;
		}
		if (cnt == TIM_CNT)
			break;
	}
}

static int check_stopped(void)
{
	int i;

	for (i = 0; i < TIM_CNT; i++) {
		if (timer_is_pending(&timers[i]))
			return -1;
	}
	return 0;
}

static void delete_timers(void)
{
	int i;

	for (i = TIM_CNT - 1; i >= 0; i--)
		timer_del(&timers[i]);
}

static void random_reschedule(void)
{
	int i;

	for (i = 0; i < TIM_CNT * 100; i++) {
		int j = rand() % TIM_CNT;

		timer_del(&timers[j]);
		timer_add(&timers[j], rand(), timer_cb, &timers[j]);
	}
}

static int timer_check_subsequent_deletion_failed;

static void check_subsequent_deletion_cb2(void *arg)
{
	LOG("this function should have never been called\n");
	timer_check_subsequent_deletion_failed = 1;
}

static void check_subsequent_deletion_cb1(void *arg)
{
	timer_del(&timers[1]);
}

static void check_subsequent_deletion(void)
{
	/* these two adds must be atomic */
	irq_disable();
	timer_add(&timers[0], 200, check_subsequent_deletion_cb1, &timers[0]);
	timer_add(&timers[1], 200, check_subsequent_deletion_cb2, &timers[1]);
	irq_enable();
}

void timer_checks(void)
{
	int i;

	for (i = 0; i < TIM_CNT; i++)
		timer_init(&timers[i]);

	LOG("\n=== starting timer tests ===\n\n");

	check_subsequent_deletion();
	wait_to_finish();
	if (check_stopped() < 0 || timer_check_subsequent_deletion_failed) {
		LOG("timer subsequent deletion test failed\n");
		return;
	}
	LOG("timer subsequent deletion test succeeded\n");

	for (i = 0; i < 300 && 0; i++) {
		arm_timers();
		delete_timers();
		if (check_stopped() < 0) {
			LOG("timer deletion test failed\n");
			return;
		}
	}
	LOG("timer deletion test passed\n");

	arm_timers();
	random_reschedule();
	wait_to_finish();
	LOG("timer random reschedule test 1 passed\n");

	arm_timers();
	wait_to_finish();
	LOG("timer reschedule test passed\n");

	is_random = 1;
	arm_timers();
	wait_to_finish();
	LOG("timer random reschedule test 2 passed\n");

	LOG("\n=== timer tests passed ===\n\n");
}
#endif
