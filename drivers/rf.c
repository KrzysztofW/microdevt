#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <timer.h>
#include <sys/byte.h>
#include <sys/chksum.h>
#include "rf.h"
#include "rf_cfg.h"
#include "adc.h"

#if RF_SAMPLING_US < CONFIG_TIMER_RESOLUTION_US
#error "RF sampling will not work with the current timer resolution"
#endif

typedef struct rf_data {
	byte_t  byte;
	ring_t *ring;
	tim_t   timer;
} rf_data_t;

typedef struct rf_ctx {
#ifdef CONFIG_RF_RECEIVER
	rf_data_t rcv_data;
	struct {
		uint8_t   cnt;
		uint8_t   prev_val;
		uint8_t   receiving;
	} rcv;
#endif
#ifdef CONFIG_RF_SENDER
	rf_data_t snd_data;
	uint8_t   burst;
	struct {
		uint16_t  frame_len;
		uint8_t   frame_pos;
		uint8_t   bit;
		uint8_t   clk;
		uint8_t   sending;
		uint8_t   burst_cnt;
		uint8_t   rpos;
	} snd;
#endif
} rf_ctx_t;

#ifdef CONFIG_RF_STATIC_ALLOC
rf_ctx_t __rf_ctx_pool[CONFIG_RF_STATIC_ALLOC];
uint8_t __rf_ctx_pool_pos;
#endif

#ifdef CONFIG_RF_RECEIVER
#ifndef RF_RCV_PIN_NB
#error "RF_RCV_PIN_NB not defined"
#endif
#endif

#ifdef CONFIG_RF_SENDER
#define FRAME_DELIMITER_LENGTH 30 /* loops */
#define START_FRAME_LENGTH 30	  /* loops */

#if !defined(RF_SND_PIN_NB) || !defined(RF_SND_PORT)
#error "RF_SND_PIN and RF_SND_PORT are not defined"
#endif
#ifdef CONFIG_RF_CHECKS
rf_data_t rf_data_check;
#endif
#endif

#ifdef CONFIG_RF_RECEIVER
ring_t *rf_get_ring(void *handle)
{
	rf_ctx_t *ctx = handle;

	return ctx->rcv_data.ring;
}

static int rf_ring_add_bit(rf_data_t *rf, uint8_t bit)
{
	int byte = byte_add_bit(&rf->byte, bit);
	int ret;

	if (byte < 0)
		return 0;

	ret = ring_addc(rf->ring, byte);
	return ret;
}

static void rf_fill_ring(rf_ctx_t *ctx, uint8_t bit)
{
	if (ctx->rcv.receiving && ctx->rcv.cnt < 11) {
		if (ctx->rcv.receiving == 1) {
			if (bit == 0)
				goto end;
			ctx->rcv.receiving = 2;
		}

		if (ctx->rcv.cnt > 3) {
			if (rf_ring_add_bit(&ctx->rcv_data, 1) < 0)
				goto end;
		} else {
			if (rf_ring_add_bit(&ctx->rcv_data, 0) < 0)
				goto end;
		}
		return;
	}

	if (ctx->rcv.cnt >= 55 && ctx->rcv.cnt <= 62) {
		/* frame delimiter is only made of low values */
		if (bit)
			goto end;

		byte_reset(&ctx->rcv_data.byte);
		ctx->rcv.receiving = 1;
		return;
	}
 end:
	ctx->rcv.receiving = 0;
	byte_reset(&ctx->rcv_data.byte);
}

static inline void rf_sample(rf_ctx_t *ctx)
{
	uint8_t v, not_v;
#ifdef RF_ANALOG_SAMPLING
	uint16_t v_analog = analog_read(RF_RCV_PIN_NB);

	if (v_analog < ANALOG_LOW_VAL)
		v = 0;
	else if (v_analog > ANALOG_HI_VAL)
		v = 1;
	else
		return;
#else
	v = RF_RCV_PIN & (1 << RF_RCV_PIN_NB);
#endif
	not_v = !v;
	if (ctx->rcv.prev_val == not_v) {
		ctx->rcv.prev_val = v;
		rf_fill_ring(ctx, not_v);
		ctx->rcv.cnt = 0;
	} else
		ctx->rcv.cnt++;
}
#endif

#ifdef CONFIG_RF_SENDER
/* returns -1 if no data sent */
static int rf_snd(rf_ctx_t *ctx)
{
	if (ctx->snd.bit) {
		ctx->snd.bit--;
		return 0;
	}
	if (ctx->snd.frame_pos < START_FRAME_LENGTH) {
		/* send garbage data to calibrate the RF sender */
		RF_SND_PORT ^= 1 << RF_SND_PIN_NB;
		ctx->snd.frame_pos++;
		return 0;
	}

	if (ctx->snd.frame_pos == START_FRAME_LENGTH) {
		uint8_t fl;
		int rlen = ring_len(ctx->snd_data.ring);

		assert(ctx->snd.frame_len == 0);
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
		if (rlen < sizeof(ctx->snd.frame_len) + 1)
			goto end;

		/* get frame length */
		__ring_getc_at(ctx->snd_data.ring,
			       (uint8_t *)&ctx->snd.frame_len, 0);
		__ring_getc_at(ctx->snd_data.ring, &fl, 1);
		ctx->snd.frame_len |= fl << 8;
		ctx->snd.rpos += 2;

		/* data are being added right now */
		if (rlen < ctx->snd.frame_len)
			goto end;

		ctx->snd.frame_pos++;
		return 0;
	}
	if (ctx->snd.frame_pos
	    <= FRAME_DELIMITER_LENGTH + START_FRAME_LENGTH) {
		ctx->snd.frame_pos++;
		return 0;
	}

	if (byte_is_empty(&ctx->snd_data.byte)) {
		uint8_t c;
		int rlen = ring_len(ctx->snd_data.ring);

		if (rlen == 0) {
			if (!ctx->snd.sending)
				return -1;
			if (RF_SND_PORT & (1 << RF_SND_PIN_NB))
				goto end;
			RF_SND_PORT |= 1 << RF_SND_PIN_NB;
			return 0;
		}
		if (ctx->snd.sending && ctx->snd.frame_len == 0) {
			if (RF_SND_PORT & (1 << RF_SND_PIN_NB)) {
				ctx->snd.sending = 0;
				ctx->snd.frame_pos = START_FRAME_LENGTH;
				if (ctx->snd.burst_cnt >= ctx->burst) {
					__ring_skip(ctx->snd_data.ring, ctx->snd.rpos);
					ctx->snd.burst_cnt = 0;
				} else
					ctx->snd.burst_cnt++;
				ctx->snd.rpos = 0;
			} else
				RF_SND_PORT |= 1 << RF_SND_PIN_NB;
			return 0;
		}
		__ring_getc_at(ctx->snd_data.ring, &c, ctx->snd.rpos);
		ctx->snd.rpos++;
		ctx->snd.frame_len--;
		byte_init(&ctx->snd_data.byte, c);
	}

	/* use 2 loops for '1' value */
	ctx->snd.bit = byte_get_bit(&ctx->snd_data.byte) ? 2 : 0;

#ifdef CONFIG_RF_CHECKS
	rf_ring_add_bit(&rf_data_check, ctx->snd.bit ? 1 : 0);
#endif
	if (ctx->snd.sending == 0)
		ctx->snd.sending = 1;
	else
		ctx->snd.clk ^= 1;

	/* send bit */
	if (ctx->snd.clk == 0)
		RF_SND_PORT |= 1 << RF_SND_PIN_NB;
	else
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);

	return 0;
 end:
	RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	memset(&ctx->snd, 0, sizeof(ctx->snd));

	return -1;
}

static void rf_snd_tim_cb(void *arg);

static void rf_start_sending(rf_ctx_t *ctx)
{
#ifdef CONFIG_RF_RECEIVER
	timer_del(&ctx->rcv_data.timer);
#endif
	if (timer_is_pending(&ctx->snd_data.timer))
		return;
	timer_add(&ctx->snd_data.timer, RF_SAMPLING_US * 2, rf_snd_tim_cb, ctx);
}

int rf_output(void *handle, uint16_t frame_len, uint8_t nb_bufs,
	      const buf_t **bufs)
{
	rf_ctx_t *ctx = handle;
	uint8_t i;

	if (ring_free_entries(ctx->snd_data.ring) < frame_len + 2)
		return -1;

	/* add frame length */
	__ring_addc(ctx->snd_data.ring, frame_len & 0xFF);
	__ring_addc(ctx->snd_data.ring, (frame_len >> 8) & 0xFF);

	/* add the buffers */
	for (i = 0; i < nb_bufs; i++)
		__ring_addbuf(ctx->snd_data.ring, bufs[i]);

	rf_start_sending(ctx);
	return 0;
}
#endif
#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg)
{
	rf_ctx_t *ctx = arg;

	timer_reschedule(&ctx->snd_data.timer, RF_SAMPLING_US * 2);
	if (rf_snd(ctx) < 0) {
#ifdef CONFIG_RF_RECEIVER
		/* enable receiving */
		timer_reschedule(&ctx->rcv_data.timer, RF_SAMPLING_US);
#endif
		timer_del(&ctx->snd_data.timer);
		return;
	}
}

#ifdef CONFIG_RF_CHECKS
static int rf_buffer_checks(rf_ctx_t *ctx, const buf_t *buf)
{
	int i, burst_cnt = ctx->burst;

	if (ring_addc(ctx->snd_data.ring, buf->len & 0xFF) < 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}
	if (ring_addc(ctx->snd_data.ring, buf->len >> 8) < 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}

	if (ring_addbuf(ctx->snd_data.ring, buf) < 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}

	/* send bytes */
	while (rf_snd(ctx) >= 0) {}

	if (!ring_is_empty(ctx->snd_data.ring)) {
		DEBUG_LOG("%s:%d failed (ring len:%d)\n", __func__, __LINE__,
			  ring_len(ctx->snd_data.ring));
		ring_print(ctx->snd_data.ring);
		return -1;
	}

	if (ring_len(rf_data_check.ring) != buf->len * (burst_cnt + 1)) {
		DEBUG_LOG("%s:%d failed (ring check len:%d, expected: %d)\n",
			  __func__, __LINE__, ring_len(rf_data_check.ring),
			  buf->len * (burst_cnt + 1));
		return -1;
	}
	do {
		for (i = 0; i < buf->len; i++) {
			uint8_t c;
			if (ring_getc(rf_data_check.ring, &c) < 0) {
				DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
				return -1;
			}
			if (c != buf->data[i]) {
				DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
				return -1;
			}
		}
	} while (burst_cnt--);

	if (!ring_is_empty(rf_data_check.ring)) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

int rf_checks(void *handle)
{
	rf_ctx_t *ctx = handle;
	uint8_t data[] = {
		0x69, 0x70, 0x00, 0x10, 0xC8, 0xA0, 0x4B, 0xF7,
		0x17, 0x7F, 0xE7, 0x81, 0x92, 0xE4, 0x0E, 0xA3,
		0x83, 0xE7, 0x29, 0x74, 0x34, 0x03
	};
	uint8_t data2[] = {
		0x69, 0x70, 0x00, 0x10, 0xC8, 0xA0, 0x4B, 0xF7,
		0x17, 0x7F, 0xE7, 0x81, 0x92, 0xE4, 0x0E, 0xA3,
		0x00
	};
	buf_t buf;
	uint8_t tim_armed = 0;

	/* save application send timer */
	if (timer_is_pending(&ctx->snd_data.timer)) {
		timer_del(&ctx->snd_data.timer);
		tim_armed = 1;
	}

	buf_init(&buf, data, sizeof(data));
	if (rf_buffer_checks(ctx, &buf) < 0)
		return -1;

	buf_init(&buf, data2, sizeof(data2));
	if (rf_buffer_checks(ctx, &buf) < 0)
		return -1;

	/* restore send timer */
	if (tim_armed)
		rf_start_sending(ctx);
	DEBUG_LOG("=== RF checks passed ===\n");
	return 0;
}
#endif
#endif

#ifdef CONFIG_RF_RECEIVER
static void rf_rcv_tim_cb(void *arg)
{
	rf_ctx_t *ctx = arg;

	timer_reschedule(&ctx->rcv_data.timer, RF_SAMPLING_US);
	rf_sample(ctx);
}
#endif

void *rf_init(uint8_t burst)
{
	rf_ctx_t *ctx;

#ifndef CONFIG_RF_STATIC_ALLOC
	if ((ctx = calloc(1, sizeof(rf_ctx_t))) == NULL) {
		DEBUG_LOG("%s: cannot allocate memory\n", __func__);
		return NULL;
	}
#else
	if (__rf_ctx_pool_pos >= CONFIG_RF_STATIC_ALLOC) {
		DEBUG_LOG("%s: too many allocations\n", __func__);
		abort();
	}
	ctx = &__rf_ctx_pool[__rf_ctx_pool_pos++];
#endif

#ifdef CONFIG_RF_RECEIVER
#ifndef CONFIG_RING_STATIC_ALLOC
	if ((ctx->rcv_data.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("%s: cannot create receive RF ring\n", __func__);
		return NULL;
	}
#else
	ctx->rcv_data.ring = ring_create(RF_RING_SIZE);
#endif
	timer_add(&ctx->rcv_data.timer, RF_SAMPLING_US, rf_rcv_tim_cb, ctx);
#ifndef RF_ANALOG_SAMPLING
	RF_RCV_PORT &= ~(1 << RF_RCV_PIN_NB);
#endif
#endif
#ifdef CONFIG_RF_SENDER
#ifndef CONFIG_RING_STATIC_ALLOC
	if ((ctx->snd_data.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("%s: cannot create send RF ring\n", __func__);
		return NULL;
	}
#else
	ctx->snd_data.ring = ring_create(RF_RING_SIZE);
#endif
	if (burst)
		ctx->burst = burst - 1;
#ifdef CONFIG_RF_CHECKS
	if ((rf_data_check.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("%s: cannot create check RF ring\n", __func__);
		return NULL;
	}
#endif
#endif
	return ctx;
}


#if 0 /* Will never be used in an infinite loop. Save space, don't compile it */
void rf_shutdown(void *handle)
{
	rf_ctx_t *ctx = handle;

#ifdef CONFIG_RF_RECEIVER
	timer_del(&ctx->rcv_data.timer);
	ring_free(ctx->rcv_data.ring);
#endif
#ifdef CONFIG_RF_SENDER
	timer_del(&ctx->snd_data.timer);
	ring_free(ctx->snd_data.ring);
#ifdef CONFIG_RF_CHECKS
	ring_free(rf_data_check.ring);
#endif
#endif
#ifndef CONFIG_RF_STATIC_ALLOC
	free(ctx);
#else
	assert(__rf_ctx_pool_pos);
	__rf_ctx_pool_pos--;
	memset(ctx, 0, sizeof(rf_ctx_t));
#endif
}
#endif
