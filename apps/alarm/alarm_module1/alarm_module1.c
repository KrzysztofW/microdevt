#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <rf.h>
#include <timer.h>
#include "../rf_common.h"

static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(16);
	const char *s = "Hello world!";

	__buf_adds(&buf, s);
	rf_sendto(0x69, &buf, 2);
	timer_reschedule(timer, 5000000UL);
}

void tim_led_cb(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= 1 << PD4;
	timer_reschedule(timer, 1000000UL);
}

int main(void)
{
	tim_t timer_rf;
	tim_t timer_led;

	init_adc();
#ifdef DEBUG
	init_stream0(&stdout, &stdin);
	DEBUG_LOG("KW alarm module 1\n");
#endif
	watchdog_shutdown();
	timer_subsystem_init();

#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	timer_init(&timer_led);
	timer_add(&timer_led, 0, tim_led_cb, &timer_led);
	watchdog_enable();

#ifdef CONFIG_RF_SENDER
	if (rf_init() < 0) {
		DEBUG_LOG("can't initialize RF\n");
		return -1;
	}
	timer_init(&timer_rf);
	timer_add(&timer_rf, 0, tim_rf_cb, &timer_rf);

	/* port D used by the LED and RF sender */
	DDRD = (1 << PD2);
#endif
	while (1) {
		/* slow functions */

		watchdog_reset();
		delay_ms(1);
	}
#ifdef CONFIG_RF_SENDER
	/* rf_shutdown(); */
#endif
	return 0;
}
