#ifndef _SWEN_H_
#define _SWEN_H_

#include "pkt-mempool.h"
#include "if.h"

void swen_input(const iface_t *iface);
int
swen_output(pkt_t *out, const iface_t *iface, uint8_t type, const void *dst);
int swen_sendto(const iface_t *iface, uint8_t to, const sbuf_t *sbuf);
void
swen_generic_cmds_init(void (*generic_cmds_cb)(int nb), const uint8_t *cmds);
void swen_ev_set(void (*ev_cb)(uint8_t from, uint8_t events, buf_t *buf));

#endif
