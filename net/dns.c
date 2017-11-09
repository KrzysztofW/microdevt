#include <alloca.h>

#include "dns.h"
#include "socket.h"
#include "../sys/list.h"
#include "../timer.h"

static uint32_t dns_resolve_ip_addr;


#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DNS_FLAG_NON_AUTH_DATA 0x0001
#define DNS_FLAG_RSVD          0x0002
#define DNS_FLAG_RECURSION_DESIRED 0x0004
#define DNS_FLAG_TRUNCATED     0x0008
#define DNS_FLAG_OPCODE        0x0010
#define DNS_FLAG_RESPONSE      0x0020
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DNS_FLAG_NON_AUTH_DATA 0x0100
#define DNS_FLAG_RSVD          0x0200
#define DNS_FLAG_RECURSION_DESIRED 0x0400
#define DNS_FLAG_TRUNCATED     0x0800
#define DNS_FLAG_OPCODE        0x1000
#define DNS_FLAG_RESPONSE      0x2000
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DNS_TYPE_A 0x0001
#define DNS_CLASS_IN 0x0001
#define DNS_UDP_PORT 0x35
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DNS_TYPE_A 0x0100
#define DNS_CLASS_IN 0x0100
#define DNS_UDP_PORT 0x3500
#endif

typedef struct dns_query dns_query_t;

struct dns_query {
	uint16_t tr_id;
	uint16_t flags;
	uint16_t question_cnt;
	uint16_t answer_rrs;
	uint16_t authority_rrs;
	uint16_t additional_rrs;
	uint8_t  queries[];
} __attribute__((__packed__));
typedef struct dns_query dns_query_t;

struct dns_query_ctx {
	struct list_head list;
	sock_info_t sock_info;
	tim_t timer;
	uint16_t tr_id;
	void (*cb)(uint32_t ip);
	uint8_t name_len;
	/* space for name must be here */
} __attribute__((__packed__));
typedef struct dns_query_ctx dns_query_ctx_t;

struct list_head dns_ctxs = LIST_HEAD_INIT(dns_ctxs);
#define DNS_QUERY_TIMEOUT 10000U /* millisecs */
#define DNS_TYPE_LEN 2
#define DNS_CLASS_LEN 2

static void dns_query_cb(void *arg)
{
	dns_query_ctx_t *ctx = arg;
	pkt_t *pkt;
	sbuf_t sb;
	int i;
	uint32_t src_addr;
	uint16_t src_port;

	if (__socket_get_pkt(&ctx->sock_info, &pkt, &src_addr, &src_port) < 0)
		return;

	sb = PKT2SBUF(pkt);
	for (i = 0; i < sb.len; i++) {
		printf(" 0x%02X", sb.data[i]);
	}
	puts("");
	ctx->cb(0);
}

static int dns_query_ctx_init(dns_query_ctx_t *ctx, const sbuf_t *sb)
{
	memset(ctx, 0, sizeof(dns_query_ctx_t));
	if (sock_info_init(&ctx->sock_info, SOCK_DGRAM, 0) < 0)
		return -1;
	__sock_info_add(&ctx->sock_info);
	ctx->name_len = sb->len;
	INIT_LIST_HEAD(&ctx->list);
	list_add_tail(&ctx->list, &dns_ctxs);
	memcpy(ctx + 1, sb->data, sb->len);
	timer_add(&ctx->timer, DNS_QUERY_TIMEOUT * 1000, dns_query_cb, ctx);
	return 0;
}

static void dns_query_ctx_free(dns_query_ctx_t *ctx)
{
	sock_info_close(&ctx->sock_info);
	list_del(&ctx->list);
	if (timer_is_pending(&ctx->timer))
		timer_del(&ctx->timer);
	free(ctx);
}

static void parse_domain_name(const sbuf_t *name, buf_t *out)
{
	uint8_t i;
	buf_t label = BUF(255);

	for (i = 0; i < name->len; i++) {
		if (name->data[i] == '.') {
			buf_addc(out, label.len);
			buf_addbuf(out, &label);
			buf_reset(&label);
		} else
			buf_addc(&label, name->data[i]);
	}
	buf_addc(out, label.len);
	buf_addbuf(out, &label);
	buf_addc(out, 0);
}

int dns_resolve(const sbuf_t *name, void (*cb)(uint32_t ip))
{
	dns_query_t *dns;
	uint8_t dns_query_len;
	dns_query_ctx_t *ctx;
	uint8_t *q;
	sbuf_t sb;
	buf_t buf;

	dns_query_len = sizeof(dns_query_t) + name->len + 2 +
		DNS_TYPE_LEN  + DNS_CLASS_LEN;
	dns = alloca(dns_query_len);
	if ((ctx = malloc(sizeof(dns_query_ctx_t) + name->len)) == NULL)
		return -1;
	if (dns_query_ctx_init(ctx, name) < 0) {
		free(ctx);
		return -1;
	}

	ctx->cb = cb;
	dns->tr_id = rand();
	dns->flags = DNS_FLAG_RECURSION_DESIRED;
	dns->question_cnt = htons(1);
	dns->answer_rrs = 0;
	dns->authority_rrs = 0;
	dns->additional_rrs = 0;
	q = dns->queries;
	buf = (buf_t){
		.data = q,
		.size = name->len + 2
	};
	parse_domain_name(name, &buf);
	q += buf.len;
	*(uint16_t *)q = DNS_TYPE_A;
	q += 2;
	*(uint16_t *)q = DNS_CLASS_IN;
	sbuf_init(&sb, dns, dns_query_len);
	printf("%s:%d\n", __func__, __LINE__);
	if (__socket_put_sbuf(&ctx->sock_info, &sb, dns_resolve_ip_addr,
			      DNS_UDP_PORT) < 0) {
		dns_query_ctx_free(ctx);
		return -1;
	}
	return 0;
}

void dns_init(uint32_t ip)
{
	dns_resolve_ip_addr = ip;
}
