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

/*
 * This simple application reads commands from the serial console and prints out
 * the relative humidity and temperature.
 * The humidity and temperature values are read every 10 seconds using a timer.
 * In case of inactivity, the app goes to sleep (power down mode).
 * In addition, the scheduler will make sure that if there are no pending tasks
 * the microcontroller will go to the IDLE power save mode.
 * A timer makes an LED bink every second and a PIR sensor wakes
 * the app up (see gpio.h file for PIN assignments).
 *
 * The app shows the usage of the serial console, the timers, the tasks,
 * the watchdog, the power management and the input/output GPIOs of
 * an ATMEGA328p microcontroller.
 */

#include <log.h>
#include <_stdio.h>
#include <watchdog.h>
#include <sys/buf.h>
#include <sys/ring.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <interrupts.h>
#include <drivers/sensors.h>
#include <power-management.h>
#include "gpio.h"

#define INACTIVITY_TIMEOUT 60 /* enter idle sleep mode after 60 */
#define ONE_SECOND 1000000
#define SENSOR_SAMPLE 30 /* sample every 30 seconds */
static uint16_t sensor_report_elapsed_secs;
static tim_t one_sec_timer = TIMER_INIT(one_sec_timer);
static uint16_t humidity_val;
static uint16_t temp_val;

#define UART_RING_SIZE 16
STATIC_RING_DECL(uart_ring, UART_RING_SIZE);

static const char *cmd1 = "show sensors";

static void usage(void)
{
	LOG("Available commands:\n");
	LOG(" %s\n\n", cmd1);
}

static void parse_serial_console(buf_t *buf)
{
	sbuf_t s;

	if (buf_get_sbuf_upto_and_skip(buf, &s, cmd1) >= 0) {
		LOG("humidity: %u%%, temp: %u\n", humidity_val, temp_val);
		return;
	}
	usage();
}

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
	parse_serial_console(&buf);
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
}

ISR(USART_RX_vect)
{
	uint8_t c = UDR0;

	if (c == '\r')
		return;
	if (c == '\n')
		schedule_task(uart_task, NULL);
	else
		ring_addc(uart_ring, c);
}

static void pir_interrupt_cb(void *arg)
{
	LOG("PIR sensor on\n");
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
}

ISR(PCINT2_vect)
{
	schedule_task(pir_interrupt_cb, NULL);
}

static void sensor_sampling_task(void *arg)
{
	adc_enable();
	humidity_val = adc_read(HUMIDITY_ANALOG_PIN);
	temp_val = adc_read(TEMPERATURE_ANALOG_PIN);

	humidity_val = hih_4000_to_rh(adc_to_millivolt(humidity_val));
	temp_val = TMP36GZ_TO_C_CENTI_DEGREES(adc_to_millivolt(temp_val));
}

static void sensor_sample(void)
{
	if (sensor_report_elapsed_secs >= SENSOR_SAMPLE) {
		sensor_report_elapsed_secs = 0;
		schedule_task(sensor_sampling_task, NULL);
	}
}
static void timer_one_sec_cb(void *arg)
{
	gpio_led_toggle();
	sensor_report_elapsed_secs++;
	sensor_sample();
	timer_reschedule(arg, ONE_SECOND);
}

#ifdef CONFIG_POWER_MANAGEMENT
static void watchdog_task(void *arg)
{
	/* wait before using the USART to stabilize */
	delay_ms(100);

	LOG("woke up!\n");
}

static void watchdog_on_wakeup(void *arg)
{
	sensor_report_elapsed_secs += 8 + 2; /* 8 secs of WD + 2 secs below */
	sensor_sample();
	schedule_task(watchdog_task, arg);

	/* stay active for 2 seconds (if needed in a project) */
	power_management_set_inactivity(INACTIVITY_TIMEOUT - 2);
}

static void pwr_mgr_on_sleep(void *arg)
{
	LOG("going to sleep...\n");
	gpio_led_off();

	/* use the watchdog interrupt to wake the uC up */
	watchdog_enable_interrupt(watchdog_on_wakeup, arg);
}
#endif

int main(void)
{
	init_stream0(&stdout, &stdin, 1);
	LOG("Hello World! version: (%s)\n", VERSION);

	timer_subsystem_init();
	irq_enable();

#ifdef CONFIG_POWER_MANAGEMENT
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
	power_management_power_down_init(INACTIVITY_TIMEOUT, pwr_mgr_on_sleep,
					 NULL);
#endif
	gpio_init();
	timer_add(&one_sec_timer, ONE_SECOND, timer_one_sec_cb, &one_sec_timer);

	/* interruptible functions */
	while (1) {
		scheduler_run_task();
		watchdog_reset();
	}
	return 0;
}
