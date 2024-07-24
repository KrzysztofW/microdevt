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

#ifndef _GPIO_H_
#define _GPIO_H_

#include "rf-cfg.h"

#define LED_PIN PD2
#define LED_PORT PORTD
#define LED_DDR DDRD

static inline void gpio_init(void)
{
	/* PORTD receiver (PD5), LED (PD2) */
	PORTD = 0xFF & ~((1 << RF_RCV_PIN_NB) | (1 << LED_PIN));

	/* LED output, PB4 input */
	DDRD = 1 << LED_PIN;
}

static inline void gpio_led_toggle(void)
{
	LED_PORT ^= 1 << LED_PIN;
}
#endif
