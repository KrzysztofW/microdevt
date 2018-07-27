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
#include "module.h"

INIT_ADC_DECL(f, DDRF, PORTF, 3)

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
#define UART_RING_SIZE 64
static ring_t *uart_ring;

static void uart_task(void *arg)
{
	buf_t buf;
	int rlen = ring_len(uart_ring);

	if (rlen > UART_RING_SIZE) {
		ring_reset(uart_ring);
		return;
	}
	buf= BUF(rlen + 1);
	__ring_get(uart_ring, &buf, rlen);
	__buf_addc(&buf, '\0');
	alarm_parse_uart_commands(&buf);
}

ISR(USART0_RX_vect)
{
	uint8_t c = UDR0;

	if (c == '\r')
		return;
	if (c == '\n') {
		schedule_task(uart_task, NULL);
	} else
		ring_addc(uart_ring, c);
}
#endif

static void blink_led(void *arg)
{
	tim_t *tim = arg;

	PORTB ^= (1 << PB7);
	timer_reschedule(tim, 10000000UL);
}

int main(void)
{
	tim_t timer_led;

	init_stream0(&stdout, &stdin, 1);
#ifdef DEBUG
	DEBUG_LOG("KW alarm v0.2\n");
#endif
#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
	uart_ring = ring_create(UART_RING_SIZE);
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
	timer_init(&timer_led);
	timer_add(&timer_led, 0, blink_led, &timer_led);


	watchdog_enable();
	sei();

#ifdef CONFIG_NETWORKING
	alarm_network_init();
#endif
#ifdef CONFIG_GSM_SIM900
	alarm_gsm_init();
#endif
#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
	alarm_rf_init();
	module_init();
#endif

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
