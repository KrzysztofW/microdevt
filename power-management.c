#include "power-management.h"
#include "timer.h"
#include "scheduler.h"

static tim_t timer;
static uint16_t pwr_mgr_sleep_timeout;
static void (*on_sleep_cb)(void *arg);
static void *on_sleep_arg;
uint16_t power_management_inactivity;

#define ONE_SECOND 1000000

static void enter_sleep_mode_cb(void *arg)
{
	if (on_sleep_cb)
		on_sleep_cb(on_sleep_arg);
	power_management_set_mode(PWR_MGR_SLEEP_MODE_PWR_DOWN);
	power_management_sleep();
}

static void timer_cb(void *arg)
{
	timer_reschedule(&timer, ONE_SECOND);
	power_management_pwr_down_set_idle();

	if (power_management_inactivity < pwr_mgr_sleep_timeout)
		return;
	/* interrupts must be enabled when entering sleep mode */
	schedule_task(enter_sleep_mode_cb, NULL);
}

void
power_management_power_down_init(uint16_t inactivity_timeout,
				 void (*on_sleep)(void *arg), void *arg)
{
	pwr_mgr_sleep_timeout = inactivity_timeout;
	on_sleep_cb = on_sleep;
	on_sleep_arg = arg;
	timer_add(&timer, ONE_SECOND, timer_cb, NULL);
}
