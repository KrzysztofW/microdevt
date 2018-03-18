#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <sys/ring.h>
#include <timer.h>
#include <sys/byte.h>
#include <sys/chksum.h>
#include "rf.h"
#include "rf_cfg.h"
#include "adc.h"

#define RF_SAMPLING_US 150
#define RF_RING_SIZE    64

#define ANALOG_LOW_VAL 170
#define ANALOG_HI_VAL  650

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
	rf_data_t rcv;
	uint16_t cnt;
	uint8_t prev_val;
	uint8_t receiving : 2;
#endif
#ifdef CONFIG_RF_SENDER
	rf_data_t snd;
	uint16_t frame_length;
	uint8_t frame_delim_started;
	uint8_t bit : 2;
	uint8_t clk : 1;
	uint8_t sending : 1;
#endif
} rf_ctx_t;

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
#endif

#ifdef CONFIG_RF_RECEIVER
ring_t *rf_get_ring(void *handle)
{
	rf_ctx_t *ctx = handle;

	return ctx->rcv.ring;
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
	if (ctx->receiving && ctx->cnt < 11) {
		if (ctx->receiving == 1) {
			if (bit == 0)
				goto end;
			ctx->receiving = 2;
		}

		if (ctx->cnt > 3) {
			if (rf_ring_add_bit(&ctx->rcv, 1) < 0)
				goto end;
		} else {
			if (rf_ring_add_bit(&ctx->rcv, 0) < 0)
				goto end;
		}
		return;
	}

	if (ctx->cnt >= 55 && ctx->cnt <= 62) {
		/* frame delimiter is only made of low values */
		if (bit)
			goto end;

		byte_reset(&ctx->rcv.byte);
		ctx->receiving = 1;
		return;
	}
 end:
	ctx->receiving = 0;
	byte_reset(&ctx->rcv.byte);
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
	if (ctx->prev_val == not_v) {
		ctx->prev_val = v;
		rf_fill_ring(ctx, not_v);
		ctx->cnt = 0;
	} else
		ctx->cnt++;
}
#endif

#ifdef CONFIG_RF_SENDER
/* returns -1 if no data sent */
static int rf_snd(rf_ctx_t *ctx)
{
	if (ctx->bit) {
		ctx->bit--;
		return 0;
	}

	if (ctx->frame_delim_started < START_FRAME_LENGTH) {
		/* send garbage data to calibrate the RF sender */
		RF_SND_PORT ^= 1 << RF_SND_PIN_NB;
		ctx->frame_delim_started++;
		return 0;
	}

	if (ctx->frame_delim_started == START_FRAME_LENGTH) {
		uint8_t fl;

		ctx->frame_length = 0;
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);

		/* get frame length */
		if (ring_getc(ctx->snd.ring, (uint8_t *)&ctx->frame_length) < 0 ||
		    ring_getc(ctx->snd.ring, &fl) < 0)
			goto end;

		ctx->frame_length |= fl << 8;
		ctx->frame_delim_started++;
		return 0;
	}
	if (ctx->frame_delim_started
	    <= FRAME_DELIMITER_LENGTH + START_FRAME_LENGTH) {
		ctx->frame_delim_started++;
		return 0;
	}

	if (byte_is_empty(&ctx->snd.byte)) {
		uint8_t c;

		if (ctx->sending && ctx->frame_length == 0
		    && ring_len(ctx->snd.ring)) {
			if (RF_SND_PORT & (1 << RF_SND_PIN_NB)) {
				ctx->sending = 0;
				ctx->frame_delim_started = START_FRAME_LENGTH;
			} else
				RF_SND_PORT |= 1 << RF_SND_PIN_NB;
			return 0;
		}

		if (ring_getc(ctx->snd.ring, &c) < 0) {
			if (!ctx->sending)
				return -1;
			if (RF_SND_PORT & (1 << RF_SND_PIN_NB))
				goto end;
			RF_SND_PORT |= 1 << RF_SND_PIN_NB;
			return 0;
		}
		ctx->frame_length--;
		byte_init(&ctx->snd.byte, c);
	}

	/* use 2 loops for '1' value */
	ctx->bit = byte_get_bit(&ctx->snd.byte) ? 2 : 0;

	if (ctx->sending == 0) {
		ctx->clk = 1;
		ctx->sending = 1;
	} else
		ctx->clk ^= 1;

	/* send bit */
	if (ctx->clk)
		RF_SND_PORT |= 1 << RF_SND_PIN_NB;
	else
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	return 0;
 end:
	RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	ctx->frame_delim_started = 0;
	ctx->sending = 0;
	ctx->bit = 0;
	return -1;
}

static void rf_snd_tim_cb(void *arg);

void rf_start_sending(rf_ctx_t *ctx)
{
#ifdef CONFIG_RF_RECEIVER
	timer_del(&ctx->rcv.timer);
#endif
	if (timer_is_pending(&ctx->snd.timer))
		return;
	timer_add(&ctx->snd.timer, RF_SAMPLING_US * 2, rf_snd_tim_cb, ctx);
}

int rf_send(void *handle, uint16_t len, int8_t burst, ...)
{
	uint16_t frame_lengths = (len + 2) * burst;
	rf_ctx_t *ctx = handle;

	if (timer_is_pending(&ctx->snd.timer) ||
	    ring_free_entries(ctx->snd.ring) < frame_lengths)
		return -1;

	/* send the same command 'burst' times as RF is not very reliable */
	while (burst) {
		va_list args;
		buf_t *buf;
#ifdef DEBUG
		uint16_t len2 = 0;
#endif
		burst--;

		/* add frame length */
		__ring_addc(ctx->snd.ring, len & 0xFF);
		__ring_addc(ctx->snd.ring, (len >> 8) & 0xFF);

		va_start(args, burst);
		while ((buf = va_arg(args, buf_t *))) {
#ifdef DEBUG
			len2 += buf_len(buf);
#endif
			/* add the buffer */
			__ring_addbuf(ctx->snd.ring, buf);
		}
		assert(len == len2);
		va_end(args);
	}

	rf_start_sending(ctx);
	return 0;
}
#endif
#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg)
{
	rf_ctx_t *ctx = arg;

	timer_reschedule(&ctx->snd.timer, RF_SAMPLING_US * 2);
	if (rf_snd(ctx) < 0) {
#ifdef CONFIG_RF_RECEIVER
		/* enable receiving */
		timer_reschedule(&ctx->rcv.timer, RF_SAMPLING_US);
#endif
		timer_del(&ctx->snd.timer);
		return;
	}
}
#endif

#ifdef CONFIG_RF_RECEIVER
static void rf_rcv_tim_cb(void *arg)
{
	rf_ctx_t *ctx = arg;

	timer_reschedule(&ctx->rcv.timer, RF_SAMPLING_US);
	rf_sample(ctx);
}
#endif

void *rf_init(void)
{
	rf_ctx_t *ctx;

	if ((ctx = calloc(1, sizeof(rf_ctx_t))) == NULL) {
		DEBUG_LOG("%s: cannot allocate memory\n");
		return NULL;
	}
#ifdef CONFIG_RF_RECEIVER
	if ((ctx->rcv.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("%s: cannot create receive RF ring\n");
		return NULL;
	}
	timer_add(&ctx->rcv.timer, RF_SAMPLING_US, rf_rcv_tim_cb, ctx);
#ifndef RF_ANALOG_SAMPLING
	RF_RCV_PORT &= ~(1 << RF_RCV_PIN_NB);
#endif
#endif
#ifdef CONFIG_RF_SENDER
	if ((ctx->snd.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("%s: cannot create send RF ring\n");
		return NULL;
	}
#endif
	return ctx;
}


#if 0 /* Will never be used in an infinite loop. Save space, don't compile it */
void rf_shutdown(void *handle)
{
	rf_ctx_t *ctx = handle;

#ifdef CONFIG_RF_RECEIVER
	timer_del(&ctx->rcv.timer);
	free(ctx->rcv.ring);
#endif
#ifdef CONFIG_RF_SENDER
	timer_del(&ctx->snd.timer);
	free(ctx->snd.ring);
#endif
}
#endif
