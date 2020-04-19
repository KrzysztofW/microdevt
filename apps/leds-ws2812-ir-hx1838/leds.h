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

#ifndef _LEDS_H_
#define _LEDS_H_

typedef enum color {
	COLOR_WHITE,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_LAST,
} color_t;

typedef enum effect {
	EFFECT_STOP,
	EFFECT_FADE,
	EFFECT_SNAKE,
	EFFECT_COLOR_SNAKE,
	EFFECT_XMAS_TREE,
	EFFECT_LAST,
} effect_t;

void leds_effect(uint8_t effect);
void leds_snake_inc_len(uint8_t inc);
void leds_effect_inc_speed(uint8_t inc);
void leds_set(uint8_t brightness, uint8_t color);
void leds_set_brightness(uint8_t brightness);
void leds_set_color(uint8_t color);
void leds_stop(void);
void leds_start(void);
void leds_pause(void);
void leds_resume(void);
void leds_pause_resume_toggle(void);
uint8_t leds_is_runing(void);
#endif
