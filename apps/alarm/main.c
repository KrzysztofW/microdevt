#include <log.h>
#include <_stdio.h>
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <timer.h>
#include <scheduler.h>
#include <net/pkt-mempool.h>
#include <interrupts.h>
#include "alarm.h"
#include "module-common.h"

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
#define UART_RING_SIZE 32
static ring_t *uart_ring;

static void uart_task(void *arg)
{
	buf_t buf = BUF(UART_RING_SIZE);
	uint8_t c;

	while (ring_getc(uart_ring, &c) >= 0) {
		if (buf_addc(&buf, c) < 0)
			__abort();
		if  (c == '\0')
			break;
	}
	if (buf.len)
		alarm_parse_uart_commands(&buf);
}

ISR(USART0_RX_vect)
{
	uint8_t c = UDR0;

	if (c == '\r')
		return;
	if (c == '\n') {
		c = '\0';
		schedule_task(uart_task, NULL);
	}
	if (ring_addc(uart_ring, c) < 0)
		ring_reset(uart_ring);
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
	ADC_SET_PRESCALER_64();
	ADC_SET_REF_VOLTAGE_INTERNAL();

	timer_subsystem_init();
	irq_enable();

#ifdef CONFIG_TIMER_CHECKS
	watchdog_shutdown();
	timer_checks();
#endif
#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER	\
	|| defined CONFIG_NETWORKING
	pkt_mempool_init();
#endif
	scheduler_init();
	timer_init(&timer_led);
	timer_add(&timer_led, 0, blink_led, &timer_led);


	watchdog_enable(WATCHDOG_TIMEOUT_8S);

#ifdef CONFIG_NETWORKING
	alarm_network_init();
#endif
#ifdef CONFIG_GSM_SIM900
	alarm_gsm_init();
#endif

#ifdef CONFIG_RF_RECEIVER
#endif

#ifdef CONFIG_RF_SENDER
	/* port F used by the RF sender */
	DDRF = (1 << PF1);
#ifdef CONFIG_RF_RECEIVER
	master_module_init();
#endif
#endif

	/* interruptible functions */
	while (1) {
#ifdef CONFIG_NETWORKING
		alarm_network_loop();
#endif
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
