#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <interrupts.h>

#include <log.h>
#include <common.h>
#include "timer.h"

#define TIMER_TABLE_SIZE 8
#define TIMER_TABLE_MASK (TIMER_TABLE_SIZE - 1)

struct timer_state {
	unsigned int current_idx;
	list_t timer_list[TIMER_TABLE_SIZE];
} __attribute__((__packed__));
struct timer_state timer_state;

void timer_process(void)
{
	unsigned int idx;
	list_t *pos, *n, *last_timer = NULL;
	uint8_t process_again = 0;

	idx = (timer_state.current_idx + 1) & TIMER_TABLE_MASK;
	timer_state.current_idx = idx;

 again:
	list_for_each_safe(pos, n, &timer_state.timer_list[idx]) {
		tim_t *timer = list_entry(pos, tim_t, list);

		/* If the next entry is removed by the callback here,
		 * the list has to be processed again from the begining as
		 * 'n' will point to the deleted entry's next value.
		 */
		if (n == LIST_POISON1) {
			process_again = 1;
			goto again;
		}
		if (process_again && last_timer) {
			if (last_timer == pos) {
				process_again = 0;
				last_timer = NULL;
			}
			continue;
		}

		assert(timer->status == TIMER_SCHEDULED);

		if (timer->remaining_loops > 0) {
			timer->remaining_loops--;
			last_timer = pos;
			continue;
		}
		timer->status = TIMER_RUNNING;

		/* we are in a timer interrupt, interrups are disabled */
		list_del_init(&timer->list);

		(*timer->cb)(timer->arg);

		/* the timer has been rescheduled */
		if (timer->status != TIMER_SCHEDULED)
			timer->status = TIMER_STOPPED;
	}
}

void timer_subsystem_init(void)
{
	int i;

	for (i = 0; i < TIMER_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&timer_state.timer_list[i]);
	__timer_subsystem_init();
}

#ifdef X86
void timer_subsystem_shutdown(void)
{
	__timer_subsystem_shutdown();
}
#endif

void timer_init(tim_t *timer)
{
	memset(timer, 0, sizeof(tim_t));
}

/* set the tick duration to the current timer resolution */
#ifdef CONFIG_TIMER_RESOLUTION_MS
#define TIMER_RESOLUTION CONFIG_TIMER_RESOLUTION_MS
#else
#define TIMER_RESOLUTION CONFIG_TIMER_RESOLUTION_US
#endif

void timer_add(tim_t *timer, uint32_t expiry, void (*cb)(void *), void *arg)
{
	unsigned int idx;
	uint32_t ticks = expiry / TIMER_RESOLUTION;
	uint8_t flags;

	/* don't schedule at current idx */
	if (ticks == 0)
		ticks = 1;

	assert(timer->status == TIMER_STOPPED || timer->status == TIMER_RUNNING);

	timer->cb = cb;
	timer->arg = arg;
	timer->remaining_loops = ticks / TIMER_TABLE_SIZE;
	timer->status = TIMER_SCHEDULED;

	irq_save(flags);
	idx = (timer_state.current_idx + ticks) & TIMER_TABLE_MASK;
	list_add_tail(&timer->list, &timer_state.timer_list[idx]);
	irq_restore(flags);
}

void timer_del(tim_t *timer)
{
	uint8_t flags;

	if (!timer_is_pending(timer))
		return;

	irq_save(flags);
	if (timer_is_pending(timer)) {
		list_del(&timer->list);
		timer->status = TIMER_STOPPED;
	}
	irq_restore(flags);
}

void timer_reschedule(tim_t *timer, uint32_t expiry)
{
	timer_add(timer, expiry, timer->cb, timer->arg);
}

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
