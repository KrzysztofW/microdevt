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

#include <sys/utils.h>
#include <sys/chksum.h>
#include <sys/buf.h>
#include <sys/ring.h>
#include <drivers/rf.h>
#ifdef CONFIG_SWEN_ROLLING_CODES
#include "swen-rc.h"
#endif
#include "event.h"
#include "swen-l3.h"
#include "swen.h"
#ifdef CONFIG_RF_GENERIC_COMMANDS
#include <eeprom.h>

#define MAGIC 0xA7
typedef struct swen_generic_cmd_hdr {
	uint8_t length;
	int8_t  cmd;
	uint8_t data[];
} swen_generic_cmd_hdr_t;

typedef struct swen_generic_cmds_storage_hdr {
	uint8_t magic;
	swen_generic_cmd_hdr_t cmds[];
} swen_generic_cmds_storage_hdr_t;

#define SWEN_GENERIC_CMDS_RECORD_TIMEOUT 10 /* seconds */
static tim_t gcmd_timer = TIMER_INIT(gcmd_timer);

#define LOOP_STOP 0
#define LOOP_CONTINUE -1
/* #define CONSISTENCY_CHECK */

static uint8_t
#ifdef CONFIG_AVR_MCU
EEMEM
#endif
swen_generic_cmds_storage[CONFIG_RF_GENERIC_COMMANDS_SIZE];

static uint8_t swen_generic_cmds[CONFIG_RF_GENERIC_COMMANDS_SIZE];
static int16_t swen_generic_cmds_record_value = -1; /* -1 => unset */
static void (*swen_generic_cmds_cb)(uint16_t cmd, uint8_t status);

static unsigned swen_generic_cmds_addr_offset(void *addr)
{
	uint8_t *a = addr;

	assert(a >= swen_generic_cmds);
	return a - swen_generic_cmds;
}

#ifdef CONSISTENCY_CHECK
static void swen_generic_cmds_check_consistency(void)
{
	uint8_t data[CONFIG_RF_GENERIC_COMMANDS_SIZE];
	sbuf_t sbuf;

	eeprom_load(data, swen_generic_cmds_storage,
		    CONFIG_RF_GENERIC_COMMANDS_SIZE);
	if (memcmp(data, swen_generic_cmds,
		   CONFIG_RF_GENERIC_COMMANDS_SIZE) == 0)
		return;
	sbuf = SBUF_INIT(data, CONFIG_RF_GENERIC_COMMANDS_SIZE);

	LOG("EEPROM:\n");
	sbuf_print_hex(&sbuf);
	LOG("RAM:\n");
	sbuf = SBUF_INIT(swen_generic_cmds, CONFIG_RF_GENERIC_COMMANDS_SIZE);
	sbuf_print_hex(&sbuf);
	__abort();
}
#endif

void swen_generic_cmds_init(void (*cb)(uint16_t cmd, uint8_t status))
{
	swen_generic_cmds_storage_hdr_t *hdr = (void *)swen_generic_cmds;

	STATIC_ASSERT(CONFIG_RF_GENERIC_COMMANDS_SIZE >
		      sizeof(swen_generic_cmds_storage_hdr_t) +
		      sizeof(swen_generic_cmd_hdr_t));

	swen_generic_cmds_cb = cb;
	eeprom_load(hdr, swen_generic_cmds_storage,
		    sizeof(swen_generic_cmds_storage_hdr_t));
	if (hdr->magic != MAGIC) {
		swen_generic_cmd_hdr_t *cmd_hdr = hdr->cmds;

		hdr->magic = MAGIC;
		cmd_hdr->length = 0;
		eeprom_update(swen_generic_cmds_storage, hdr,
			      sizeof(swen_generic_cmds_storage_hdr_t) +
			      sizeof(swen_generic_cmd_hdr_t));
		return;
	}
	eeprom_load(swen_generic_cmds +
		    sizeof(swen_generic_cmds_storage_hdr_t),
		    swen_generic_cmds_storage +
		    sizeof(swen_generic_cmds_storage_hdr_t),
		    CONFIG_RF_GENERIC_COMMANDS_SIZE -
		    sizeof(swen_generic_cmds_storage_hdr_t));
}

static void swen_generic_cmds_record_timeout_cb(void *arg)
{
	swen_generic_cmds_cb(swen_generic_cmds_record_value,
			     GENERIC_CMD_STATUS_ERROR_TIMEOUT);
	swen_generic_cmds_record_value = -1;
}

void swen_generic_cmds_start_recording(int16_t value)
{
	swen_generic_cmds_record_value = value;
	timer_del(&gcmd_timer);
	timer_add(&gcmd_timer, SWEN_GENERIC_CMDS_RECORD_TIMEOUT * 1000000,
		  swen_generic_cmds_record_timeout_cb, NULL);
}

int swen_generic_cmds_for_each(int (*cb)(uint8_t number,
					 swen_generic_cmd_hdr_t *cmd_hdr,
					 void *arg), void *arg)
{
	swen_generic_cmds_storage_hdr_t *hdr = (void *)swen_generic_cmds;
	uint16_t i = 0;
	uint8_t number = 0;

	while (i < CONFIG_RF_GENERIC_COMMANDS_SIZE) {
		swen_generic_cmd_hdr_t *cmd_hdr = &hdr->cmds[i];

		if (cb(number, cmd_hdr, arg) == 0)
			return i;
		if (cmd_hdr->length == 0)
			return -1;
		i += sizeof(swen_generic_cmd_hdr_t) + cmd_hdr->length;
		number++;
	}
	return -1;
}

static int get_recorded_cmds_cb(uint8_t number,
				swen_generic_cmd_hdr_t *cmd_hdr, void *arg)
{
	buf_t *buf = arg;

	if (cmd_hdr->cmd != -1) {
		if (cmd_hdr->length == 0 || buf_has_room(buf, 2) < 0)
			return LOOP_STOP;
		__buf_addc(buf, number);
		__buf_addc(buf, cmd_hdr->cmd);
	}

	return LOOP_CONTINUE;
}

void swen_generic_cmds_get_list(buf_t *buf)
{
	int buf_len = buf->len;

	if (buf_addc(buf, GENERIC_CMD_STATUS_LIST) < 0)
		return;
	swen_generic_cmds_for_each(get_recorded_cmds_cb, buf);

	/* remove the GENERIC_CMD_STATUS_LIST byte if the list is empty */
	if (buf->len == buf_len + 1)
		__buf_shrink(buf, 1);
}

static int
delete_recorded_cmd_cb(uint8_t number, swen_generic_cmd_hdr_t *cmd_hdr,
		       void *arg)
{
	uint8_t nb = (uintptr_t)arg;

	if (number == nb && cmd_hdr->cmd != -1) {
		unsigned offset = swen_generic_cmds_addr_offset(cmd_hdr);

		cmd_hdr->cmd = -1;
		eeprom_update(swen_generic_cmds_storage + offset,
			      cmd_hdr, sizeof(swen_generic_cmd_hdr_t));
#ifdef CONSISTENCY_CHECK
		swen_generic_cmds_check_consistency();
#endif
		return LOOP_STOP;
	}
	return LOOP_CONTINUE;
}

void swen_generic_cmds_delete_recorded_cmd(uint8_t number)
{
	uint8_t status;

	if (swen_generic_cmds_for_each(delete_recorded_cmd_cb,
				       (void *)(uintptr_t)number) < 0)
		status = GENERIC_CMD_STATUS_ERROR_NOT_FOUND;
	else
		status = GENERIC_CMD_STATUS_OK;
	swen_generic_cmds_cb(swen_generic_cmds_record_value, status);
}

static int
cmd_record_cb(uint8_t number, swen_generic_cmd_hdr_t *cmd_hdr, void *arg)
{
	uint8_t status = GENERIC_CMD_STATUS_OK;
	buf_t *buf = arg;
	unsigned offset = swen_generic_cmds_addr_offset(cmd_hdr);

	if (offset + buf->len > CONFIG_RF_GENERIC_COMMANDS_SIZE) {
		DEBUG_LOG("cannot record, no more room\n");
		status = GENERIC_CMD_STATUS_ERROR_FULL;
		goto end;
	}
	if ((cmd_hdr->length >= buf->len && cmd_hdr->cmd == -1) ||
	    cmd_hdr->length == 0) {
		uint8_t *storage;

		timer_del(&gcmd_timer);
		cmd_hdr->length = buf->len;
		cmd_hdr->cmd = swen_generic_cmds_record_value;
		memcpy(cmd_hdr->data, buf->data, buf->len);
		storage = swen_generic_cmds_storage + offset;
		eeprom_update(storage, cmd_hdr, sizeof(swen_generic_cmd_hdr_t)
			      + buf->len);
		status = GENERIC_CMD_STATUS_OK;
		DEBUG_LOG("cmd recorded (nb:%u cmd:%d)\n", number,
			  swen_generic_cmds_record_value);
		goto end;
	}
	return LOOP_CONTINUE;
 end:
	swen_generic_cmds_cb(swen_generic_cmds_record_value, status);
	swen_generic_cmds_record_value = -1;
	return LOOP_STOP;
}

static int cmds_parse_cb(uint8_t number, swen_generic_cmd_hdr_t *cmd_hdr,
			 void *arg)
{
	buf_t *buf = arg;
	sbuf_t sbuf1 = buf2sbuf(buf);
	sbuf_t sbuf2 = SBUF_INIT(cmd_hdr->data, cmd_hdr->length);

	if (cmd_hdr->length && cmd_hdr->cmd == -1)
		return LOOP_CONTINUE;
	return sbuf_cmp(&sbuf1, &sbuf2);
}

static void swen_generic_cmds_record(buf_t *buf)
{
	swen_generic_cmds_storage_hdr_t *hdr = (void *)swen_generic_cmds;
	int ret = swen_generic_cmds_for_each(cmds_parse_cb, buf);

	if (ret >= 0) {
		swen_generic_cmds_cb(hdr->cmds[ret].cmd,
				     GENERIC_CMD_STATUS_ERROR_DUPLICATE);
		swen_generic_cmds_record_value = -1;
		timer_del(&gcmd_timer);
		return;
	}
	swen_generic_cmds_for_each(cmd_record_cb, buf);
#ifdef CONSISTENCY_CHECK
	swen_generic_cmds_check_consistency();
#endif
}

static void swen_parse_generic_cmds(buf_t *buf)
{
	swen_generic_cmds_storage_hdr_t *hdr = (void *)swen_generic_cmds;
	int ret = swen_generic_cmds_for_each(cmds_parse_cb, buf);

#ifdef CONSISTENCY_CHECK
	swen_generic_cmds_check_consistency();
#endif
	if (ret < 0)
		return;
	swen_generic_cmds_cb(hdr->cmds[ret].cmd, GENERIC_CMD_STATUS_RCV);
}

#endif

void (*swen_event_cb)(uint8_t from, uint8_t events, buf_t *buf);

#ifdef CONFIG_RF_SENDER
#ifdef CONFIG_IP_OVER_SWEN
int swen_get_route(const uint32_t *ip, uint8_t *dst)
{
	/* TODO */
	*dst = 0x00;
	return 0;
}
#endif

int
swen_output(pkt_t *pkt, const iface_t *iface, uint8_t type, const void *dst)
{
	swen_hdr_t *hdr;

	pkt_adj(pkt, -(int)sizeof(swen_hdr_t));
	hdr = btod(pkt);
	hdr->from = *(iface->hw_addr);

#ifdef CONFIG_IP_OVER_SWEN
	if (type == L3_PROTO_IP) {
		if (swen_get_route(dst, &hdr->to) < 0)
			/* no route */
			return -1;
	} else
#endif
		hdr->to = *(uint8_t *)dst;
	hdr->proto = type;
	hdr->chksum = 0;
	hdr->chksum = htons(cksum(hdr, pkt_len(pkt)));
	return iface->send(iface, pkt);
}

int swen_sendto(const iface_t *iface, uint8_t to, const sbuf_t *sbuf)
{
	pkt_t *pkt;

	if (to == iface->hw_addr[0]) {
		buf_t buf = sbuf2buf(sbuf);

		if (swen_event_cb) {
			swen_event_cb(to, EV_READ, &buf);
		}
		return 0;
	}

	if ((pkt = pkt_alloc()) == NULL)
		return -1;

	pkt_adj(pkt, (int)sizeof(swen_hdr_t));
	if (buf_addsbuf(&pkt->buf, sbuf) >= 0
	    && swen_output(pkt, iface, L3_PROTO_NONE, &to) >= 0)
		return 0;

	pkt_free(pkt);
	return -1;
}
#endif

void swen_ev_set(void (*ev_cb)(uint8_t from, uint8_t events, buf_t *buf))
{
	swen_event_cb = ev_cb;
}

static inline void __swen_input(pkt_t *pkt, const iface_t *iface)
{
	swen_hdr_t *hdr = btod(pkt);

	if (pkt_len(pkt) < sizeof(swen_hdr_t))
		goto end;

	if (hdr->to != iface->hw_addr[0] || cksum(hdr, pkt->buf.len) != 0) {
#ifdef CONFIG_RF_GENERIC_COMMANDS
		if (swen_generic_cmds_record_value == -1)
			swen_parse_generic_cmds(&pkt->buf);
		else
			swen_generic_cmds_record(&pkt->buf);
#endif
		goto end;
	}
	pkt_adj(pkt, (int)sizeof(swen_hdr_t));

	switch (hdr->proto) {
	case L3_PROTO_NONE:
		if (swen_event_cb)
			swen_event_cb(hdr->from, EV_READ, &pkt->buf);
		break;
#ifdef CONFIG_SWEN_L3
	case L3_PROTO_SWEN:
		swen_l3_input(hdr->from, pkt, iface);
		return;
#endif
#ifdef CONFIG_SWEN_ROLLING_CODES
	case L3_PROTO_SWEN_RC:
		swen_rc_input(hdr->from, pkt, iface);
		return;
#endif
#ifdef CONFIG_IP_OVER_SWEN
#ifdef CONFIG_IP
	case L3_PROTO_IP:
		ip_input(pkt, iface);
		return;
#endif
#endif
	default:
		/* unsupported */
		break;
	}
 end:
	pkt_free(pkt);
}

#ifdef CONFIG_RF_RECEIVER
void swen_input(const iface_t *iface)
{
	pkt_t *pkt;

	while ((pkt = pkt_get(iface->rx)))
		__swen_input(pkt, iface);
}
#endif
