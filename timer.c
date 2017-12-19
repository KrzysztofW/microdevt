#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <log.h>
#include "timer.h"

#define TIMER_TABLE_SIZE 8
#define TIMER_TABLE_MASK (TIMER_TABLE_SIZE - 1)

#ifdef TIMER_DEBUG
#define TIMER_PRINTF(args...) LOG(args)
#else
#define TIMER_PRINTF(args...)
#endif

struct timer_state {
	int current_idx;
	list_t timer_list[TIMER_TABLE_SIZE];
} __attribute__((__packed__));
struct timer_state timer_state;

void timer_process(void)
{
	int idx;
	list_t *pos, *n;
	uint8_t process_again = 0;

	timer_state.current_idx = idx = (timer_state.current_idx + 1)
		& TIMER_TABLE_MASK;

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

		assert(timer->status == TIMER_SCHEDULED);

		if (timer->remaining_loops > 0 && process_again == 0) {
			timer->remaining_loops--;
			continue;
		}
		timer->status = TIMER_STOPPED;

		/* we are in a timer interrupt, interrups are disabled */
		list_del_init(&timer->list);

		(*timer->cb)(timer->arg);
	}
}

int timer_subsystem_init(void)
{
	int i;

	TIMER_PRINTF("timer initialization\n");

	for (i = 0; i < TIMER_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&timer_state.timer_list[i]);

	__timer_subsystem_init();

	return 0;
}

void timer_add(tim_t *timer, unsigned long expiry_us, void (*cb)(void *),
	       void *arg)
{
	int idx;
	unsigned long ticks = expiry_us / CONFIG_TIMER_RESOLUTION_US;

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
uint8_t is_random;

static void timer_cb(void *arg)
{
	tim_t *tim = arg;
	uint16_t expiry = is_random ? rand() : 0;

	if (timer_iters < 1000*TIM_CNT)
		timer_reschedule(tim, expiry);
	timer_iters++;
}

static void timer_arm(void)
{
	int i;
	uint16_t expiry = is_random ? rand() : 0;

	timer_iters = 0;
	for (i = 0; i < TIM_CNT; i++)
		timer_add(&timers[i], expiry, timer_cb, &timers[i]);
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

static int timer_check_stopped(void)
{
	int i;

	for (i = 0; i < TIM_CNT; i++) {
		if (timer_is_pending(&timers[i]))
			return -1;
	}
	return 0;
}

static void timer_delete_timers(void)
{
	int i;

	for (i = TIM_CNT - 1; i >= 0; i--)
		timer_del(&timers[i]);
}

int timer_check_subsequent_deletion_failed;

static void timer_check_subsequent_deletion_cb2(void *arg)
{
	LOG("%s: this function should never be called\n", __func__);
	timer_check_subsequent_deletion_failed = 1;
}

static void timer_check_subsequent_deletion_cb1(void *arg)
{
	timer_del(&timers[1]);
}

static void timer_check_subsequent_deletion(void)
{

	timer_add(&timers[0], 100, timer_check_subsequent_deletion_cb1,
		  &timers[0]);
	timer_add(&timers[1], 100, timer_check_subsequent_deletion_cb2,
		  &timers[1]);
}

void timer_checks(void)
{
	int i;

	LOG("\n=== starting timer tests ===\n\n");

	timer_check_subsequent_deletion();
	timer_wait_to_finish();
	if (timer_check_stopped() < 0 || timer_check_subsequent_deletion_failed) {
		LOG("timer subsequent deletion test failed\n");
		return;
	}
	LOG("timer subsequent deletion test succeeded\n");

	for (i = 0; i < 300; i++) {
		timer_arm();
		timer_delete_timers();
		if (timer_check_stopped() < 0) {
			LOG("timer deletion test failed\n");
			return;
		}
	}
	LOG("timer deletion test passed\n");

	timer_arm();
	timer_wait_to_finish();
	LOG("timer reschedule test passed\n");

	is_random = 1;
	timer_arm();
	timer_wait_to_finish();
	LOG("timer random reschedule test passed\n");

	LOG("\n=== timer tests passed ===\n\n");
}
#endif
