#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <timer.h>
#include <drivers/gsm-at.h>
#include <scheduler.h>
#include "alarm-module1.h"

#ifdef CONFIG_GSM_SIM900
static void gsm_cb(uint8_t status, const sbuf_t *from, const sbuf_t *msg)
{
	DEBUG_LOG("gsm callback (status:%d)\n", status);
	if (status == GSM_STATUS_RECV) {
		DEBUG_LOG("from:");
		sbuf_print(from);
		DEBUG_LOG(" msg:<");
		sbuf_print(msg);
		DEBUG_LOG(">\n");
	}
}

ISR(USART_RX_vect)
{
	gsm_handle_interrupt(UDR0);
}

#endif

void tim_led_cb(void *arg)
{
	tim_t *timer = arg;
#ifdef CONFIG_GSM_SIM900
	static uint8_t gsm;
#endif
	PORTD ^= 1 << PD4;
	timer_reschedule(timer, 1000000UL);
#ifdef CONFIG_GSM_SIM900
	if ((gsm % 60) == 0)
		gsm_send_sms("+33687236420", "SMS from KW alarm");
	gsm++;
#endif
}

INIT_ADC_DECL(c, DDRC, PORTC)

int main(void)
{
	tim_t timer_led;

	init_adc_c();
#ifdef DEBUG
#ifdef CONFIG_GSM_SIM900
	init_stream0(&stdout, &stdin, 1);
#else
	init_stream0(&stdout, &stdin, 0);
#endif
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

#ifdef CONFIG_GSM_SIM900
	gsm_init(stdin, stdout, gsm_cb);
#endif
	alarm_module1_rf_init();

	/* incorruptible functions */
	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
