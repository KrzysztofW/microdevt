/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#include <stdio.h>
#include <string.h>
#include <sys/chksum.h>
#ifdef CONFIG_RND_SEED
#include <sys/random.h>
#endif
#include <sys/scheduler.h>
#include "rf-pwm.h"
#include "rf-cfg.h"
#ifndef X86
#include "adc.h"
#endif
#if RF_SAMPLING_US < CONFIG_TIMER_RESOLUTION_US
#error "RF sampling will not work with the current timer resolution"
#endif

/* #define RF_DEBUG */
#ifdef RF_DEBUG
extern iface_t *rf_debug_iface1;
extern iface_t *rf_debug_iface2;
#endif

#ifdef CONFIG_RF_RECEIVER
#ifndef RF_RCV_PIN_NB
#error "RF_RCV_PIN_NB not defined"
#endif
#endif

#ifdef CONFIG_RF_SENDER
#define FRAME_DELIMITER_LENGTH 29 /* loops */
#define START_FRAME_LENGTH 30	  /* loops */

#if !defined(RF_SND_PIN_NB) || !defined(RF_SND_PORT)
#error "RF_SND_PIN and RF_SND_PORT are not defined"
#endif
#ifdef CONFIG_RF_CHECKS
extern pkt_t *pkt_recv;
extern rf_ctx_t rf_check_recv_ctx;
#endif
#endif

#ifdef CONFIG_RF_RECEIVER

void rf_input(const iface_t *iface) {}

static int rf_add_bit(rf_ctx_t *ctx, uint8_t bit)
{
	int byte = byte_add_bit(&ctx->rcv_data.byte, bit);

	if (byte < 0)
		return 0;

	return buf_addc(&ctx->rcv_data.pkt->buf, byte);
}

#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg);

static void __rf_start_sending(iface_t *iface)
{
	if (ring_len(iface->tx)) {
		rf_ctx_t *ctx = iface->priv;

		timer_del(&ctx->timer);
		rf_snd_tim_cb(iface);
	}
}
#endif

static void rf_fill_data(const iface_t *iface, uint8_t bit)
{
	rf_ctx_t *ctx = iface->priv;
#ifdef CONFIG_RND_SEED
	static uint16_t rnd;

	rnd <<= 1;
	rnd |= bit;
	rnd_seed *= 16807 * rnd;
#endif
	if (ctx->rcv.receiving && ctx->rcv.cnt < 11) {
		if (ctx->rcv.receiving == 1) {
			/* first bit must be set to '1' */
			if (bit == 0)
				goto end;
			ctx->rcv.receiving = 2;
		}

		if (rf_add_bit(ctx, (ctx->rcv.cnt > 3)) < 0)
			goto end;
		return;
	}

	if (ctx->rcv.cnt >= 55 && ctx->rcv.cnt <= 62) {
		/* frame delimiter is only made of low values */
		if (bit)
			goto end;

		byte_reset(&ctx->rcv_data.byte);
		if (ctx->rcv.receiving) {
			if_schedule_receive(iface, ctx->rcv_data.pkt);
			ctx->rcv_data.pkt = NULL;
		}
		if (ctx->rcv_data.pkt == NULL) {
			if ((ctx->rcv_data.pkt = pkt_get(iface->pkt_pool))
			    == NULL) {
				if_schedule_receive(iface, NULL);
				DEBUG_LOG("%s: cannot alloc pkt\n", __func__);
				goto end;
			}
		} else
			buf_reset(&ctx->rcv_data.pkt->buf);
		ctx->rcv.receiving = 1;
		return;
	}
	if (ctx->rcv.receiving && ctx->rcv_data.pkt
	    && pkt_len(ctx->rcv_data.pkt)) {
		if_schedule_receive(iface, ctx->rcv_data.pkt);
		ctx->rcv_data.pkt = NULL;
	}
 end:
	ctx->rcv.receiving = 0;
	byte_reset(&ctx->rcv_data.byte);
#ifdef CONFIG_RF_SENDER
	__rf_start_sending((iface_t *)iface);
#endif
}

static void rf_rcv_tim_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
	uint8_t v, not_v;

	timer_reschedule(&ctx->timer, RF_SAMPLING_US);
	v = RF_RCV_PIN & (1 << RF_RCV_PIN_NB);
	not_v = !v;

	if (ctx->rcv.prev_val == not_v) {
		ctx->rcv.prev_val = v;
		rf_fill_data(iface, not_v);
		ctx->rcv.cnt = 0;
	} else
		ctx->rcv.cnt++;
}
#endif

#ifdef RF_DEBUG
static int rf_debug_send(rf_ctx_t *src, uint8_t byte)
{
	rf_ctx_t *dst;
	iface_t *iface;

	if (src == rf_debug_iface2->priv) {
		dst = rf_debug_iface1->priv;
		iface = rf_debug_iface1;
	}  else if (src == rf_debug_iface1->priv) {
		dst = rf_debug_iface2->priv;
		iface = rf_debug_iface2;
	} else
		__abort();

	if (dst->rcv_data.pkt == NULL) {
		if ((dst->rcv_data.pkt = pkt_get(iface->pkt_pool)) == NULL) {
			DEBUG_LOG("%s no pkts\n", __func__);
			if_schedule_receive(iface, NULL);
			return -1;
		}
	}
	if (buf_addc(&dst->rcv_data.pkt->buf, byte) < 0) {
		DEBUG_LOG("%s: buf len:%d\n", __func__,
			  dst->rcv_data.pkt->buf.len);
		return -1;
	}
	return 0;
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
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
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

		if (ctx->snd_data.buf.len == 0) {
			if ((RF_SND_PORT & (1 << RF_SND_PIN_NB)) == 0) {
				RF_SND_PORT |= 1 << RF_SND_PIN_NB;
				return 0;
			}
#ifndef CONFIG_RF_BURST
			return -1;
#else
			if (ctx->snd.burst_cnt == ctx->burst)
				return -1;
			ctx->snd.burst_cnt++;
			__buf_reset_keep(&ctx->snd_data.buf);
			ctx->snd.frame_pos = START_FRAME_LENGTH;
			return 0;
#endif
		}
		__buf_getc(&ctx->snd_data.buf, &c);
		byte_init(&ctx->snd_data.byte, c);
#ifdef RF_DEBUG
		if (rf_debug_send(ctx, c) < 0)
			return -1;
#endif
	}

	/* use 2 loops for '1' value */
	ctx->snd.bit = byte_get_bit(&ctx->snd_data.byte) ? 2 : 0;

#ifdef CONFIG_RF_CHECKS
	rf_add_bit(&rf_check_recv_ctx, ctx->snd.bit ? 1 : 0);
#endif

	/* send bit */
	if (ctx->snd.clk == 0)
		RF_SND_PORT |= 1 << RF_SND_PIN_NB;
	else
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	ctx->snd.clk ^= 1;
	return 0;
}

static void rf_start_sending(const iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	if (ctx->rcv.receiving)
		return;
	timer_del(&ctx->timer);
	timer_add(&ctx->timer, RF_SAMPLING_US * 2, rf_snd_tim_cb,
		  (void *)iface);
}

int rf_output(const iface_t *iface, pkt_t *pkt)
{
	if (pkt_put(iface->tx, pkt) < 0)
		return -1;
	rf_start_sending(iface);
	return 0;
}
#endif

#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
#ifdef RF_DEBUG
	rf_ctx_t *debug_ctx1 = rf_debug_iface1->priv;
	rf_ctx_t *debug_ctx2 = rf_debug_iface2->priv;
#endif
	while (1) {
		if (ctx->snd_data.pkt == NULL) {
			pkt_t *pkt;

			if ((ctx->snd_data.pkt = pkt_get(iface->tx)) == NULL)
				break;
			pkt = ctx->snd_data.pkt;
			buf_init(&ctx->snd_data.buf, pkt->buf.data,
				 pkt->buf.len);
		}
		if (rf_snd(ctx) >= 0) {
			timer_add(&ctx->timer, RF_SAMPLING_US * 2,
				  rf_snd_tim_cb, arg);
			return;
		}
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
		if_schedule_tx_pkt_free(ctx->snd_data.pkt);
		memset(&ctx->snd, 0, sizeof(ctx->snd));
		ctx->snd_data.pkt = NULL;
		ctx->snd.frame_pos = START_FRAME_LENGTH;
#ifdef RF_DEBUG
		if (iface == rf_debug_iface1) {
			if_schedule_receive(rf_debug_iface2,
					    debug_ctx2->rcv_data.pkt);
			debug_ctx2->rcv_data.pkt = NULL;
		} else {
			if_schedule_receive(rf_debug_iface1,
					    debug_ctx1->rcv_data.pkt);
			debug_ctx1->rcv_data.pkt = NULL;
		}
#endif
	}
	ctx->snd.frame_pos = 0;
#ifdef CONFIG_RF_RECEIVER
#ifndef RF_DEBUG
	timer_add(&ctx->timer, RF_SAMPLING_US, rf_rcv_tim_cb, arg);
#endif
#endif
}
#endif

void rf_init(iface_t *iface, rf_ctx_t *ctx, uint8_t burst)
{
	timer_init(&ctx->timer);
#if defined (CONFIG_RF_SENDER) && defined (CONFIG_RF_BURST)
#ifdef RF_DEBUG
	ctx->burst = 0;
#else
	if (burst)
		ctx->burst = burst - 1;
#endif
#endif
	iface->priv = ctx;
#if defined (CONFIG_RF_RECEIVER) && !defined(X86) && !defined(RF_DEBUG)
	timer_add(&ctx->timer, RF_SAMPLING_US, rf_rcv_tim_cb, iface);
#else
	(void)rf_rcv_tim_cb;
#endif
}

/* XXX Will never be used in an infinite loop. Save space, don't compile it */
#ifdef TEST
void rf_shutdown(const iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	timer_del(&ctx->timer);
}
#endif
