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
};

typedef struct __attribute__((__packed__)) swen_hdr_t {
	uint8_t to;
	uint8_t from;
	uint8_t proto;
	uint16_t chksum;
} swen_hdr_t;

void swen_input(const iface_t *iface);
int
swen_output(pkt_t *out, const iface_t *iface, uint8_t type, const void *dst);
int swen_sendto(const iface_t *iface, uint8_t to, const sbuf_t *sbuf);
void swen_generic_cmds_init(void (*cb)(uint16_t cmd, uint8_t status));
void swen_generic_cmds_start_recording(int16_t value);
void swen_ev_set(void (*ev_cb)(uint8_t from, uint8_t events, buf_t *buf));
void swen_generic_cmds_get_list(buf_t *buf);
int swen_generic_cmds_delete_recorded_cmd(uint8_t number);
extern void (*swen_event_cb)(uint8_t from, uint8_t events, buf_t *buf);

#endif
