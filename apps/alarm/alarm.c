#include <log.h>
#include <_stdio.h>
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <timer.h>
#include <scheduler.h>
#include <net/pkt-mempool.h>
#include "alarm.h"

INIT_ADC_DECL(f, DDRF, PORTF)

static void blink_led(void *arg)
{
	tim_t *tim = arg;

	PORTB ^= (1 << PB7);
	timer_reschedule(tim, 1000000UL);
}

int main(void)
{
	tim_t timer_led;
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 0);
	DEBUG_LOG("KW alarm v0.2\n");
#endif
	init_adc_f();
	timer_subsystem_init();
	watchdog_shutdown();
#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER	\
	|| defined CONFIG_NETWORKING
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
#endif
	scheduler_init();

	watchdog_enable();
	sei();
	alarm_network_init();
	alarm_gsm_init();
	alarm_rf_init();

	/* slow functions */
	while (1) {
#ifdef CONFIG_NETWORKING
		alarm_network_loop();
#endif
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
