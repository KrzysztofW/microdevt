#include "event.h"
#include "swen-rc.h"
#include "swen.h"

typedef struct __attribute__((__packed__)) swen_rc_hdr {
	uint8_t  len;
	uint32_t counter;
} swen_rc_hdr_t;
static list_t ctx_list = LIST_HEAD_INIT(ctx_list);

int swen_rc_sendto(swen_rc_ctx_t *ctx, const sbuf_t *sbuf)
{
	pkt_t *pkt;
	swen_rc_hdr_t *hdr;

	if ((pkt = pkt_alloc()) == NULL)
		return -1;

	pkt_adj(pkt, (int)(sizeof(swen_hdr_t) + sizeof(swen_rc_hdr_t)));

	if (buf_addsbuf(&pkt->buf, sbuf) < 0)
		return -1;
	pkt_adj(pkt, -(int)sizeof(swen_rc_hdr_t));
	hdr = btod(pkt);
	hdr->len = sbuf->len;
	hdr->counter = *ctx->remote_counter;

	if (xtea_encode(&pkt->buf, ctx->key) < 0)
		goto error;
	if (swen_output(pkt, ctx->iface, L3_PROTO_SWEN_RC, &ctx->dst) < 0)
		goto error;

	return 0;

 error:
	pkt_free(pkt);
	return -1;
}

/* slow linear search for ctx */
static swen_rc_ctx_t *swen_rc_ctx_lookup(uint8_t dst)
{
	swen_rc_ctx_t *ctx;

	list_for_each_entry(ctx, &ctx_list, list) {
		if (ctx->dst == dst)
			return ctx;
	}
	return NULL;
}

void swen_rc_input(uint8_t from, pkt_t *pkt, const iface_t *iface)
{
	swen_rc_ctx_t *ctx = swen_rc_ctx_lookup(from);
	swen_rc_hdr_t *hdr = btod(pkt);
	uint32_t window;

	if (ctx == NULL || ctx->iface != iface)
		goto end;

	if (xtea_decode(&pkt->buf, ctx->key) < 0 ||
	    pkt->buf.len < hdr->len + sizeof(swen_rc_hdr_t))
		goto end;

	window = 0xFFFFFFFF - ((0xFFFFFFFF + *ctx->local_counter -
				hdr->counter) & 0xFFFFFFFF);
	if (window > 255)
		goto end;

	ctx->set_rc_cnt(ctx->local_counter, window);
	buf_shrink(&pkt->buf, pkt->buf.len
		   - (sizeof(swen_rc_hdr_t) + hdr->len));

	pkt_adj(pkt, sizeof(swen_rc_hdr_t));
	swen_event_cb(from, EV_READ, &pkt->buf);
 end:
	pkt_free(pkt);
}

void swen_rc_init(swen_rc_ctx_t *ctx, const iface_t *iface, uint8_t to,
		  uint32_t *local_cnt, uint32_t *remote_cnt,
		  void (*set_rc_cnt)(uint32_t *counter, uint8_t value),
		  const uint32_t *key)
{
	ctx->local_counter = local_cnt;
	ctx->remote_counter = remote_cnt;
	ctx->dst = to;
	ctx->iface = iface;
	ctx->key = key;
	ctx->set_rc_cnt = set_rc_cnt;
	INIT_LIST_HEAD(&ctx->list);
	list_add(&ctx->list, &ctx_list);
}
