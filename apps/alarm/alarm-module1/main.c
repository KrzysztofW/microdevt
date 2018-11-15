#include <avr/sleep.h>
#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <timer.h>
#include <scheduler.h>
#include <interrupts.h>
#include "version.h"
#include "alarm-module1.h"
#include "gpio.h"
#include "../module-common.h"

#ifdef DEBUG
#define UART_RING_SIZE 16
STATIC_RING_DECL(uart_ring, UART_RING_SIZE);
#endif

#ifdef DEBUG
static void uart_task(void *arg)
{
	buf_t buf;
	int rlen = ring_len(uart_ring);

	if (rlen > UART_RING_SIZE) {
		ring_reset(uart_ring);
		return;
	}
	buf = BUF(rlen + 1);
	__ring_get(uart_ring, &buf, rlen);
	__buf_addc(&buf, '\0');
	module1_parse_uart_commands(&buf);
}

ISR(USART_RX_vect)
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

int main(void)
{
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 1);
	DEBUG_LOG("KW alarm module 1 (%s)\n", revision);
#endif
	timer_subsystem_init();
	irq_enable();

#ifdef CONFIG_TIMER_CHECKS
	watchdog_shutdown();
	timer_checks();
#endif
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
	gpio_init();

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
	pkt_mempool_init();
	module1_init();
#endif

	/* interruptible functions */
	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
