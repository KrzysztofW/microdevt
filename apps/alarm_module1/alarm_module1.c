#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <rf.h>
#include <timer.h>

static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
#if 1
	sbuf_t s = SBUF_INITS("Hello world!");
	rf_sendto(0x69, &s);
#else
	buf_t buf;
	//uint8_t i;
	uint8_t data[] = {
		0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
		//0x55, 0x55, 0x55, 0x55, 0x01, 0xFF, 0x02, 0x03,
		//0x04, 0x05, 0x6, 0x7, 0x8, 0x9, 0xa,
	};
	uint8_t i;

	/* rf_arm_snd_timer(); */
	/* return; */

	buf_init(&buf, data, sizeof(data));
	if (rf_send_data(&buf) < 0)
		while (1) {}
	for (i = 0; i < 50; i++) {
		if (rf_send_byte(i) < 0)
			while (1) {}
	}
	rf_start_sending();
#endif
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
	init_streams();
	DEBUG_LOG("KW alarm module 1\n");
#endif
	watchdog_shutdown();
	timer_subsystem_init();

#ifdef CONFIG_TIMER_CHECKS
	watchdog_shutdown();
	delay_ms(1000); /* wait for system to be initialized */
	timer_checks();
#endif
	memset(&timer_rf, 0, sizeof(tim_t));
	timer_add(&timer_led, 2000000, tim_led_cb, &timer_led);

	watchdog_enable();

#ifdef CONFIG_RF_SENDER
	if (rf_init() < 0) {
		DEBUG_LOG("can't initialize RF\n");
		return -1;
	}
	DEBUG_LOG("RF initialized\n");
	memset(&timer_rf, 0, sizeof(tim_t));
	timer_add(&timer_rf, 2000000, tim_rf_cb, &timer_rf);
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
