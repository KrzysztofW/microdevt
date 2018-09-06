#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

static void timer_disable_timer_int(tim_t *timer)
{
	if (timer->status != TIMER_RUNNING)
		disable_timer_int();
}

static void timer_enable_timer_int(tim_t *timer)
{
	if (timer->status != TIMER_RUNNING)
		enable_timer_int();
}

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

void timer_add(tim_t *timer, unsigned long expiry_us, void (*cb)(void *),
	       void *arg)
{
	unsigned int idx;
	unsigned long ticks = expiry_us / CONFIG_TIMER_RESOLUTION_US;

	assert(timer->status == TIMER_STOPPED || timer->status == TIMER_RUNNING);

	timer->cb = cb;
	timer->arg = arg;

	/* don't schedule at current idx */
	if (ticks == 0)
		ticks = 1;

	timer->remaining_loops = ticks / TIMER_TABLE_SIZE;

	timer_disable_timer_int(timer);
	idx = (timer_state.current_idx + ticks) & TIMER_TABLE_MASK;
	list_add_tail(&timer->list, &timer_state.timer_list[idx]);
	timer_enable_timer_int(timer);
	timer->status = TIMER_SCHEDULED;
}

void timer_del(tim_t *timer)
{
	if (timer_is_pending(timer)) {
		timer_disable_timer_int(timer);
		list_del(&timer->list);
		timer_enable_timer_int(timer);
		timer->status = TIMER_STOPPED;
	}
}

void timer_reschedule(tim_t *timer, unsigned long expiry_us)
{
	timer_add(timer, expiry_us, timer->cb, timer->arg);
}

int timer_is_pending(tim_t *timer)
{
	return timer->status == TIMER_SCHEDULED;
}

#ifdef CONFIG_TIMER_CHECKS
#define TIM_CNT 64
tim_t timers[TIM_CNT];
unsigned long timer_iters;
uint8_t is_random;

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

int timer_check_subsequent_deletion_failed;

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
	timer_add(&timers[0], 100, check_subsequent_deletion_cb1, &timers[0]);
	timer_add(&timers[1], 100, check_subsequent_deletion_cb2, &timers[1]);
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
	wait_to_finish();
	LOG("timer reschedule test passed\n");

	is_random = 1;
	arm_timers();
	wait_to_finish();
	LOG("timer random reschedule test passed\n");

	LOG("\n=== timer tests passed ===\n\n");
}
#endif
