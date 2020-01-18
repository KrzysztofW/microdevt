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

#ifndef _RF_COMMON_H_
#define _RF_COMMON_H_

/* Modules' addresses */
#define RF_MOD0_HW_ADDR 0x69
#define RF_MOD1_HW_ADDR 0x70
#define RF_MOD2_HW_ADDR 0x71
#define RF_MOD3_HW_ADDR 0x72
#define RF_MOD4_HW_ADDR 0x73
#define RF_MOD5_HW_ADDR 0x74
#define RF_MOD6_HW_ADDR 0x75
#define RF_MOD7_HW_ADDR 0x76
#define RF_MOD8_HW_ADDR 0x77
#define RF_MOD9_HW_ADDR 0x78

/* Command types */
#define RF_CMD_PIR         0 /* move detector triggered */
#define RF_CMD_OC          1 /* open/close detector triggered */
#define RF_CMD_SH          2 /* shake detector triggered */
#define RF_CMD_PWR         3 /* power supply disconnected */
#define RF_CMD_PWR_BAT     4 /* low battery level cmd */
#define RF_CMD_USER1       5 /* generic cmd */
#define RF_CMD_USER2       6 /* generic cmd */
#define RF_CMD_USER3       7 /* generic cmd */
#define RF_CMD_USER4       8 /* generic cmd */
#define RF_CMD_USER5       9 /* generic cmd */
#define RF_CMD_USER6      10 /* generic cmd */

#endif
