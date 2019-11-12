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

#ifndef _GSM_AT_H_
#define _GSM_AT_H_

typedef enum gsm_status {
	GSM_STATUS_OK,
	GSM_STATUS_READY,
	GSM_STATUS_TIMEOUT,
	GSM_STATUS_ERROR,
	GSM_STATUS_RECV,
} gsm_status_t;

void
gsm_init(FILE *in, FILE *out, void (*cb)(uint8_t status, const sbuf_t *from,
					 const sbuf_t *msg));
int gsm_send_sms(const char *number, const char *sms);
void gsm_handle_interrupt(uint8_t c);
int gsm_tests(void);

#endif
