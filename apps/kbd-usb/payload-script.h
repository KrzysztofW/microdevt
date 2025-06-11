/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2024, Krzysztof Witek
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

#ifndef _PAYLOAD_SCRIPT_H_
#define _PAYLOAD_SCRIPT_H_
#include "inttypes.h"

#define CUSTOM_CMD                       0xFB

#define CUSTOM_CMD_DELAY                 0x01
#define CUSTOM_CMD_JITTER_ON             0x02
#define CUSTOM_CMD_JITTER_OFF            0x03
#define CUSTOM_CMD_WAIT_FOR_CAPS_ON      0x04
#define CUSTOM_CMD_WAIT_FOR_CAPS_OFF     0x05
#define CUSTOM_CMD_WAIT_FOR_CAPS_CHNG    0x06
#define CUSTOM_CMD_WAIT_FOR_NUMLOCK_ON   0x07
#define CUSTOM_CMD_WAIT_FOR_NUMLOCK_OFF  0x08
#define CUSTOM_CMD_WAIT_FOR_NUMLOCK_CHNG 0x09
#define CUSTOM_CMD_WAIT_FOR_BNT          0x0A
#define CUSTOM_CMD_LED_ON                0x0B
#define CUSTOM_CMD_LED_OFF               0x0C
#define CUSTOM_CMD_INJECT_MODE           0x0D

#define SERIAL_DATA_START_SEQ        0xFD
#define SERIAL_DATA_END_SEQ          0xFE
#define SERIAL_DATA_OK               0x59 /* Y */
#define SERIAL_DATA_NOK              0x4E /* N */
#define SERIAL_DATA_TOO_LONG         0x4C /* L */
#define SERIAL_DATA_INVALID          0x49 /* I */
#define SERIAL_DATA_STORAGE_ERROR    0x45 /* E */

#define PAYLOAD_MAX_ID 3
typedef struct __attribute__((__packed__)) script_hdr {
	uint32_t magic;
	uint16_t size;
	uint8_t  data[];
} script_hdr_t;

#define PAYLOAD_DATA_LENGTH CONFIG_EEPROM_SIZE
#define PAYLOAD_DATA_MAGIC  0x4FC2A7E3
#endif
