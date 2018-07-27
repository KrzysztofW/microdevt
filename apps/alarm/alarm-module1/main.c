#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <timer.h>
#include <scheduler.h>
#include "alarm-module1.h"
#include "../module.h"

void tim_led_cb(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= 1 << PD4;
	timer_reschedule(timer, 1000000UL);
}

INIT_ADC_DECL(c, DDRC, PORTC);

int main(void)
{
	tim_t timer_led;

	init_adc_c();
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 0);
	DEBUG_LOG("KW alarm module 1\n");
#endif
	watchdog_shutdown();
	timer_subsystem_init();
	scheduler_init();

#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	timer_init(&timer_led);
	timer_add(&timer_led, 3000000, tim_led_cb, &timer_led);
	watchdog_enable();

	/* port D used by the LED and RF sender */
	DDRD = (1 << PD2);
	sei();

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
	alarm_module1_rf_init();
	module_init();
#endif

	/* incorruptible functions */
	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
