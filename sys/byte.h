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

#ifndef _BYTE_H_
#define _BYTE_H_
#include <log.h>

struct byte {
	uint8_t c;
	uint8_t pos;
} __PACKED__;
typedef struct byte byte_t;

static inline void byte_reset(byte_t *byte)
{
	byte->pos = 0;
}

static inline void byte_init(byte_t *byte, uint8_t c)
{
	byte->c = c;
	byte->pos = 8;
}

static inline uint8_t byte_is_empty(const byte_t *byte)
{
	return byte->pos == 0;
}

static inline uint8_t byte_is_set(const byte_t *byte)
{
	return byte->pos == 8;
}

static inline int byte_add_bit(byte_t *byte, uint8_t bit)
{
	byte->c = (byte->c << 1) | bit;
	byte->pos++;

	if (byte->pos >= 8) {
		byte_reset(byte);
		return byte->c;
	}
	return -1;
}

static inline uint8_t __byte_get_bit(byte_t *byte)
{
	uint8_t bit;

	bit = byte->c >> 7;
	byte->c <<= 1;
	byte->pos--;
	return bit;
}

static inline int byte_get_bit(byte_t *byte)
{
	if (byte->pos == 0)
		return -1;
	return __byte_get_bit(byte);
}

static inline void print_byte(const byte_t *byte)
{
	int j;

	for (j = 0; j < byte->pos; j++) {
		LOG("%s", ((byte->c  >> (7 - j)) & 1) ? "X" : " ");
	}
}

#endif
