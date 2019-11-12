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

#include <avr/io.h>
#include "usart.h"

void usart0_init(uint16_t baud, uint8_t interrupt)
{
	unsigned i = interrupt ? _BV(RXCIE0) : 0;

	UBRR0 = baud;
	/* Asynchronous mode, no parity, 1-stop bit, 8-bit data */
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	/* no 2x mode, no multi-processor mode */
	UCSR0A = 0x00;

	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | i;
}

void usart0_put(uint8_t b)
{
	/* wait for data register to be ready */
	while ((UCSR0A & _BV(UDRE0)) == 0) {}
	/* load b for transmission */
	UDR0 = b;
}

int usart0_get(uint8_t *c)
{
	/* poll for available data */
	if ((UCSR0A & _BV(RXC0)) == 0)
		return -1;
	*c = UDR0;
	return 0;
}

#ifdef CONFIG_USART1
void usart1_init(uint16_t baud, uint8_t interrupt)
{
	unsigned i = interrupt ? _BV(RXCIE1) : 0;

	UBRR1 = baud;
	/* Asynchronous mode, no parity, 1-stop bit, 8-bit data */
	UCSR1C = _BV(UCSZ01) | _BV(UCSZ00);

	/* no 2x mode, no multi-processor mode */
	UCSR1A = 0x00;

	UCSR1B = _BV(RXEN1) | _BV(TXEN1) | i;
}

void usart1_put(uint8_t b)
{
	/* wait for data register to be ready */
	while ((UCSR1A & _BV(UDRE1)) == 0) {}
	/* load b for transmission */
	UDR1 = b;
}

int usart1_get(uint8_t *c)
{
	/* poll for available data */
	if ((UCSR1A & _BV(RXC1)) == 0)
		return -1;
	*c = UDR1;
	return 0;
}

#endif
