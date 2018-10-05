#include <stdlib.h>
#include <crypto/xtea.h>
#include <scheduler.h>
#include <sys/random.h>
#include <power-management.h>
#include "swen.h"
#include "swen-l3.h"
#include "pkt-mempool.h"

typedef enum swen_l3_op {
	S_OP_ASSOC_SYN,
	S_OP_ASSOC_SYN_ACK,
	S_OP_ASSOC_COMPLETE,
	S_OP_DISASSOC,
	S_OP_DATA,
	S_OP_ACK,
	S_OP_KEY_EXCHANGE,
} swen_l3_op_t;

#define SWEN_L3_RETRANSMIT_DELAY 1000000 /* 1s */
#define SWEN_L3_MAX_RETRIES 15

typedef struct __attribute__((__packed__)) swen_l3_hdr {
	uint8_t op;
	uint8_t seq_id;
	uint8_t ack;
} swen_l3_hdr_t;

typedef struct __attribute__((__packed__)) swen_l3_hdr_encr {
	uint8_t len;
	swen_l3_hdr_t l3_hdr;
} swen_l3_hdr_encr_t;

/* #define SWEN_L3_DEBUG */
#ifdef SWEN_L3_DEBUG
#define OP_STR_CASE(op)				\
	case (op):				\
	return #op;				\
	break

#ifdef DEBUG
static const char *swen_l3_print_op(uint8_t op)
{
	switch (op) {
		OP_STR_CASE(S_OP_ASSOC_SYN);
		OP_STR_CASE(S_OP_ASSOC_SYN_ACK);
		OP_STR_CASE(S_OP_ASSOC_COMPLETE);
		OP_STR_CASE(S_OP_DISASSOC);
		OP_STR_CASE(S_OP_DATA);
		OP_STR_CASE(S_OP_ACK);
		OP_STR_CASE(S_OP_KEY_EXCHANGE);
	default:
		return "UNKNOWN";
	}
}
#endif
#endif

static list_t assoc_list = LIST_HEAD_INIT(assoc_list);

pkt_t *swen_l3_get_pkt(swen_l3_assoc_t *assoc)
{
	pkt_t *pkt;

	if (list_empty(&assoc->incoming_pkts))
		return NULL;

	pkt = list_first_entry(&assoc->incoming_pkts, pkt_t, list);
	list_del(&pkt->list);
	return pkt;
}

static int swen_l3_output(uint8_t op, swen_l3_assoc_t *assoc,
			  const sbuf_t *sbuf);

static inline uint8_t swen_l3_get_pkt_retries(pkt_t *pkt)
{
	return pkt->buf.data[pkt->buf.len];
}

static inline void swen_l3_set_pkt_retries(pkt_t *pkt, uint8_t retries)
{
	pkt->buf.data[pkt->buf.len] = retries;
}

static inline uint8_t swen_l3_get_pkt_seqid(pkt_t *pkt)
{
	return pkt->buf.data[pkt->buf.len + 1];
}

static inline void swen_l3_set_pkt_seqid(pkt_t *pkt, uint8_t seqid)
{
	pkt->buf.data[pkt->buf.len + 1] = seqid;
}

static void swen_l3_free_assoc_pkts(swen_l3_assoc_t *assoc)
{
	pkt_t *pkt, *pkt_tmp;

	timer_del(&assoc->timer);
	list_for_each_entry_safe(pkt, pkt_tmp, &assoc->retrn_pkts, list) {
		list_del(&pkt->list);
		pkt_free(pkt);
	}
}

int swen_l3_associate(swen_l3_assoc_t *assoc)
{
	assoc->state = S_STATE_CONNECTING;
	assoc->ack = 0;
	return swen_l3_output(S_OP_ASSOC_SYN, assoc, NULL);
}

static void swen_l3_task_cb(void *arg)
{
	swen_l3_assoc_t *assoc = arg;
	pkt_t *pkt, *pkt_tmp;

	if (list_empty(&assoc->retrn_pkts)) {
		uint8_t op;

		if (assoc->state == S_STATE_CONNECTING)
			op = S_OP_ASSOC_SYN;
		else if (assoc->state == S_STATE_CLOSING)
			op = S_OP_DISASSOC;
		else if (assoc->state == S_STATE_CONN_COMPLETE)
			op = S_OP_ASSOC_COMPLETE;
		else
			return;
		swen_l3_output(op, assoc, NULL);
		return;
	}

	list_for_each_entry_safe(pkt, pkt_tmp, &assoc->retrn_pkts, list) {
		uint8_t retries;

		retries = swen_l3_get_pkt_retries(pkt);
		if (retries == 0) {
			swen_l3_free_assoc_pkts(assoc);
#ifdef CONFIG_EVENT
			event_schedule_event(&assoc->event, EV_ERROR);
#endif
			swen_l3_associate(assoc);
			return;
		}
		retries--;
		swen_l3_set_pkt_retries(pkt, retries);
		pkt_retain(pkt);
		assoc->iface->send(assoc->iface, pkt);
		break;
	}
	timer_reschedule(&assoc->timer, SWEN_L3_RETRANSMIT_DELAY);
}

#ifdef TEST
void swen_l3_retransmit_pkts(swen_l3_assoc_t *assoc)
{
	timer_del(&assoc->timer);
	swen_l3_task_cb(assoc);
}
#endif

static void swen_l3_timer_cb(void *arg)
{
	schedule_task(swen_l3_task_cb, arg);
}

static int __swen_l3_output(pkt_t *pkt, swen_l3_assoc_t *assoc, uint8_t op,
			    const sbuf_t *sbuf)
{
	swen_l3_hdr_t *hdr;
	swen_l3_hdr_encr_t *hdr_encr = NULL;
	uint8_t one_shot = 1;
	uint8_t len = 0;
	uint8_t hdr_len;

#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
	if (assoc->enc_key)
		hdr_len = sizeof(swen_l3_hdr_encr_t);
	else
		hdr_len = sizeof(swen_l3_hdr_t);

	pkt_adj(pkt, (int)(sizeof(swen_hdr_t) + hdr_len));
	if (sbuf) {
		/* reserve one byte for retry info and one byte for seqid */
		if (buf_has_room(&pkt->buf, sbuf->len + 2) < 0)
			return -1;
		__buf_addsbuf(&pkt->buf, sbuf);
		len = sbuf->len;
	}
	pkt_adj(pkt, -((int8_t)hdr_len));

	if (assoc->enc_key) {
		hdr_encr = btod(pkt);
		hdr = &hdr_encr->l3_hdr;
	} else
		hdr = btod(pkt);

	hdr->op = op;
	if (assoc->ack_needed) {
		assoc->ack += assoc->ack_needed;
		assoc->ack_needed = 0;
	}
	hdr->ack = assoc->ack;
	hdr->seq_id = assoc->seq_id;

	switch (op) {
	case S_OP_ASSOC_SYN_ACK:
		hdr->seq_id = assoc->seq_id + 1;
		hdr->ack = assoc->new_ack;
		break;
	case S_OP_ACK:
		break;
	default:
		one_shot = 0;
		break;
	}

#ifdef SWEN_L3_DEBUG
	DEBUG_LOG("%s: to:0x%X pkt:%p iface:%p op:%s seqid:0x%02X ack:0x%02X "
		  "(assoc seqid:0x%02X assoc->ack:0x%02X) one_shot:%d\n\n",
		  __func__, assoc->dst, pkt, assoc->iface, swen_l3_print_op(hdr->op),
		  hdr->seq_id, hdr->ack, assoc->seq_id, assoc->ack, one_shot);
#endif
	if (assoc->enc_key) {
		hdr_encr->len = len;
		if (xtea_encode(&pkt->buf, assoc->enc_key) < 0)
			return -1;
	}
	pkt_retain(pkt);
	if (swen_output(pkt, assoc->iface, L3_PROTO_SWEN, &assoc->dst) < 0) {
		pkt_free(pkt);
		return -1;
	}
	if (one_shot == 0) {
		swen_l3_set_pkt_retries(pkt, SWEN_L3_MAX_RETRIES);
		swen_l3_set_pkt_seqid(pkt, assoc->seq_id);

		/* TODO sliding pkt window */
		list_add_tail(&pkt->list, &assoc->retrn_pkts);
	} else
		pkt_free(pkt);
	return 0;
}

static int
swen_l3_output(uint8_t op, swen_l3_assoc_t *assoc, const sbuf_t *sbuf)
{
	pkt_t *pkt;

	if (list_empty(&assoc->retrn_pkts)) {
		uint8_t factor = op == S_OP_ASSOC_SYN ? 5 + rand() % 5 : 1;

		timer_del(&assoc->timer);
		timer_add(&assoc->timer, SWEN_L3_RETRANSMIT_DELAY * factor,
			  swen_l3_timer_cb, assoc);
	}
	if ((pkt = pkt_alloc()) == NULL) {
		if (op == S_OP_DATA)
			return -1;
		return 0;
	}

	assoc->seq_id++;
	if (__swen_l3_output(pkt, assoc, op, sbuf) < 0) {
		pkt_free(pkt);
		return -1;
	}

	/* make sure the timer did not expire before exiting this funtion */
	if (!timer_is_pending(&assoc->timer))
		timer_add(&assoc->timer, SWEN_L3_RETRANSMIT_DELAY,
			  swen_l3_timer_cb, assoc);

	return 0;
}

void swen_l3_disassociate(swen_l3_assoc_t *assoc)
{
	swen_l3_output(S_OP_DISASSOC, assoc, NULL);
	assoc->state = S_STATE_CLOSING;
}

void swen_l3_assoc_init(swen_l3_assoc_t *assoc, const uint32_t *enc_key)
{
	memset(assoc, 0, sizeof(swen_l3_assoc_t));
	assoc->enc_key = enc_key;
#ifdef CONFIG_EVENT
	event_init(&assoc->event);
#endif
	INIT_LIST_HEAD(&assoc->list);
	INIT_LIST_HEAD(&assoc->retrn_pkts);
	INIT_LIST_HEAD(&assoc->incoming_pkts);
	timer_init(&assoc->timer);
}

void swen_l3_assoc_shutdown(swen_l3_assoc_t *assoc)
{
	assoc->state = S_STATE_CLOSED;
	swen_l3_free_assoc_pkts(assoc);
	list_del(&assoc->list);
}

void
swen_l3_assoc_bind(swen_l3_assoc_t *assoc, uint8_t to, const iface_t *iface)
{
	assert(assoc->state == S_STATE_CLOSED);
#ifdef CONFIG_RND_SEED
	assoc->seq_id = rand_r(&rnd_seed);
#else
	assoc->seq_id = rand();
#endif
	assoc->iface = iface;
	assoc->dst = to;
	list_add(&assoc->list, &assoc_list);
}

/* slow linear search for assoc */
static swen_l3_assoc_t *swen_l3_assoc_lookup(uint8_t dst)
{
	swen_l3_assoc_t *assoc;

	list_for_each_entry(assoc, &assoc_list, list) {
		if (assoc->dst == dst)
			return assoc;
	}
	return NULL;
}

int swen_l3_send(swen_l3_assoc_t *assoc, const sbuf_t *sbuf)
{
	if (assoc->state != S_STATE_CONNECTED)
		return -1;
	return swen_l3_output(S_OP_DATA, assoc, sbuf);
}

static int swen_l3_in_window(uint8_t ack, uint8_t seq_id)
{
	int diff = ack - seq_id;

	return (diff & 0xFF) < 3;
}

static int swen_l3_retrn_ack_pkts(swen_l3_assoc_t *assoc, uint8_t seq_id)
{
	pkt_t *pkt, *pkt_tmp;
	int ret = -1;

	list_for_each_entry_safe(pkt, pkt_tmp, &assoc->retrn_pkts, list) {
		if (swen_l3_in_window(seq_id, swen_l3_get_pkt_seqid(pkt))) {
			list_del(&pkt->list);
			pkt_free(pkt);
			ret = 0;
		}
	}
	if (list_empty(&assoc->retrn_pkts))
		timer_del(&assoc->timer);

	return ret;
}

static void
__swen_l3_output_reuse_pkt(pkt_t *pkt, swen_l3_assoc_t *assoc, uint8_t op)
{
	buf_reset(&pkt->buf);
	__swen_l3_output(pkt, assoc, op, NULL);
}

static void swen_l3_send_ack_task_cb(void *arg)
{
	pkt_t *pkt;
	swen_l3_assoc_t *assoc = arg;

	if (!assoc->ack_needed)
		return;

	if ((pkt = pkt_alloc()) == NULL) {
		schedule_task(swen_l3_send_ack_task_cb, assoc);
		return;
	}
	__swen_l3_output(pkt, assoc, S_OP_ACK, NULL);
}

void swen_l3_input(uint8_t from, pkt_t *pkt, const iface_t *iface)
{
	swen_l3_hdr_t *hdr;
	swen_l3_assoc_t *assoc;
	int hdr_len;
	uint8_t ack, seq_id;

	/* check if the address is bound locally */
	if ((assoc = swen_l3_assoc_lookup(from)) == NULL ||
	    assoc->iface != iface)
		goto end;

	if (assoc->enc_key) {
		swen_l3_hdr_encr_t *hdr_encr = btod(pkt);

		if (xtea_decode(&pkt->buf, assoc->enc_key) < 0 ||
		    pkt->buf.len < hdr_encr->len + sizeof(swen_l3_hdr_encr_t))
			goto end;

		buf_shrink(&pkt->buf, pkt->buf.len
			   - (sizeof(swen_l3_hdr_encr_t) + hdr_encr->len));
		hdr = &hdr_encr->l3_hdr;
		hdr_len = sizeof(swen_l3_hdr_encr_t);
	} else {
		hdr = btod(pkt);
		hdr_len = sizeof(swen_l3_hdr_t);
	}
	pkt_adj(pkt, hdr_len);

#ifdef SWEN_L3_DEBUG
	DEBUG_LOG("%s: pkt:%p iface:%p op:%s seqid:0x%02X ack:0x%02X "
		  "(assoc seqid:0x%02X assoc->ack:0x%02X)\n\n",
		  __func__, pkt, iface, swen_l3_print_op(hdr->op), hdr->seq_id,
		  hdr->ack, assoc->seq_id, assoc->ack);
#endif

	switch (hdr->op) {
	case S_OP_ASSOC_SYN:
		if (hdr->ack != 0)
			break;
		assoc->new_ack = hdr->seq_id;
		__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ASSOC_SYN_ACK);
		return;

	case S_OP_ASSOC_COMPLETE:
		seq_id = assoc->seq_id + 1;
		if (hdr->ack == seq_id) {
			ack = assoc->new_ack + 1;
			if (hdr->seq_id == ack) {
				swen_l3_free_assoc_pkts(assoc);
				assoc->seq_id++;
				assoc->new_ack = 0;
				assoc->ack = hdr->seq_id;

				if (assoc->state != S_STATE_CONNECTED) {
					assoc->state = S_STATE_CONNECTED;
#ifdef CONFIG_EVENT
					event_schedule_event(&assoc->event, EV_WRITE);
#endif
				}
				__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ACK);
				return;
			}
		}
		if (hdr->seq_id == assoc->ack) {
			__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ACK);
			return;
		}
		break;

	case S_OP_ASSOC_SYN_ACK:
		if (assoc->state != S_STATE_CONNECTING)
			break;
		if (hdr->ack != assoc->seq_id)
			break;
		if (swen_l3_retrn_ack_pkts(assoc, hdr->ack) < 0)
			break;
		assoc->state = S_STATE_CONN_COMPLETE;
		assoc->ack = hdr->seq_id;
		pkt_free(pkt);
		swen_l3_output(S_OP_ASSOC_COMPLETE, assoc, NULL);
		return;

	case S_OP_ACK:
		if (swen_l3_retrn_ack_pkts(assoc, hdr->ack) < 0)
			break;
		if (assoc->state == S_STATE_CLOSING) {
			assoc->state = S_STATE_CLOSED;
			event_schedule_event(&assoc->event, EV_HUNGUP);
			break;
		}
		if (assoc->state == S_STATE_CONN_COMPLETE) {
			assoc->state = S_STATE_CONNECTED;
#ifdef CONFIG_EVENT
			event_schedule_event(&assoc->event, EV_WRITE);
#endif
		}
		break;

	case S_OP_DATA:
		if (assoc->state != S_STATE_CONNECTED
		    && assoc->state != S_STATE_CLOSING)
			break;
		swen_l3_retrn_ack_pkts(assoc, hdr->ack);

		if (swen_l3_in_window(assoc->ack + assoc->ack_needed,
				      hdr->seq_id)) {
			__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ACK);
			return;
		}

		/* save the current ack value in case a write is done
		 * in the event callback */
		assoc->ack_needed++;
		list_add_tail(&pkt->list, &assoc->incoming_pkts);
#ifdef CONFIG_EVENT
		event_schedule_event(&assoc->event, EV_READ | EV_WRITE);
#endif
		schedule_task(swen_l3_send_ack_task_cb, assoc);
		return;

	case S_OP_DISASSOC:
		swen_l3_retrn_ack_pkts(assoc, hdr->ack);
		if (hdr->seq_id == assoc->ack && assoc->state == S_STATE_CLOSED) {
			__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ACK);
			return;
		}
		if (swen_l3_in_window(assoc->ack + assoc->ack_needed,
				      hdr->seq_id)) {
			__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ACK);
			return;
		}
		assoc->ack_needed++;

		if (assoc->state == S_STATE_CONNECTED)
			assoc->state = S_STATE_CLOSED;
#ifdef CONFIG_EVENT
		event_schedule_event(&assoc->event, EV_HUNGUP);
#endif
		__swen_l3_output_reuse_pkt(pkt, assoc, S_OP_ACK);
		return;

	case S_OP_KEY_EXCHANGE:
		DEBUG_LOG("KEY_EXCHANGE unsupported\n");
		break;
	}
 end:
	pkt_free(pkt);
}
