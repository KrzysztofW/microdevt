#include <assert.h>
#include <sys/chksum.h>
#include <sys/buf.h>
#include <sys/ring.h>
#include <drivers/rf.h>
#include "swen.h"

/* XXX the size of this structure has to be even (in order to use
 * cksum_partial() on it). */
typedef struct __attribute__((__packed__)) swen_hdr_t {
	uint8_t to;
	uint8_t from;
	uint16_t len;
	uint16_t chksum;
} swen_hdr_t;

#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
typedef struct swen_ctx {
	uint8_t addr; /* XXX to be removed (already in iface) */
	uint8_t from_addr;
	uint8_t valid_data_left;

	void   *rf_handle;
#ifdef CONFIG_RF_GENERIC_COMMANDS
	void (*generic_cmds_cb)(int nb);
	uint8_t *generic_cmds;
#endif
} swen_ctx_t;

void *
swen_init(void *rf_handle, uint8_t addr, void (*generic_cmds_cb)(int nb),
	  uint8_t *cmds)
{
	swen_ctx_t *ctx = malloc(sizeof(swen_ctx_t));

	if (ctx == NULL) {
		DEBUG_LOG("%s: cannot allocate memory\n");
		return NULL;
	}
	ctx->addr = addr;
	ctx->rf_handle = rf_handle;
#ifdef CONFIG_RF_GENERIC_COMMANDS
	ctx->generic_cmds_cb = generic_cmds_cb;
	ctx->generic_cmds = cmds;
#endif
	return ctx;
}

/* Will never be used in an infinite loop. Save space, don't compile it */
#if 0
void swen_shutdown(void *handle)
{
	swen_ctx_t *ctx = handle;

	rf_shutdown(ctx->rf_handle);
	free(ctx);
}
#endif
#endif

#ifdef CONFIG_RF_SENDER
int swen_sendto(void *handle, uint8_t to, const buf_t *data)
{
	swen_ctx_t *ctx = handle;
	swen_hdr_t hdr;
	uint32_t csum;
	buf_t hdr_buf;
	uint16_t frame_len;
	const buf_t *bufs[2];

	hdr.from = ctx->addr;
	hdr.to = to;
	hdr.len = htons(data->len);
	hdr.chksum = 0;

	STATIC_ASSERT(!(sizeof(swen_hdr_t) & 0x1));
	csum = cksum_partial(&hdr, sizeof(swen_hdr_t));
	csum += cksum_partial(data->data, data->len);
	hdr.chksum = cksum_finish(csum);
	buf_init(&hdr_buf, (void *)&hdr, sizeof(hdr));
	frame_len = data->len + sizeof(swen_hdr_t);
	bufs[0] = &hdr_buf;
	bufs[1] = data;

	return rf_output(ctx->rf_handle, frame_len, 2, bufs);
}
#endif

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static int
swen_parse_generic_cmds(swen_ctx_t *ctx, ring_t *ring, int rlen)
{
	uint8_t cmd_index;
	uint8_t *cmds;

	if (ctx->generic_cmds_cb == NULL || ctx->generic_cmds == NULL)
		return -1;

	cmd_index = 0;
	cmds = ctx->generic_cmds;
	while (cmds[0] != 0) {
		uint8_t len = cmds[0];

		/* ring len is the same throughout the loop */
		if (rlen < len)
			return 0;
		cmds++;
		if (ring_cmp(ring, cmds, len) == 0) {
			__ring_skip(ring, len);
			ctx->generic_cmds_cb(cmd_index);
			return 0;
		}
		cmd_index++;
		cmds += len;
	}
	return -1;
}
#endif

int swen_recvfrom(void *handle, uint8_t *from, buf_t *buf)
{
	swen_ctx_t *ctx = handle;
	ring_t *ring = rf_get_ring(ctx->rf_handle);
	uint8_t rlen;

	while ((rlen = ring_len(ring))) {
		if (ctx->valid_data_left) {
			uint8_t l = __ring_get_dont_skip(ring, buf,
							 ctx->valid_data_left);
			ctx->valid_data_left -= l;
			__ring_skip(ring, l);
			*from = ctx->from_addr;
			return 0;
		}

		if (rlen < sizeof(swen_hdr_t))
			return -1;

		/* check if data is for us */
		if (ring_cmp(ring, &ctx->addr, 1) == 0) {
			swen_hdr_t hdr;
			uint16_t hdr_len;
			buf_t b;
			unsigned l;

			buf_init(&b, &hdr, sizeof(swen_hdr_t));
			b.len = 0;
			__ring_get_dont_skip(ring, &b, sizeof(swen_hdr_t));
			hdr_len = ntohs(hdr.len);
			l = hdr_len + sizeof(swen_hdr_t);

			if (l > ring->mask) {
				__ring_skip(ring, 1);
				continue;
			}
			if (rlen < l)
				return -1;
			if (__ring_cksum(ring, l) != 0) {
				__ring_skip(ring, 1);
				continue;
			}

			*from = hdr.from;
			ctx->from_addr = hdr.from;
			__ring_skip(ring, sizeof(swen_hdr_t));
			l = __ring_get_dont_skip(ring, buf, hdr_len);
			ctx->valid_data_left = hdr_len - l;
			__ring_skip(ring, l);
			return 0;
		}
#ifdef CONFIG_RF_GENERIC_COMMANDS
		if (swen_parse_generic_cmds(ctx, ring, rlen) < 0)
#endif
			/* skip 1 byte of garbage */
			__ring_skip(ring, 1);
	}
	return -1;
}
#endif
