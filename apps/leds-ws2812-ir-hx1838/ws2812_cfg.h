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

#ifndef _WS2812_CFG_H_
#define _WS2812_CFG_H_

#define WS2812_RESET_DELAY 50
#ifdef ATTINY85
#define WS2812_PIN PB1
#define WS2812_PORT PORTB
#define WS2812_DDR DDRB
#else
#define WS2812_PIN PD5
#define WS2812_PORT PORTD
#define WS2812_DDR DDRD
#endif

#endif
