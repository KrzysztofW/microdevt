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

#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define WATCHDOG_TIMEOUT_15MS WDTO_15Ms
#define WATCHDOG_TIMEOUT_30MS WDTO_30MS
#define WATCHDOG_TIMEOUT_60MS WDTO_60MS
#define WATCHDOG_TIMEOUT_120MS WDTO_120MS
#define WATCHDOG_TIMEOUT_250MS WDTO_250MS
#define WATCHDOG_TIMEOUT_500MS WDTO_500MS
#define WATCHDOG_TIMEOUT_1S WDTO_1S
#define WATCHDOG_TIMEOUT_2S WDTO_2S
#define WATCHDOG_TIMEOUT_4S WDTO_4S
#define WATCHDOG_TIMEOUT_8S WDTO_8S

void watchdog_enable_interrupt(void (*cb)(void *arg), void *arg);

static inline void watchdog_enable_reset(void)
{
	WDTCSR |= _BV(WDE);
}

static inline void watchdog_disable_reset(void)
{
	WDTCSR &= ~(_BV(WDE));
}

static inline void watchdog_shutdown(void)
{
	wdt_reset();
	MCUSR = 0;
	WDTCSR |= _BV(WDCE);
	WDTCSR = 0;
}

static inline void watchdog_enable(uint8_t timeout)
{
	wdt_enable(timeout);
}

static inline void watchdog_reset(void)
{
	wdt_reset();
}

#endif
