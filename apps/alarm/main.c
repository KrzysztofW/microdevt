#include <log.h>
#include <_stdio.h>
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <timer.h>
#include <scheduler.h>
#include <net/pkt-mempool.h>
#include <interrupts.h>
#include "alarm.h"
#include "module-common.h"
#include "gpio.h"

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
#define UART_RING_SIZE 32
STATIC_RING_DECL(uart_ring, UART_RING_SIZE);

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

int main(void)
{
	init_stream0(&stdout, &stdin, 1);
#ifdef DEBUG
	DEBUG_LOG("KW alarm v0.2 (%s)\n", VERSION);
#ifndef CONFIG_AVR_SIMU
	DEBUG_LOG("MCUSR:%X\n", MCUSR);
	MCUSR = 0;
#endif
#endif
	ADC_SET_PRESCALER_64();
	ADC_SET_REF_VOLTAGE_INTERNAL();

	timer_subsystem_init();
	irq_enable();

#ifdef CONFIG_TIMER_CHECKS
	watchdog_shutdown();
	timer_checks();
#endif
	gpio_init();
	pkt_mempool_init();

#ifndef CONFIG_AVR_SIMU
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
#endif

#ifdef CONFIG_NETWORKING
	alarm_network_init();
#endif
#ifdef CONFIG_GSM_SIM900
	alarm_gsm_init();
#endif

	/* port F used by the RF sender */
	master_module_init();

	/* interruptible functions */
	while (1) {
#ifdef CONFIG_NETWORKING
		alarm_network_loop();
#endif
		scheduler_run_task();
#ifndef CONFIG_AVR_SIMU
		watchdog_reset();
#endif
	}
	return 0;
}
