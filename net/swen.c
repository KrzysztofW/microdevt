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

typedef struct cmd_replay_data {
	uint8_t number;
	iface_t *iface;
} cmd_replay_data_t;

#define SWEN_GENERIC_CMDS_RECORD_TIMEOUT 10 /* seconds */
static tim_t gcmd_timer = TIMER_INIT(gcmd_timer);

#define LOOP_STOP 0
#define LOOP_CONTINUE -1
/* #define EEPROM_CONSISTENCY_CHECK */

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

#ifdef EEPROM_CONSISTENCY_CHECK
void swen_generic_cmds_dump_storage(uint8_t check_eeprom)
{
	uint8_t data[CONFIG_RF_GENERIC_COMMANDS_SIZE];
	sbuf_t sbuf;

	eeprom_load(data, swen_generic_cmds_storage,
		    CONFIG_RF_GENERIC_COMMANDS_SIZE);
	if (check_eeprom) {
		if (memcmp(data, swen_generic_cmds,
			   CONFIG_RF_GENERIC_COMMANDS_SIZE) == 0)
			return;
	}

	sbuf = SBUF_INIT(data, CONFIG_RF_GENERIC_COMMANDS_SIZE);
	LOG("EEPROM:\n");
	sbuf_print_hex(&sbuf);
	LOG("RAM:\n");
	sbuf = SBUF_INIT(swen_generic_cmds, CONFIG_RF_GENERIC_COMMANDS_SIZE);
	sbuf_print_hex(&sbuf);

	if (check_eeprom)
		__abort();
}
void swen_generic_cmds_erase_storage(void)
{
	swen_generic_cmds_storage_hdr_t *hdr = (void *)swen_generic_cmds;

	hdr->magic = MAGIC;
	memset(swen_generic_cmds + 1, 0, CONFIG_RF_GENERIC_COMMANDS_SIZE);
	eeprom_update(swen_generic_cmds_storage, swen_generic_cmds,
		      CONFIG_RF_GENERIC_COMMANDS_SIZE);
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
		eeprom_update(swen_generic_cmds_storage, swen_generic_cmds,
			      CONFIG_RF_GENERIC_COMMANDS_SIZE);
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
	swen_generic_cmd_hdr_t *cmd_hdr = &hdr->cmds[0];
	uint16_t i = 0;
	uint8_t number = 0;

	while (i < CONFIG_RF_GENERIC_COMMANDS_SIZE) {
		if (cb(number, cmd_hdr, arg) == LOOP_STOP)
			return i;
		if (cmd_hdr->length == 0)
			return -1;
		i += sizeof(swen_generic_cmd_hdr_t) + cmd_hdr->length;
		number++;
		cmd_hdr = (void *)((uint8_t *)cmd_hdr + i);
	}
	return -1;
}

static int get_recorded_cmds_cb(uint8_t number,
				swen_generic_cmd_hdr_t *cmd_hdr, void *arg)
{
	buf_t *buf = arg;

	if (cmd_hdr->cmd != -1) {
		uint16_t v;

		if (cmd_hdr->length == 0 || buf_has_room(buf, 2) < 0)
			return LOOP_STOP;
		v = (cmd_hdr->cmd << 8) | number;
		__buf_add(buf, &v, sizeof(v));
	}

	return LOOP_CONTINUE;
}

void swen_generic_cmds_get_list(buf_t *buf)
{
	swen_generic_cmds_for_each(get_recorded_cmds_cb, buf);
}

static int
delete_recorded_cmd_cb(uint8_t number, swen_generic_cmd_hdr_t *cmd_hdr,
		       void *arg)
{
	int nb = (intptr_t)arg;

	if ((nb == -1 || (uint8_t)number == nb) && cmd_hdr->cmd != -1) {
		unsigned offset = swen_generic_cmds_addr_offset(cmd_hdr);

		cmd_hdr->cmd = -1;
		eeprom_update(swen_generic_cmds_storage + offset,
			      cmd_hdr, sizeof(swen_generic_cmd_hdr_t));
#ifdef EEPROM_CONSISTENCY_CHECK
		swen_generic_cmds_dump_storage(1);
#endif
		if (nb != -1)
			return LOOP_STOP;
	}
	return LOOP_CONTINUE;
}

static int
swen_generic_cmds_is_empty_cb(uint8_t number, swen_generic_cmd_hdr_t *cmd_hdr,
			      void *arg)
{
	unsigned *len = arg;

	if (cmd_hdr->length)
		*len += cmd_hdr->length + sizeof(swen_generic_cmd_hdr_t);
	return cmd_hdr->cmd == -1 || cmd_hdr->length == 0 ?
		LOOP_CONTINUE : LOOP_STOP;
}

static void swen_generic_cmds_wipe(void)
{
	unsigned len = 0;
	swen_generic_cmds_storage_hdr_t *hdr;
	swen_generic_cmds_storage_hdr_t *storage_hdr;

	if (swen_generic_cmds_for_each(swen_generic_cmds_is_empty_cb, &len)
	    >= 0)
		return;

	hdr = (void *)swen_generic_cmds;
	memset(hdr->cmds, 0, len);
	storage_hdr = (void *)swen_generic_cmds_storage;
	eeprom_update(storage_hdr->cmds, hdr->cmds, len);
}

void swen_generic_cmds_delete_recorded_cmd(int number)
{
	uint8_t status;

	if (swen_generic_cmds_for_each(delete_recorded_cmd_cb,
				       (void *)(intptr_t)number) < 0
	    && number != -1) {
		status = GENERIC_CMD_STATUS_ERROR_NOT_FOUND;
	} else {
		status = GENERIC_CMD_STATUS_OK;
		swen_generic_cmds_wipe();
	}
	(*swen_generic_cmds_cb)(swen_generic_cmds_record_value, status);
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
	if ((cmd_hdr->length == buf->len && cmd_hdr->cmd == -1) ||
	    cmd_hdr->length == 0) {
		uint8_t *storage;

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
	timer_del(&gcmd_timer);
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
#ifdef EEPROM_CONSISTENCY_CHECK
	swen_generic_cmds_dump_storage(1);
#endif
}

static void swen_parse_generic_cmds(buf_t *buf)
{
	swen_generic_cmds_storage_hdr_t *hdr = (void *)swen_generic_cmds;
	swen_generic_cmd_hdr_t *cmd_hdr = &hdr->cmds[0];
	int ret = swen_generic_cmds_for_each(cmds_parse_cb, buf);

#ifdef EEPROM_CONSISTENCY_CHECK
	swen_generic_cmds_dump_storage(1);
#endif
	if (ret < 0)
		return;
	cmd_hdr = (void *)((uint8_t *)cmd_hdr + ret);
	swen_generic_cmds_cb(cmd_hdr->cmd, GENERIC_CMD_STATUS_RCV);
}

static int
replay_cmd_cb(uint8_t number, swen_generic_cmd_hdr_t *cmd_hdr, void *arg)
{
	cmd_replay_data_t *rd = arg;
	pkt_t *pkt;

	if (number != rd->number)
		return LOOP_CONTINUE;

	pkt = pkt_alloc();
	if (pkt == NULL || buf_add(&pkt->buf, cmd_hdr->data, cmd_hdr->length) < 0)
		goto end;
	if (rd->iface->send(rd->iface, pkt) < 0)
		goto end;
	return LOOP_STOP;
 end:
	DEBUG_LOG("cannot send\n");
	pkt_free(pkt);
	return LOOP_STOP;
}

int swen_generic_cmd_replay(iface_t *iface, uint8_t number)
{
	cmd_replay_data_t replay_data;

	replay_data.number = number;
	replay_data.iface = iface;

	return swen_generic_cmds_for_each(replay_cmd_cb, &replay_data);
}

const char *swen_generic_cmd_status2str(uint8_t status)
{
	switch (status) {
	case GENERIC_CMD_STATUS_OK:
		return "OK";
	case GENERIC_CMD_STATUS_ERROR_FULL:
		return "FULL";
	case GENERIC_CMD_STATUS_ERROR_DUPLICATE:
		return "DUPLICATE";
	case GENERIC_CMD_STATUS_ERROR_TIMEOUT:
		return "TIMEOUT";
	default:
		return "unsupported";
	}
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
swen_output(pkt_t *pkt, iface_t *iface, uint8_t type, const void *dst)
{
	swen_hdr_t *hdr;

	pkt_adj(pkt, -(int)sizeof(swen_hdr_t));
	hdr = btod(pkt);
	hdr->from = *(iface->hw_addr);
#ifdef SWEN_MULTI_PROTOCOL
#ifdef CONFIG_IP_OVER_SWEN
	if (type == L3_PROTO_IP) {
		if (swen_get_route(dst, &hdr->to) < 0)
			/* no route */
			return -1;
	} else
#endif
		hdr->to = *(uint8_t *)dst;
	hdr->proto = type;
#else
	hdr->to = *(uint8_t *)dst;
#endif
	hdr->chksum = 0;

	/* No need to htons() as chksum field is placed at odd position
	 * in the header */
	hdr->chksum = cksum(hdr, pkt_len(pkt));

	return iface->send(iface, pkt);
}

int swen_sendto(iface_t *iface, uint8_t to, const sbuf_t *sbuf)
{
	pkt_t *pkt;

#if 0
	/* check if we're sending to us */
	if (to == iface->hw_addr[0]) {
		buf_t buf = sbuf2buf(sbuf);

		if (swen_event_cb)
			swen_event_cb(to, EV_READ, &buf);
		return 0;
	}
#endif
	if ((pkt = pkt_alloc()) == NULL)
		return -1;

	if (sbuf->len > pkt->buf.size - sizeof(swen_hdr_t))
		goto error;

	pkt_adj(pkt, (int)sizeof(swen_hdr_t));

	if (buf_addsbuf(&pkt->buf, sbuf) >= 0
	    && swen_output(pkt, iface, L3_PROTO_NONE, &to) >= 0)
		return 0;
 error:
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

	if (pkt_len(pkt) < sizeof(swen_hdr_t) ||
	    hdr->to != iface->hw_addr[0] || cksum(hdr, pkt->buf.len) != 0) {
#ifdef CONFIG_RF_GENERIC_COMMANDS
		/* check the minimum generic command length */
		if (pkt->buf.len < 3)
			goto end;
		if (swen_generic_cmds_record_value == -1)
			swen_parse_generic_cmds(&pkt->buf);
		else
			swen_generic_cmds_record(&pkt->buf);
#endif
		goto end;
	}
	pkt_adj(pkt, (int)sizeof(swen_hdr_t));

#ifdef SWEN_MULTI_PROTOCOL
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
#else
	if (swen_event_cb)
		swen_event_cb(hdr->from, EV_READ, &pkt->buf);
#endif
 end:
	pkt_free(pkt);
}

#ifdef CONFIG_RF_RECEIVER
void swen_input(iface_t *iface)
{
	pkt_t *pkt;

	while ((pkt = pkt_get(iface->rx)))
		__swen_input(pkt, iface);
}

#if defined(CONFIG_RF_GENERIC_COMMANDS) &&		\
    defined(CONFIG_RF_GENERIC_COMMANDS_CHECKS)
static pkt_t *sent_pkt;
static int iface_send(iface_t *iface, pkt_t *pkt)
{
	sent_pkt = pkt;
	return 0;
}

int swen_generic_cmds_check(iface_t *iface)
{
	const char cmd1[] = "123";
	const char cmd2[] = "456";
	uint8_t data[] = {
		0xA7, /* magic number */
		0x04, 0x00, 0x31, 0x32, 0x33, 0x00, /* cmd1 */
		0x04, 0x01, 0x34, 0x35, 0x36, 0x00, /* cmd2 */
	};
	uint8_t data_deleted_cmd1[] = {
		0xA7, /* magic number */
		0x04, 0xFF, 0x31, 0x32, 0x33, 0x00, /* cmd1 */
		0x04, 0x01, 0x34, 0x35, 0x36, 0x00, /* cmd2 */
	};
	uint8_t data_deleted_cmd2[] = {
		0xA7, /* magic number */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	pkt_t *pkt = pkt_alloc();
	sbuf_t s1;
	sbuf_t s2;

	if (pkt == NULL)
		return -1;

	swen_generic_cmds_record_value = 0;
	if (buf_adds(&pkt->buf, cmd1) < 0)
		return -1;

	swen_generic_cmds_record(&pkt->buf);

	buf_reset(&pkt->buf);
	swen_generic_cmds_record_value = 1;
	if (buf_adds(&pkt->buf, cmd2) < 0)
		return -1;

	swen_generic_cmds_record(&pkt->buf);
	__swen_input(pkt, iface);

	pkt = pkt_alloc();
	if (pkt == NULL)
		return -1;

	if (buf_adds(&pkt->buf, cmd1) < 0)
		return -1;

	__swen_input(pkt, iface);
#ifdef EEPROM_CONSISTENCY_CHECK
	swen_generic_cmds_dump_storage(1);
#endif
	sbuf_init(&s1, data, sizeof(data));
	sbuf_init(&s2, swen_generic_cmds, sizeof(data));
	if (sbuf_cmp(&s1, &s2) != 0)
		return -1;

	/* cmd replay */
	iface->send = iface_send;
	if (swen_generic_cmd_replay(iface, 1) < 0) {
		LOG("cannot replay");
		return -1;
	}
	s1 = SBUF_INIT_BIN(cmd2);
	s2 = buf2sbuf(&sent_pkt->buf);
	if (sbuf_cmp(&s1, &s2) != 0) {
		pkt_free(sent_pkt);
		LOG("cannot replay");
		return -1;
	}
	pkt_free(sent_pkt);
	if (swen_generic_cmd_replay(iface, 0) < 0) {
		LOG("cannot replay");
		return -1;
	}
	s1 = SBUF_INIT_BIN(cmd1);
	s2 = buf2sbuf(&sent_pkt->buf);
	if (sbuf_cmp(&s1, &s2) != 0) {
		pkt_free(sent_pkt);
		LOG("cannot replay");
		return -1;
	}
	pkt_free(sent_pkt);

	/* cmd deletion */
	swen_generic_cmds_delete_recorded_cmd(0);
	sbuf_init(&s1, data_deleted_cmd1, sizeof(data_deleted_cmd1));
	sbuf_init(&s2, swen_generic_cmds, sizeof(data));
	if (sbuf_cmp(&s1, &s2) != 0)
		return -1;

#ifdef EEPROM_CONSISTENCY_CHECK
	swen_generic_cmds_dump_storage(1);
#endif
	swen_generic_cmds_delete_recorded_cmd(1);
	sbuf_init(&s1, data_deleted_cmd2, sizeof(data_deleted_cmd2));
	if (sbuf_cmp(&s1, &s2) != 0)
		return -1;
#ifdef EEPROM_CONSISTENCY_CHECK
	swen_generic_cmds_dump_storage(1);
#endif
	return 0;
}
#endif
#endif
