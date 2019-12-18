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

#ifndef _AVR_POWER_MANAGEMENT_H_
#define _AVR_POWER_MANAGEMENT_H_
#include <avr/sleep.h>
#include <sys/power-management.h>
#include "common.h"

#define PWR_MGR_SLEEP_MODE_IDLE SLEEP_MODE_IDLE
#define PWR_MGR_SLEEP_MODE_ADC SLEEP_MODE_ADC
#define PWR_MGR_SLEEP_MODE_PWR_DOWN SLEEP_MODE_PWR_DOWN
#define PWR_MGR_SLEEP_MODE_SAVE SLEEP_MODE_PWR_SAVE
#define PWR_MGR_SLEEP_MODE_STANDBY SLEEP_MODE_STANDBY
#define PWR_MGR_SLEEP_MODE_EXT_STANDY SLEEP_MODE_EXT_STANDBY

/* disable brown-out detector */
static inline void power_management_disable_bod(void)
{
#ifdef ATMEGA328P
	MCUCR |= _BV(BODS) | _BV(BODSE);
	MCUCR &= ~_BV(BODSE);
#endif
/* ATMEGA2561 unsupported */
}

#define power_management_set_mode(mode) set_sleep_mode(mode)
#define power_management_sleep() sleep_mode()

#endif
