/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <interrupts.h>
#include "alarm-module1.h"
#include "gpio.h"
#include "../module-common.h"

#ifdef DEBUG
#define UART_RING_SIZE 16
STATIC_RING_DECL(uart_ring, UART_RING_SIZE);

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
	DEBUG_LOG("KW alarm module 1 (%s)\n", VERSION);
#ifndef CONFIG_AVR_SIMU
	DEBUG_LOG("MCUSR:%X\n", MCUSR);
	MCUSR = 0;
#endif
#endif
	timer_subsystem_init();
	irq_enable();

#ifdef CONFIG_TIMER_CHECKS
#ifndef CONFIG_AVR_SIMU
	watchdog_shutdown();
#endif
	timer_checks();
#endif

#ifndef CONFIG_AVR_SIMU
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
#endif
	gpio_init();

	pkt_mempool_init();
	module1_init();

	/* interruptible functions */
	while (1) {
		scheduler_run_task();
#ifndef CONFIG_AVR_SIMU
		watchdog_reset();
#endif
	}
	return 0;
}
