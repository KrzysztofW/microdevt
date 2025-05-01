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

#define PAYLOAD_MAX_ID 3
typedef struct __attribute__((__packed__)) script {
	uint32_t magic;
	uint16_t size;
	uint16_t checksum;
	uint8_t id;
	uint8_t data[];
} script_t;

#define PAYLOAD_DATA_LENGTH 161
#define PAYLOAD_DATA_MAGIC 0x4FC2A7E3
#endif
