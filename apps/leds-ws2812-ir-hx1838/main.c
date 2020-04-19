/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2020, Krzysztof Witek
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
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/opts.h>
#include <interrupts.h>
#include <power-management.h>
#include <net/event.h>
#include <net/swen.h>
#include <drivers/ir-hx1838.h>
#include <eeprom.h>
#include <ws2812.h>
#include "gpio.h"
#include "leds.h"

#define INACTIVITY_TIMEOUT 3600

#ifdef CONFIG_IR_CGV_REMOTE
#define IR_CGV_REMOTE
#else
#define IR_CAR_MP3_REMOTE
#endif

#ifdef IR_CAR_MP3_REMOTE
#define IR_CMD_EQ 0x90
#define IR_CMD_0 0x68
#define IR_CMD_1 0x30
#define IR_CMD_2 0x18
#define IR_CMD_3 0x7A
#define IR_CMD_4 0x10
#define IR_CMD_PLUS  0xA8
#define IR_CMD_MINUS 0xE0
#define IR_CMD_PLUS_100 0x98
#define IR_CMD_PLUS_200 0xB0
#define IR_CMD_SLOW 0x22
#define IR_CMD_FAST 0x02
#define IR_CMD_PAUSE_RESUME 0xC2
#endif
#ifdef IR_CGV_REMOTE
#define IR_CMD_EQ 0x48
#define IR_CMD_0 0x38
#define IR_CMD_1 0x90
#define IR_CMD_2 0xB8
#define IR_CMD_3 0xF8
#define IR_CMD_4 0xB0
#define IR_CMD_PLUS  0x58
#define IR_CMD_MINUS 0x70
#define IR_CMD_PLUS_100 0x40
#define IR_CMD_PLUS_200 0x60
#define IR_CMD_SLOW 0x62
#define IR_CMD_FAST 0xE2
#define IR_CMD_PAUSE_RESUME 0x10

#define IR_CMD_RED 0x42
#define IR_CMD_GREEN 0x02
#define IR_CMD_YELLOW 0x80
#define IR_CMD_BLUE 0xC0
#endif

uint8_t ir_runing;
static uint8_t color;
static int16_t brightness = 155;

static tim_t ir_finished_timer = TIMER_INIT(ir_finished_timer);
#define IR_FINISHED 50000

#if defined(DEBUG) && defined(ATMEGA328P)
static tim_t one_sec_timer = TIMER_INIT(one_sec_timer);
#define ONE_SECOND 1000000
static uint8_t effect;
#define UART_RING_SIZE 16
STATIC_RING_DECL(uart_ring, UART_RING_SIZE);

static sbuf_t s_help = SBUF_INITS("help");
static sbuf_t s_color = SBUF_INITS("color");
static sbuf_t s_brightness = SBUF_INITS("brightness");
static sbuf_t s_effect = SBUF_INITS("effect");
static sbuf_t s_effect_speed = SBUF_INITS("speed inc");
static sbuf_t s_stop = SBUF_INITS("stop");

typedef enum command {
	CMD_HELP,
	CMD_COLOR,
	CMD_BRIGHTNESS,
	CMD_EFFECT,
	CMD_EFFECT_SPEED,
	CMD_SNAKE_LENGTH,
	CMD_STOP,
} command_t;

static cmd_t cmds[] = {
	{ .s = &s_help, .args = { ARG_TYPE_NONE }, .cmd = CMD_HELP },
	{ .s = &s_color, .args = { ARG_TYPE_NONE }, .cmd = CMD_COLOR, },
	{ .s = &s_brightness, .args = { ARG_TYPE_UINT8, ARG_TYPE_NONE },
	  .cmd = CMD_BRIGHTNESS, },
	{ .s = &s_effect, .args = { ARG_TYPE_NONE }, .cmd = CMD_EFFECT, },
	{ .s = &s_effect_speed, .args = { ARG_TYPE_BOOL, ARG_TYPE_NONE },
	  .cmd = CMD_EFFECT_SPEED, },
	{ .s = &s_stop, .args = { ARG_TYPE_NONE }, .cmd = CMD_STOP, },
};

static void ir_cb(uint8_t cmd, uint8_t repeated);
static void handle_commands(uint8_t cmd, buf_t *args)
{
	switch (cmd) {
	case CMD_HELP:
		opts_print_usage(cmds, countof(cmds));
		break;
	case CMD_COLOR:
		ir_cb(IR_CMD_0, 0);
		break;
	case CMD_BRIGHTNESS:
		brightness = args->data[0];
		leds_set_brightness(brightness);
		break;
	case CMD_EFFECT:
		leds_effect(effect);
		if (effect == EFFECT_STOP) {
			color = 0;
			brightness = 155;
		}
		effect = (effect + 1) % EFFECT_LAST;
		break;
	case CMD_EFFECT_SPEED:
		if (args->data[0])
			leds_effect_inc_speed(1);
		else
			leds_effect_inc_speed(0);
		break;
	case CMD_STOP:
		leds_stop();
		break;
	}
#ifdef ATMEGA328P
	DEBUG_LOG("color:%u brightness=%d effect:%u\n", color, brightness,
		  effect);
#endif
}

static void uart_task(void *arg)
{
	buf_t buf = BUF(UART_RING_SIZE);
	buf_t args = BUF(8);
	uint8_t c;

	while (ring_getc(uart_ring, &c) >= 0) {
		__buf_addc(&buf, c);
		if  (c == '\0')
			break;
	}
	if (buf.len > 1 &&
	    opts_parse_buf(cmds, countof(cmds), &buf, &args,
			   handle_commands) < 0)
		opts_print_usage(cmds, countof(cmds));
}

/* Note that as the ws2812_send() function may be long to execute
 * and it is run with interrupts disabled so there might be lost
 * characters in the serial console interrupt handler.
 */
ISR(USART_RX_vect)
{
	uint8_t c = UDR0;

	leds_pause();
	if (c == '\r')
		return;
	if (c == '\n') {
		c = '\0';
		schedule_task(uart_task, NULL);
		leds_resume();
	}
	if (ring_addc(uart_ring, c) < 0)
		ring_reset(uart_ring);
}
#endif	/* DEBUG */

#ifdef CONFIG_POWER_MANAGEMENT
static void watchdog_on_wakeup(void *arg) {}
static void pwr_mgr_on_sleep(void *arg)
{
	leds_stop();
	watchdog_enable_interrupt(watchdog_on_wakeup, arg);
}
#endif

static void ir_finished_cb(void *arg)
{
	ir_runing = 0;
}

static void ir_cb(uint8_t cmd, uint8_t is_repeated)
{
	int8_t m = 1;
	uint8_t e;

	ir_runing = 0;
#ifdef ATMEGA328P
	DEBUG_LOG("ir:0x%X (%u)\n", cmd, is_repeated);
#endif
	switch (cmd) {
#ifdef IR_CGV_REMOTE
	case IR_CMD_RED:
		leds_set_color(COLOR_RED);
		if (!leds_is_runing())
			leds_start();
		return;
	case IR_CMD_GREEN:
		leds_set_color(COLOR_GREEN);
		if (!leds_is_runing())
			leds_start();
		return;
	case IR_CMD_YELLOW:
		leds_set_color(COLOR_YELLOW);
		if (!leds_is_runing())
			leds_start();
		return;
	case IR_CMD_BLUE:
		leds_set_color(COLOR_BLUE);
		if (!leds_is_runing())
			leds_start();
		return;
#endif
	case IR_CMD_EQ:
		leds_set_color(color);
		color = (color + 1) % (COLOR_LAST);
		if (!leds_is_runing())
			leds_start();
		return;
	case IR_CMD_0:
		leds_stop();
		watchdog_enable_interrupt(watchdog_on_wakeup, NULL);
		return;
	case IR_CMD_1:
		e = EFFECT_FADE;
		break;
	case IR_CMD_2:
		e = EFFECT_SNAKE;
		break;
	case IR_CMD_3:
		e = EFFECT_COLOR_SNAKE;
		break;
	case IR_CMD_4:
		e = EFFECT_XMAS_TREE;
		break;
	case IR_CMD_MINUS:
		m = -1;
	case IR_CMD_PLUS:
		brightness += 5 * m;
		if (brightness > 255)
			brightness = 255;
		else if (brightness <= 5)
			brightness = 5;
		leds_set_brightness(brightness);
		if (!leds_is_runing())
			leds_start();
		return;
	case IR_CMD_PLUS_200:
		leds_snake_inc_len(1);
		return;
	case IR_CMD_PLUS_100:
		leds_snake_inc_len(0);
		return;
	case IR_CMD_FAST:
		leds_effect_inc_speed(1);
		return;
	case IR_CMD_SLOW:
		leds_effect_inc_speed(0);
		return;
	case IR_CMD_PAUSE_RESUME:
		leds_pause_resume_toggle();
		return;
	default:
		return;
	}
	if (is_repeated)
		return;
	leds_effect(e);
}

ISR(INT0_vect)
{
	power_management_pwr_down_reset();

	ir_runing = 1;
	WS2812_ABORT_TRANSFER();
	ir_falling_edge_interrupt_cb();
	if (!timer_is_pending(&ir_finished_timer))
		timer_reschedule(&ir_finished_timer, IR_FINISHED);
}

#if defined(DEBUG) && defined(ATMEGA328P)
static void one_sec_cb(void *arg)
{
	gpio_led_toggle();
	timer_reschedule(&one_sec_timer, ONE_SECOND);
}
#endif

int main(void)
{
	timer_subsystem_init();
#if defined(DEBUG) && defined(ATMEGA328P)
	init_stream0(&stdout, &stdin, 1);
	DEBUG_LOG("KW LEDS v0.1 (%s)\n", VERSION);
	timer_add(&one_sec_timer, ONE_SECOND, one_sec_cb, NULL);
#endif
	timer_add(&ir_finished_timer, IR_FINISHED, ir_finished_cb, NULL);
	gpio_init();
	irq_enable();

#ifndef CONFIG_AVR_SIMU
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
#endif
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_power_down_init(INACTIVITY_TIMEOUT, pwr_mgr_on_sleep,
					 NULL);
#endif
	ir_init(ir_cb);

	/* go to sleep directly and wait for commands */
	watchdog_enable_interrupt(watchdog_on_wakeup, NULL);

	/* interruptible functions */
	while (1) {
		scheduler_run_task();
#ifndef CONFIG_AVR_SIMU
		watchdog_reset();
#endif
	}
	return 0;
}
