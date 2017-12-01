#include <alloca.h>
#include <log.h>
#include "dns.h"
#include "socket.h"
#include "../sys/list.h"
#include "../timer.h"

static uint32_t dns_resolve_ip_addr;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DNS_FLAG_NON_AUTH_DATA 0x0010
#define DNS_FLAG_RECURSION_DESIRED 0x0100
#define DNS_FLAG_TRUNCATED     0x0200
#define DNS_FLAG_RESPONSE      0x8000
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DNS_FLAG_NON_AUTH_DATA 0x1000
#define DNS_FLAG_RECURSION_DESIRED 0x0001
#define DNS_FLAG_TRUNCATED     0x0002
#define DNS_FLAG_RESPONSE      0x0008
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
	sock_info_t sock_info;
	struct list_head list;
	tim_t timer;
	uint16_t tr_id;
	void (*cb)(uint32_t ip);
	uint8_t name_len;
	/* space for name must be here */
} __attribute__((__packed__));
typedef struct dns_query_ctx dns_query_ctx_t;

#define DNS_QUERY_TIMEOUT 10000U /* millisecs */
#define DNS_TYPE_LEN 2
#define DNS_CLASS_LEN 2

static void dns_query_ctx_free(dns_query_ctx_t *ctx)
{
	sock_info_close(&ctx->sock_info);
	list_del(&ctx->list);
	if (timer_is_pending(&ctx->timer))
		timer_del(&ctx->timer);
	free(ctx);
}

static void dns_query_timeout_cb(void *arg)
{
	dns_query_ctx_t *ctx = arg;

	DEBUG_LOG("DNS timeout\n");
	ctx->cb(0);
	dns_query_ctx_free(ctx);
}

static int
dns_parse_answer(const dns_query_t *dns_answer, int data_len, uint32_t *ip)
{
	buf_t buf;
	uint8_t nb = ntohs(dns_answer->question_cnt);
	uint8_t len;

	buf_init(&buf, (void *)dns_answer->queries, data_len);
	while (nb && buf_len(&buf) > 0) {
		len = *(buf_data(&buf));

		if (len == 0) {
			buf_adj(&buf, 5); /* skip len, type, class */
			nb--;
			continue;
		}
		if (len > buf_len(&buf))
			return -1;
		buf_adj(&buf, len + 1);
	}
	nb = ntohs(dns_answer->answer_rrs);
	while (nb && buf_len(&buf)) {
		uint16_t type;

		/* skip name */
		buf_adj(&buf, 2);
		type = *(uint16_t *)buf_data(&buf);
		/* skip type, class, ttl, first byte of len */
		buf_adj(&buf, 9);
		len = *buf_data(&buf);
		buf_adj(&buf, 1); /* skip second byte of len */
		if (len > buf_len(&buf))
			return -1;
		if (type != DNS_TYPE_A) {
			buf_adj(&buf, len);
			nb--;
			continue;
		}
		if (len != sizeof(uint32_t)) { /* only ipv4 is supported */
			nb--;
			continue;
		}
		*ip = *(uint32_t *)buf_data(&buf);
		return 0;
	}

	return -1;
}

static void ev_dns_cb(sock_info_t *sock_info, uint8_t events)
{
	dns_query_ctx_t *ctx = container_of(sock_info, dns_query_ctx_t, sock_info);
	pkt_t *pkt;
	dns_query_t *dns_answer;
	uint32_t ip = 0;
	int data_len;

	DEBUG_LOG("received read event\n");
	if (__socket_get_pkt(&ctx->sock_info, &pkt, NULL, NULL) < 0)
		return;

	if (pkt_len(pkt) < sizeof(dns_query_t))
		goto error;

	dns_answer = btod(pkt, dns_query_t *);
	if (ctx->tr_id != dns_answer->tr_id)
		goto error;

	if ((data_len = pkt_len(pkt) - (sizeof(dns_query_t) - 1)) < 0)
		goto error;

	if (dns_parse_answer(dns_answer, data_len, &ip) < 0) {
		DEBUG_LOG("failed parsing DNS answer\n");
		goto end;
	}
 end:
	ctx->cb(ip);
	dns_query_ctx_free(ctx);
 error:
	pkt_free(pkt);
}

static int dns_query_ctx_init(dns_query_ctx_t *ctx, const sbuf_t *sb)
{
	memset(ctx, 0, sizeof(dns_query_ctx_t));
	if (sock_info_init(&ctx->sock_info, SOCK_DGRAM) < 0)
		return -1;
	ev_set(&ctx->sock_info, EV_READ, ev_dns_cb);
	ctx->name_len = sb->len;
	ctx->tr_id = rand();
	INIT_LIST_HEAD(&ctx->list);
	memcpy(ctx + 1, sb->data, sb->len);
	timer_add(&ctx->timer, DNS_QUERY_TIMEOUT * 1000, dns_query_timeout_cb,
		  ctx);
	return 0;
}

static void dns_serialize_domain_name(const sbuf_t *name, buf_t *out)
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
	dns->tr_id = ctx->tr_id;
	dns->flags = DNS_FLAG_RECURSION_DESIRED;
	dns->question_cnt = htons(1);
	dns->answer_rrs = 0;
	dns->authority_rrs = 0;
	dns->additional_rrs = 0;
	buf = (buf_t){
		.data = dns->queries,
		.size = name->len + 2
	};
	dns_serialize_domain_name(name, &buf);
	q = buf_data(&buf) + buf_len(&buf);
	*(uint16_t *)q = DNS_TYPE_A;
	q += 2;
	*(uint16_t *)q = DNS_CLASS_IN;
	sbuf_init(&sb, dns, dns_query_len);
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
