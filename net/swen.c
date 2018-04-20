#include <sys/utils.h>
#include <sys/chksum.h>
#include <sys/buf.h>
#include <sys/ring.h>
#include <drivers/rf.h>
#include "event.h"
#include "swen.h"

/* XXX the size of this structure has to be even (in order to use
 * cksum_partial() on it). */
typedef struct __attribute__((__packed__)) swen_hdr_t {
	uint8_t to;
	uint8_t from;
	uint16_t proto :  2;
	uint16_t len   : 14;
	uint16_t chksum;
} swen_hdr_t;

#ifdef CONFIG_RF_GENERIC_COMMANDS
static void (*generic_cmds_cb)(int nb);
static const uint8_t *generic_cmds;

void
swen_generic_cmds_init(void (*cb)(int nb), const uint8_t *cmds)
{
	generic_cmds_cb = cb;
	generic_cmds = cmds;
}
#endif

static void (*swen_event_cb)(uint8_t from, uint8_t events, buf_t *buf);

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
	hdr = btod(pkt, swen_hdr_t *);
	hdr->from = *(iface->hw_addr);

#ifdef CONFIG_IP_OVER_SWEN
	if (type == L3_PROTO_IP) {
		if (swen_get_route(dst, &hdr->to) < 0) {
			/* no route */
			pkt_free(pkt);
			return -1;
		}
	} else
#endif
		hdr->to = *(uint8_t *)dst;
	hdr->proto = type;
	hdr->len = htons(pkt_len(pkt));
	hdr->chksum = 0;
	hdr->chksum = cksum(hdr, pkt_len(pkt));
	iface->send(iface, pkt);
	return 0;
}

int swen_sendto(const iface_t *iface, uint8_t to, const sbuf_t *sbuf)
{
	pkt_t *pkt = pkt_alloc();

	if (pkt == NULL)
		return -1;

	pkt_adj(pkt, (int)sizeof(swen_hdr_t));
	if (buf_addsbuf(&pkt->buf, sbuf) < 0)
		return -1;
	return swen_output(pkt, iface, L3_PROTO_SWEN, NULL);
}
#endif

#if defined CONFIG_RF_RECEIVER && defined CONFIG_RF_GENERIC_COMMANDS
static int swen_parse_generic_cmds(const buf_t *buf)
{
	uint8_t cmd_index;
	const uint8_t *cmds;
	sbuf_t sbuf = buf2sbuf(buf);

	if (generic_cmds_cb == NULL || generic_cmds == NULL)
		return -1;

	cmd_index = 0;
	cmds = generic_cmds;
	while (cmds[0] != 0) {
		uint8_t len = cmds[0];
		sbuf_t cmd;

		/* ring len is the same throughout the loop */
		if (buf->len < len)
			return -1;
		cmds++;
		sbuf_init(&cmd, cmds, len);
		if (sbuf_cmp(&sbuf, &cmd) == 0) {
			generic_cmds_cb(cmd_index);
			return 0;
		}
		cmd_index++;
		cmds += len;
	}
	return -1;
}
#endif

void swen_ev_set(void (*ev_cb)(uint8_t from, uint8_t events, buf_t *buf))
{
	swen_event_cb = ev_cb;
}

static inline void __swen_input(pkt_t *pkt, const iface_t *iface)
{
	swen_hdr_t *hdr;

#ifdef CONFIG_RF_GENERIC_COMMANDS
	if (swen_parse_generic_cmds(&pkt->buf) >= 0)
		goto end;
#endif

	if (pkt->buf.len < sizeof(swen_hdr_t))
		goto end;

	hdr = btod(pkt, swen_hdr_t *);

	/* check if data is for us */
	if (hdr->to != iface->hw_addr[0])
		goto end;

	pkt_adj(pkt, (int)sizeof(swen_hdr_t));
	if (hdr->len != pkt->buf.len)
		goto end;

	if (cksum(hdr, pkt->buf.len) != 0)
		goto end;

	/* TODO: chained pkts */

	switch (hdr->proto) {
	case RF_TYPE_SWEN:
		if (swen_event_cb)
			swen_event_cb(hdr->from, EV_READ, &pkt->buf);
		break;
#ifdef CONFIG_SWEN_L4
	case RF_TYPE_SWEN4:
		swen_l4_input(hdr->from, pkt, iface);
		return;
#endif
#ifdef CONFIG_IP_OVER_SWEN
#ifdef CONFIG_IP
	case RF_TYPE_IP:
		ip_input(pkt, iface);
		return;
#endif
#ifdef CONFIG_ICMP
	case RF_TYPE_ICMP:
		ip_icmp(pkt, iface);
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

	while ((pkt = pkt_get(iface->rx))) {
		/* XXX swen_hdr_t size must be <= MIN generic command size */
		if (pkt_len(pkt) > sizeof(swen_hdr_t))
			__swen_input(pkt, iface);
	}
}
#endif
