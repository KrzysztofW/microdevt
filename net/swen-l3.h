#ifndef _SWEN_L3_H_
#define _SWEN_L3_H_

#include <timer.h>
#include "pkt-mempool.h"
#include "if.h"
#include "swen.h"

typedef struct swen_l3_assoc {
	uint8_t dst;
	uint8_t seq_id;
	uint8_t ack;
	uint8_t new_ack;
	uint8_t ack_needed;
	uint8_t state;
	/* uint8_t win_size; */
	tim_t timer;
	list_t list;
	list_t pkt_list;
	const iface_t *iface;
	const uint32_t *enc_key;
} swen_l3_assoc_t;

void swen_l3_assoc_init(swen_l3_assoc_t *assoc);
void swen_l3_assoc_shutdown(swen_l3_assoc_t *assoc);
int swen_l3_associate(swen_l3_assoc_t *assoc);
void swen_l3_disassociate(swen_l3_assoc_t *assoc);
void swen_l3_assoc_bind(swen_l3_assoc_t *assoc, uint8_t to,
			const iface_t *iface, const uint32_t *enc_key);
void swen_l3_input(uint8_t from, pkt_t *pkt, const iface_t *iface);
int swen_l3_send(swen_l3_assoc_t *assoc, const sbuf_t *sbuf);
#ifdef TEST
void swen_l3_retransmit_pkts(swen_l3_assoc_t *assoc);
#endif

#endif
