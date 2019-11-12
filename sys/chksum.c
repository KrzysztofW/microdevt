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

#include <stdint.h>
#include "chksum.h"

uint32_t cksum_partial(const void *data, uint16_t len)
{
	const uint16_t *w = data;
	uint32_t sum = 0;

	while (len > 1)  {
		sum += *w++;
		len -= 2;
	}

	if (len == 1)
		sum += *(uint8_t *)w;

	return sum;
}

uint16_t cksum_finish(uint32_t csum)
{
	csum = (csum >> 16) + (csum & 0xffff);
	csum += csum >> 16;

	return ~csum;
}

uint16_t cksum(const void *data, uint16_t len)
{
	return cksum_finish(cksum_partial(data, len));
}
