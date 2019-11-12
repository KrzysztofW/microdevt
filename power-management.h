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

#ifndef _POWER_MANAGEMENT_H_
#define _POWER_MANAGEMENT_H_
#include <power-management.h>

extern uint16_t power_management_inactivity;

#define power_management_pwr_down_reset() power_management_inactivity = 0
#define power_management_pwr_down_set_idle() power_management_inactivity++
#define power_management_set_inactivity(value)	\
	power_management_inactivity = value

void
power_management_power_down_init(uint16_t inactivity_timeout,
				 void (*on_sleep)(void *arg), void *arg);
void power_management_power_down_enable(void);
void power_management_power_down_disable(void);

#endif
