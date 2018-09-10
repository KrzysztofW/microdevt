#ifndef _SWEN_RC_H_
#define _SWEN_RC_H_

#include <crypto/xtea.h>
#include "pkt-mempool.h"
#include "if.h"

#define SWEN_RC_WINDOW 1024
typedef struct swen_rc_ctx {
	uint32_t *local_counter;
	uint32_t *remote_counter;
	const uint32_t *key;
	uint8_t dst;
	const iface_t *iface;
	void (*set_rc_cnt)(uint32_t *counter, uint8_t value);
	list_t list;
} swen_rc_ctx_t;

int swen_rc_sendto(swen_rc_ctx_t *ctx, const sbuf_t *sbuf);
void swen_rc_init(swen_rc_ctx_t *ctx, const iface_t *iface, uint8_t to,
		  uint32_t *local_cnt, uint32_t *remote_cnt,
		  void (*set_rc_cnt)(uint32_t *counter, uint8_t value),
		  const uint32_t *key);
void swen_rc_input(uint8_t from, pkt_t *pkt, const iface_t *iface);

#endif
