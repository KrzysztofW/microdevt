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

#ifndef _SWEN_H_
#define _SWEN_H_

#include "pkt-mempool.h"
#include "if.h"

enum generic_cmd_status {
	GENERIC_CMD_STATUS_OK,
	GENERIC_CMD_STATUS_RCV,
	GENERIC_CMD_STATUS_LIST,
	GENERIC_CMD_STATUS_ERROR_FULL,
	GENERIC_CMD_STATUS_ERROR_TIMEOUT,
	GENERIC_CMD_STATUS_ERROR_DUPLICATE,
	GENERIC_CMD_STATUS_ERROR_NOT_FOUND,
};

typedef struct __PACKED__ swen_hdr_t {
	uint8_t to;
	uint8_t from;
	uint8_t proto;
	uint16_t chksum;
} swen_hdr_t;

void swen_input(iface_t *iface);
int
swen_output(pkt_t *out, iface_t *iface, uint8_t type, const void *dst);

/** Send buffer to a peer
 *
 * @param[in] iface  interface
 * @param[in] to     destination address
 * @param[in] sbuf   static buffer to send
 */
int swen_sendto(iface_t *iface, uint8_t to, const sbuf_t *sbuf);

/** Initialize generic commands
 *
 * @param[in] cb  callback on reception
 */
void swen_generic_cmds_init(void (*cb)(uint16_t cmd, uint8_t status));

/** Start recording generic commands
 *
 * @param[in] value  command number to record
 */
void swen_generic_cmds_start_recording(int16_t value);

/** Set event callback
 *
 * @param[in] ev_cb  function to be called on wake up
 */
void swen_ev_set(void (*ev_cb)(uint8_t from, uint8_t events, buf_t *buf));

/** Get generic command list
 *
 * @param[out] buf  buffer
 */
void swen_generic_cmds_get_list(buf_t *buf);

/** Delete recorded generic command
 *
 * @param[in] number command number
 */
void swen_generic_cmds_delete_recorded_cmd(int number);

/** Delete all recorded commands
 */
static inline void swen_generic_cmds_delete_all_recorded_cmd(void)
{
	swen_generic_cmds_delete_recorded_cmd(-1);
}

void swen_generic_cmds_dump_storage(uint8_t check_eeprom);

extern void (*swen_event_cb)(uint8_t from, uint8_t events, buf_t *buf);
int swen_generic_cmds_check(iface_t *iface);

#endif
