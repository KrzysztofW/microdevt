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

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <interrupts.h>
#include <ws2812.h>
#include <log.h>
#include "leds.h"

#define NB_LEDS 60
#define LEDS_MULTIPLY 5

#if defined(ATTINY85) && NB_LEDS > 80
#pragma message("\n\nAttiny85 does not have enough SRAM to handle more than" \
		" ~80 LEDS\n")
#undef NB_LEDS
#define NB_LEDS 80
#endif

#define NB_COLORS ((NB_LEDS * LEDS_MULTIPLY) / 2)
#define FADE (256 / NB_COLORS)
#define LEDS_EFFECT_DELAY 50000
#define LEDS_EFFECT_DELAY_MAX 10000000
#define LEDS_EFFECT_DELAY_STEP 10000

static rgb_t colors[] = {
	{ .r = 255, .g = 255, .b = 255, },

	/* green */
	{ .r = 0, .g = 255, .b = 0, },

	/* red */
	{ .r = 255, .g = 0, .b = 0, },

	/* orange */
	{ .r = 255, .g = 100, .b = 0, },

	/* yellow */
	{ .r = 100, .g = 255, .b = 0, },

	/* light blue */
	{ .r = 0, .g = 100, .b = 255, },

	 /* blue */
	{ .r = 0, .g = 0, .b = 255, },

	/* purple */
	{ .r = 100, .g = 0, .b = 255, },
};

static rgb_t leds[NB_LEDS];
static tim_t leds_timer = TIMER_INIT(leds_timer);
static uint8_t cur_color = COLOR_WHITE;
static uint8_t cur_brightness = 255;
static void (*task_cb)(void);
static unsigned long task_delay = LEDS_EFFECT_DELAY;
static unsigned long task_delay_saved;

static uint16_t snake_head;
static uint16_t snake_length = MIN(5, (NB_LEDS / 4) + 1);
extern uint8_t ir_runing;

static void leds_task_cb(void *arg)
{
	timer_interrupt_disable();
	ws2812_send_rgb_multi(leds, NB_LEDS, LEDS_MULTIPLY);
	timer_interrupt_enable();

	if (arg && task_cb)
		task_cb();
}

static void leds_timer_cb(void *arg)
{
	if (ir_runing) {
		timer_add(&leds_timer, task_delay, leds_timer_cb, task_cb);
		return;
	}
	schedule_task(leds_task_cb, arg);

}

static void leds_arm_timer(void)
{
	if (timer_is_pending(&leds_timer))
		return;

	timer_add(&leds_timer, task_delay, leds_timer_cb, task_cb);
}

static void __led_set(uint16_t led, uint16_t brightness, uint8_t color)
{
	switch (color) {
	case COLOR_RED:
		leds[led].r = brightness;
		leds[led].g = 0;
		leds[led].b = 0;
		break;
	case COLOR_GREEN:
		leds[led].r = 0;
		leds[led].g = brightness;
		leds[led].b = 0;
		break;
	case COLOR_YELLOW:
		leds[led].r = brightness;
		leds[led].g = brightness;
		leds[led].b = 0;
		break;
	case COLOR_BLUE:
		leds[led].r = 0;
		leds[led].g = 0;
		leds[led].b = brightness;
		break;
	case COLOR_MAGENTA:
		leds[led].r = brightness;
		leds[led].g = 0;
		leds[led].b = brightness;
		break;
	case COLOR_CYAN:
		leds[led].r = 0;
		leds[led].g = brightness;
		leds[led].b = brightness;
		break;
	case COLOR_WHITE:
		leds[led].r = brightness;
		leds[led].g = brightness;
		leds[led].b = brightness;
		break;
	}
}

static void __leds_set(uint16_t brightness, uint8_t color)
{
	uint16_t i;

	for (i = 0; i < NB_LEDS; i++)
		__led_set(i, brightness, color);
}

void leds_set(uint8_t brightness, uint8_t color)
{
	timer_del(&leds_timer);
	__leds_set(brightness, color);
	timer_add(&leds_timer, 0, leds_timer_cb, NULL);
}

void leds_set_color(uint8_t color)
{
	cur_color = color;
}

void leds_set_brightness(uint8_t brightness)
{
	cur_brightness = brightness;
}

void leds_start(void)
{
	leds_set(cur_brightness, cur_color);
}

void leds_pause(void)
{
	timer_del(&leds_timer);
}
void leds_resume(void)
{
	if (task_cb)
		timer_add(&leds_timer, task_delay, leds_timer_cb, task_cb);
}
void leds_pause_resume_toggle(void)
{
	if (timer_is_pending(&leds_timer)) {
		leds_pause();
		return;
	}
	leds_resume();
}
void leds_stop(void)
{
	timer_del(&leds_timer);
	task_cb = NULL;
	__leds_set(0, COLOR_WHITE);
	irq_disable();
	ws2812_send_rgb_multi(leds, NB_LEDS, LEDS_MULTIPLY);
	irq_enable();
}

static void __leds_snake(uint8_t change_colors)
{
	uint16_t snake_tail;
	static uint8_t color_pos = COLOR_WHITE;

	/* turn the head on */
	__led_set(snake_head, cur_brightness, cur_color);

	if (snake_head < snake_length)
		snake_tail = NB_LEDS - (snake_length - snake_head);
	else
		snake_tail = snake_head - snake_length;

	/* turn the tail off */
	__led_set(snake_tail, 0, COLOR_WHITE);

	snake_head++;
	if (snake_head >= NB_LEDS) {
		snake_head = 0;
		if (change_colors) {
			cur_color = color_pos++;
			if (color_pos == COLOR_LAST)
				color_pos = COLOR_WHITE;
		}
	}
	leds_arm_timer();
}

static void leds_snake(void)
{
	return __leds_snake(0);
}

static void leds_color_snake(void)
{
	return __leds_snake(1);
}

void leds_snake_inc_len(uint8_t inc)
{
	if (task_cb != leds_snake && task_cb != leds_color_snake)
		return;

	if (inc) {
		if (snake_length <= NB_LEDS - 2)
			snake_length++;
		return;
	}
	if (snake_length > 1) {
		snake_length--;
		snake_head--;
		if (snake_head > NB_LEDS)
			snake_head = 0;
	}
}

void leds_effect_inc_speed(uint8_t inc)
{
	if (inc) {
		if (task_delay > LEDS_EFFECT_DELAY_STEP)
			task_delay -= LEDS_EFFECT_DELAY_STEP;
		else {
			task_delay /= 10;
			if (task_delay == 0)
				task_delay = 1;
		}
		return;
	}
	if (task_delay < LEDS_EFFECT_DELAY_STEP)
		task_delay *= 10;
	else if (task_delay < LEDS_EFFECT_DELAY_MAX)
		task_delay += LEDS_EFFECT_DELAY_STEP;
}

static void __leds_fade(void)
{
	uint16_t i = 0;
	static uint16_t j = 0;
	static uint16_t k = 0;

	for (i = 0; i < NB_LEDS - 1; i++)
		leds[i+1] = leds[i];

	if (k > NB_COLORS) {
		j++;
		if (j > sizeof(colors) / sizeof(rgb_t))
			j = 0;
		k = 0;
	}
	k++;

	if (leds[0].r < (colors[j].r - FADE))
		leds[0].r = MIN((leds[0].r + FADE), cur_brightness);

	if (leds[0].r > (colors[j].r + FADE))
		leds[0].r -= FADE;

	if (leds[0].g < (colors[j].g - FADE))
		leds[0].g = MIN((leds[0].g + FADE), cur_brightness);

	if (leds[0].g > (colors[j].g + FADE))
		leds[0].g -= FADE;

	if (leds[0].b < (colors[j].b - FADE))
		leds[0].b = MIN((leds[0].b + FADE), cur_brightness);

	if (leds[0].b > (colors[j].b + FADE))
		leds[0].b -= FADE;
	leds_arm_timer();
}

static void __leds_christmas_tree(void)
{
	uint16_t i;

	for (i = 0; i < NB_LEDS; i++) {
		leds[i].r = rand() % cur_brightness;
		leds[i].g = rand() % cur_brightness;
		leds[i].b = rand() % cur_brightness;
	}

	/* execute as fast as possible */
	leds_arm_timer();
}

void leds_effect(uint8_t effect)
{
	if (task_delay_saved) {
		task_delay = task_delay_saved;
		task_delay_saved = 0;
	}
	leds_stop();

	switch (effect) {
	case EFFECT_FADE:
		task_cb = __leds_fade;
		break;
	case EFFECT_SNAKE:
		task_cb = leds_snake;
		break;
	case EFFECT_COLOR_SNAKE:
		task_cb = leds_color_snake;
		break;
	case EFFECT_XMAS_TREE:
		task_delay_saved = task_delay;
		task_delay = LEDS_EFFECT_DELAY;
		task_cb = __leds_christmas_tree;
		break;

	default:
		return;
	}
	leds_arm_timer();
}

uint8_t leds_is_runing(void)
{
	return task_cb != NULL;
}
