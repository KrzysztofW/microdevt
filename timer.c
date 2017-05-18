#include <assert.h>
#include <stdlib.h>
#ifndef ZCHK
#include <avr/io.h>
#include <avr/interrupt.h>
#endif
#include <stdio.h>

#include "timer.h"

#define TIMER_TABLE_SIZE 32
#define TIMER_TABLE_MASK (TIMER_TABLE_SIZE - 1)

#ifdef TIMER_DEBUG
#define TIMER_PRINTF(args...) printf(args)
#else
#define TIMER_PRINTF(args...)
#endif

#define MIN_TIMER_RES 300
static unsigned long timer_resolution_us = MIN_TIMER_RES;

struct timer_state {
	int current_idx;
	struct list_head timer_list[TIMER_TABLE_SIZE];
} timer_state;

static void timer_process(void)
{
	int idx;
	struct list_head *pos, *n;

	timer_state.current_idx = idx = (timer_state.current_idx + 1) & TIMER_TABLE_MASK;
	list_for_each_safe(pos, n, &timer_state.timer_list[idx]) {
		tim_t *timer = list_entry(pos, tim_t, list);

		assert(timer->status == TIMER_SCHEDULED);

		if (timer->remaining_loops > 0) {
			timer->remaining_loops--;
			continue;
		}
		timer->status = TIMER_STOPPED;
		(*timer->cb)(timer->arg);
		if (timer->status == TIMER_SCHEDULED) {
			/* timer reloaded by the callback function */
			continue;
		}
		list_del(&timer->list);
		timer->status = TIMER_STOPPED;
	}
}

static void timer_init_list(void)
{
	int i;

	for (i = 0; i < TIMER_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&timer_state.timer_list[i]);
}

#ifndef ZCHK
static inline void timer_enable_int(void)
{
	TIMSK0 = (1 << TOIE0); /* Enable timer0 overflow interrupt(TOIE0) */
}

static inline void timer_disable_int(void)
{
	TIMSK0 = 0;
}

/* 8-bit timer 300us resolution
 * 255-0.0003/(1/(16000000/64.)) = 180
 * where 255 is the 8-bit counter max value
 *       0.0003 = 300us (RF HI/LOW duration)
 *       1/(16000000) = 1/F_CPU
 *       64 = prescaler value
 */
#define TIM_COUNTER 180
//#define TIM_COUNTER 217

ISR(TIMER0_OVF_vect)
{
	timer_disable_int();
	timer_process();
	TCNT0 = TIM_COUNTER;
	timer_enable_int();
}

int timer_subsystem_init(unsigned long resolution_us)
{
	TIMER_PRINTF("timer initialization\n");

	if (resolution_us < MIN_TIMER_RES) {
		printf("timer resolution cannot be smaller than %d", MIN_TIMER_RES);
		return -1;
	}

	timer_resolution_us = resolution_us;
	timer_init_list();

	/* 8-bit timer */
	TCNT0 = TIM_COUNTER;
	TCCR0A = 0x00;
	TCCR0B = (1<<CS00) | (1<<CS01);  // Timer mode with 64 prescler
	timer_enable_int();
	sei();

	return 0;
}
#else
void __zchk_timer_process(void)
{
	timer_process();
}

int timer_subsystem_init(unsigned long resolution_us)
{
	timer_resolution_us = resolution_us;
	timer_init_list();
	return 0;
}
#endif

void timer_add(tim_t *timer, unsigned long expiry_us, void (*cb)(void *), void *arg)
{
	int idx;
	unsigned long ticks = expiry_us / timer_resolution_us;

	assert(timer->status == TIMER_STOPPED);
	assert(timer->cb || cb);
	assert(timer->arg || arg);

	if (cb != NULL) {
		timer->cb = cb;
	}
	if (arg != NULL) {
		timer->arg = arg;
	}

	/* don't schedule at current idx */
	if (ticks == 0) {
		ticks = 1;
	}

	idx = (timer_state.current_idx + ticks) & TIMER_TABLE_MASK;
	timer->remaining_loops = ticks / TIMER_TABLE_SIZE; /* >> TIMER_TABLE_ORDER */

	/* current_idx will be processed one loop later */
	if (timer_state.current_idx == idx && timer->remaining_loops) {
		timer->remaining_loops--;
	}

	list_add_tail(&timer->list, &timer_state.timer_list[idx]);
	timer->status = TIMER_SCHEDULED;
}

void timer_del(tim_t *timer)
{
	list_del(&timer->list);
	timer->status = TIMER_STOPPED;
}

void timer_reschedule(tim_t *timer, unsigned long expiry_us)
{
	timer_del(timer);
	timer_add(timer, expiry_us, NULL, NULL);
}

int timer_is_pending(tim_t *timer)
{
	return timer->status == TIMER_SCHEDULED;
}
