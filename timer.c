#include <assert.h>
#include <stdlib.h>
#ifdef CONFIG_AVR_MCU
#include <avr/io.h>
#include <avr/interrupt.h>
#endif
#include <stdio.h>

#include "timer.h"

#define TIMER_TABLE_SIZE 8
#define TIMER_TABLE_MASK (TIMER_TABLE_SIZE - 1)

#ifdef TIMER_DEBUG
#define TIMER_PRINTF(args...) printf(args)
#else
#define TIMER_PRINTF(args...)
#endif

/* 8-bit timer 300us resolution
 * 255-0.0003/(1/(16000000/64.)) = 180
 * where 255 is the 8-bit counter max value
 *       0.0003 = 300us (RF HI/LOW duration)
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       64 = prescaler value
 *
 * 16-bit timer 150us resolution
 * 65535-0.00015/(1/(16000000/8.)) = 65235
 * where 65535 is the 16-bit counter max value
 *       0.00015 = 150us (RF HI/LOW duration)
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       8 = prescaler value
 *       <=> (65535 - (150*(16000000/8))/1000000)
 */
#ifdef CONFIG_AVR_MCU
#define MIN_TIMER_RES 150UL // XXX the scope show a resolution of 200us!
#define TIM_COUNTER (UINT16_MAX - (MIN_TIMER_RES*(CONFIG_AVR_F_CPU/8))/1000000)
#else
#define MIN_TIMER_RES 1
#endif
static unsigned long timer_resolution_us = MIN_TIMER_RES;

struct timer_state {
	int current_idx;
	struct list_head timer_list[TIMER_TABLE_SIZE];
} __attribute__((__packed__));
struct timer_state timer_state;

static void timer_process(void)
{
	int idx;
	struct list_head *pos, *n;

	timer_state.current_idx = idx = (timer_state.current_idx + 1)
		& TIMER_TABLE_MASK;
	list_for_each_safe(pos, n, &timer_state.timer_list[idx]) {
		tim_t *timer = list_entry(pos, tim_t, list);

		assert(timer->status == TIMER_SCHEDULED);

		if (timer->remaining_loops > 0) {
			timer->remaining_loops--;
			continue;
		}
		timer->status = TIMER_STOPPED;

		/* we are in a timer interrupt, interrups are disabled */
		list_del_init(&timer->list);

		(*timer->cb)(timer->arg);
	}
}

#ifdef CONFIG_AVR_MCU
/* 8-bit timer */
#if 0
ISR(TIMER0_OVF_vect)
{
	timer_process();
	TCNT0 = TIM_COUNTER;
}
#endif

/* 16-bit timer */
ISR(TIMER1_OVF_vect)
{
	timer_process();
	TCNT1H = (TIM_COUNTER >> 8) & 0xFF;
	TCNT1L = TIM_COUNTER & 0xFF;
}

static inline void disable_timer_int(void)
{
	TCCR1B = 0;
}

static inline void enable_timer_int(void)
{
	TCCR1B = (1<<CS11);
}
#endif

int timer_subsystem_init(unsigned long resolution_us)
{
	int i;

	TIMER_PRINTF("timer initialization\n");

	for (i = 0; i < TIMER_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&timer_state.timer_list[i]);

	if (resolution_us < MIN_TIMER_RES) {
		TIMER_PRINTF("timer resolution cannot be smaller than %lu",
			     MIN_TIMER_RES);
		return -1;
	}

	timer_resolution_us = resolution_us;

#ifdef CONFIG_AVR_MCU
#if 0
	/* 8-bit timer */
	TCNT0 = TIM_COUNTER;
	TCCR0A = 0;
	TCCR0B = (1<<CS00) | (1<<CS01); /* Timer mode with 64 prescler */
	TIMSK0 = (1 << TOIE0); /* Enable timer0 overflow interrupt(TOIE0) */
#endif
	/* 16-bit timer */
	TCNT1H = (TIM_COUNTER >> 8);
	TCNT1L = TIM_COUNTER & 0xFF;

	TCCR1A = 0;
	TCCR1B = (1<<CS11);    /* Timer mode with 8 prescler */
	TIMSK1 = (1 << TOIE1); /* Enable timer0 overflow interrupt(TOIE1) */

	sei();
#endif

	return 0;
}

#ifndef CONFIG_AVR_MCU
#define disable_timer_int()
#define enable_timer_int()

void __timer_process(void)
{
	timer_process();
}
#endif

void timer_add(tim_t *timer, unsigned long expiry_us, void (*cb)(void *),
	       void *arg)
{
	int idx;
	unsigned long ticks = expiry_us / timer_resolution_us;

	assert(timer->status == TIMER_STOPPED);
	assert(timer->cb || cb);

	if (cb != NULL)
		timer->cb = cb;
	if (arg != NULL)
		timer->arg = arg;

	/* don't schedule at current idx */
	if (ticks == 0)
		ticks = 1;

	idx = (timer_state.current_idx + ticks) & TIMER_TABLE_MASK;
	timer->remaining_loops = ticks / TIMER_TABLE_SIZE; /* >> TIMER_TABLE_ORDER */

	timer->status = TIMER_SCHEDULED;

	/* Current_idx will be processed one loop later.
	 * If current_idx == idx, current_idx has already been processed.
	 */
	if (timer_state.current_idx == idx && timer->remaining_loops)
		timer->remaining_loops--;

	/* timer_process() might fire up during the list insertion */
	disable_timer_int();
	list_add_tail(&timer->list, &timer_state.timer_list[idx]);
	enable_timer_int();
}

void timer_del(tim_t *timer)
{
	disable_timer_int();
	list_del(&timer->list);
	enable_timer_int();
	timer->status = TIMER_STOPPED;
}

void timer_reschedule(tim_t *timer, unsigned long expiry_us)
{
	timer_add(timer, expiry_us, NULL, NULL);
}

int timer_is_pending(tim_t *timer)
{
	return timer->status == TIMER_SCHEDULED;
}

#ifdef CONFIG_TIMER_CHECKS
#define TIM_CNT 64UL
tim_t timers[TIM_CNT];
unsigned long timer_iters;

static void timer_cb(void *arg)
{
	tim_t *tim = arg;

	if (timer_iters < 3000*TIM_CNT)
		timer_reschedule(tim, 0);
	timer_iters++;
}

static void timer_arm(uint8_t is_random)
{
	int i;

	timer_iters = 0;
	for (i = 0; i < TIM_CNT; i++) {
		unsigned long expiry = is_random ? rand() : 0;

		timer_add(&timers[i], expiry, timer_cb, &timers[i]);
	}
}

static void timer_wait_to_finish(void)
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

void timer_checks(void)
{
	int i;

	printf("\n=== starting timer tests ===\n\n");

	timer_arm(0);
	for (i = TIM_CNT - 1; i >= 0; i--)
		timer_del(&timers[i]);
	timer_wait_to_finish();
	printf("timer deletion test passed\n");

	timer_arm(0);
	timer_wait_to_finish();
	printf("timer reschedule test passed\n");

	timer_arm(1);
	timer_wait_to_finish();
	printf("timer random reschedule test passed\n");

	printf("\n=== timer tests passed ===\n\n");
}
#endif
