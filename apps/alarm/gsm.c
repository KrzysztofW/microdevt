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

#include <avr/interrupt.h>
#include <_stdio.h>
#include <sys/buf.h>
#include <sys/timer.h>
#include <usart.h>
#include <drivers/gsm-at.h>

static FILE *gsm_in, *gsm_out;
static tim_t timer = TIMER_INIT(timer);

ISR(USART1_RX_vect)
{
	gsm_handle_interrupt(UDR1);
}

static void gsm_cb(uint8_t status, const sbuf_t *from, const sbuf_t *msg)
{
	DEBUG_LOG("gsm callback (status:%d)\n", status);

	switch (status) {
	case GSM_STATUS_RECV:
		DEBUG_LOG("from:");
		sbuf_print(from);
		DEBUG_LOG(" msg:<");
		sbuf_print(msg);
		DEBUG_LOG(">\n");
		break;
	case GSM_STATUS_READY:
	case GSM_STATUS_TIMEOUT:
	case GSM_STATUS_ERROR:
		break;
	}
}

static void gsm_tim_cb(void *arg)
{
	gsm_send_sms("+33687236420", "SMS from KW alarm");
}

void alarm_gsm_init(void)
{
	init_stream1(&gsm_in, &gsm_out, 1);
	gsm_init(gsm_in, gsm_out, gsm_cb);
	timer_add(&timer, 30000000, gsm_tim_cb, NULL);
}
