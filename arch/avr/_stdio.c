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

#include "usart.h"
#include "stdio.h"

#define SYSTEM_CLOCK F_CPU

#ifdef CONFIG_USART0
static int putchar0(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r') {
		usart0_put('\r');
		usart0_put('\n');
	}
	usart0_put(c);
	return 0;
}

static int getchar0(FILE *stream)
{
	uint8_t c;

	(void)stream;
	while (usart0_get(&c) < -1) {}
	return c;
}

static FILE stream0 =
	FDEV_SETUP_STREAM(putchar0, getchar0, _FDEV_SETUP_RW);

void init_stream0(FILE **out_fd, FILE **in_fd, uint8_t interrupt)
{
	*out_fd = &stream0;
	*in_fd = &stream0;
	usart0_init(BAUD_RATE(SYSTEM_CLOCK, CONFIG_USART0_SPEED), interrupt);
}

#endif

#ifdef CONFIG_USART1
static int putchar1(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r') {
		usart1_put('\r');
		usart1_put('\n');
	}
	usart1_put(c);
	return 0;
}

static int getchar1(FILE *stream)
{
	uint8_t c;

	(void)stream;
	while (usart1_get(&c) < -1) {}
	return c;
}

static FILE stream1 =
	FDEV_SETUP_STREAM(putchar1, getchar1, _FDEV_SETUP_RW);

void init_stream1(FILE **out_fd, FILE **in_fd, uint8_t interrupt)
{
	/* initialize the standard streams to the user defined one */
	*out_fd = &stream1;
	*in_fd = &stream1;
	usart1_init(BAUD_RATE(SYSTEM_CLOCK, CONFIG_USART1_SPEED), interrupt);
}

#endif
