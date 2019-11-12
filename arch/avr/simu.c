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

/* simulavr/src/simulavr  -f <elf> -d atmega328 -F 16000000 */
#include <sys/utils.h>
#define SIMU_INFO_FILE src/simulavr_info.h
#define incl_header(x) __STR(x)
#include incl_header(CONFIG_AVR_SIMU_PATH/SIMU_INFO_FILE)

SIMINFO_DEVICE(__STR(CONFIG_AVR_SIMU_MCU));
SIMINFO_CPUFREQUENCY(F_CPU);
SIMINFO_SERIAL_IN("D0", "-", CONFIG_USART0_SPEED);
SIMINFO_SERIAL_OUT("D1", "-", CONFIG_USART0_SPEED);

