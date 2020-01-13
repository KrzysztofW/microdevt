/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2020, Krzysztof Witek
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

#include <sys/timer.h>
#include <sys/scheduler.h>
#include <interrupts.h>
#ifdef CONFIG_RND_SEED
#include <sys/random.h>
#endif
#include "rf.h"
#include "rf-cfg.h"
#ifdef CONFIG_RF_CHECKS
#include "rf-checks.h"
#endif

/* #define RF_DEBUG */
#ifdef RF_DEBUG
extern iface_t *rf_debug_iface1;
extern iface_t *rf_debug_iface2;
#endif

#ifdef CONFIG_RF_SENDER
static void rf_start_sending(iface_t *iface);

#ifdef RF_WAIT_BEFORE_SENDING
static void rf_snd_delay_cb(void *arg)
{
#ifndef CONFIG_RF_RECEIVER
	iface_t *iface = arg;

	rf_start_sending(iface);
#endif
}
#endif
#endif

#ifdef CONFIG_RF_RECEIVER
static void rf_recv_sync_cb(void *arg);

static int rf_add_bit(rf_ctx_t *ctx, uint8_t bit)
{
	int byte = byte_add_bit(&ctx->recv_data.byte, bit);

	if (byte < 0)
		return 0;

	return buf_addc(&ctx->recv_data.pkt->buf, byte);
}

static void rf_init_receiver(iface_t *iface);

static void rf_recv_finish(iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	ctx->cnt = 0;
	byte_reset(&ctx->recv_data.byte);
	if (ctx->recv_data.pkt == NULL || pkt_len(ctx->recv_data.pkt) == 0)
		goto end;

#ifdef CONFIG_IFACE_STATS
	iface->rx_packets++;
#endif
	if_schedule_receive(iface, &ctx->recv_data.pkt);

#if defined(CONFIG_RF_SENDER) && defined(RF_FINISH_DELAY)
	timer_del(&ctx->wait_before_sending_timer);
	timer_add(&ctx->wait_before_sending_timer, RF_FINISH_DELAY,
		  rf_snd_delay_cb, NULL);
#endif
	/* we can't send yet but we can still receive */
 end:
	rf_init_receiver(iface);
}

static void rf_recv_data_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
	uint8_t v = RF_RCV_PIN & (1 << RF_RCV_PIN_NB);

	if (v == ctx->prev_val) {
		ctx->cnt++;
		if (ctx->cnt > RF_LOW_TICKS + RF_HI_TICKS + 1) {
			rf_recv_finish(iface);
			return;
		}
		goto end;
	}

	if (ctx->cnt > RF_LOW_TICKS + RF_HI_TICKS + 1 ||
	    (ctx->cnt > 1 && rf_add_bit(ctx, !!ctx->prev_val) < 0)) {
#ifdef CONFIG_IFACE_STATS
		iface->rx_errors++;
#endif
		rf_recv_finish(iface);
		return;
	}
	ctx->prev_val = v;
	ctx->cnt = 0;
 end:
	timer_add(&ctx->timer, RF_PULSE_WIDTH, rf_recv_data_cb, iface);
}

static void rf_recv_sync_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
	uint8_t v = RF_RCV_PIN & (1 << RF_RCV_PIN_NB);
	void (*cb)(void *);
#ifdef CONFIG_RND_SEED
	static uint16_t rnd;

	rnd <<= 1;
	rnd |= !!v;
	rnd_seed *= rnd;
#endif

#ifdef CONFIG_RF_SENDER
	if (ring_len(iface->tx)) {
#ifdef RF_FINISH_DELAY
		if (!timer_is_pending(&ctx->wait_before_sending_timer)) {
			rf_start_sending(iface);
			return;
		}
#else
		rf_start_sending(iface);
		return;
#endif
	}
#endif
	cb = rf_recv_sync_cb;
	if (v == 0) {
		ctx->cnt++;
		goto end;
	}
	if (ctx->cnt < RF_RESET_TICKS - 2) {
		ctx->cnt = 0;
		goto end;
	}

	if (ctx->recv_data.pkt == NULL &&
	    (ctx->recv_data.pkt = pkt_get(iface->pkt_pool)) == NULL) {
		DEBUG_LOG("RF receiver: cannot alloc pkt\n");
#ifdef CONFIG_IFACE_STATS
		iface->rx_errors++;
#endif
		if_schedule_receive(iface, NULL);
		goto end;
	}

	cb = rf_recv_data_cb;
	ctx->cnt = 0;
	ctx->prev_val = 1;
 end:
	timer_add(&ctx->timer, RF_PULSE_WIDTH, cb, iface);
}
#endif

#ifdef CONFIG_RF_RECEIVER
static void rf_init_receiver(iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	timer_add(&ctx->timer, RF_PULSE_WIDTH, rf_recv_sync_cb, iface);
}
#endif

#ifdef CONFIG_RF_SENDER
static void rf_snd_cb(void *arg);
#ifdef CONFIG_RF_CHECKS
void rf_snd(void *arg)
{
	rf_snd_cb(arg);
}
#endif

static void rf_snd_low_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
	unsigned long delay;

	RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	if (ctx->flags)
		delay = RF_PULSE_WIDTH;
	else
		delay = RF_PULSE_WIDTH * RF_HI_TICKS;
	timer_add(&ctx->timer, delay + RF_PULSE_WIDTH, rf_snd_cb, iface);
}

static void rf_snd_sync_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;

	RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	timer_add(&ctx->timer, RF_PULSE_WIDTH * RF_RESET_TICKS, rf_snd_cb,
		  iface);
}

static void rf_snd_calibrate_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;

	RF_SND_PORT ^= 1 << RF_SND_PIN_NB;
	if (ctx->cnt < (RF_HI_TICKS + RF_LOW_TICKS) * 10) {
		timer_add(&ctx->timer, RF_PULSE_WIDTH, rf_snd_calibrate_cb,
			  iface);
		ctx->cnt++;
		return;
	}
	timer_add(&ctx->timer, RF_PULSE_WIDTH, rf_snd_sync_cb, iface);
}

static void rf_snd_finish(void *arg)
{
	iface_t *iface = arg;
#ifdef CONFIG_RF_SENDER
	rf_ctx_t *ctx = iface->priv;
#endif

#ifdef CONFIG_RF_RECEIVER
	rf_init_receiver(iface);
#endif
	RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
#if defined(CONFIG_RF_SENDER) && defined(RF_DELAY_BETWEEN_SENDS)
	timer_del(&ctx->wait_before_sending_timer);
	timer_add(&ctx->wait_before_sending_timer, RF_DELAY_BETWEEN_SENDS,
		  rf_snd_delay_cb, NULL);
#elif defined(CONFIG_RF_SENDER)
	if ((ctx->snd_data.pkt = pkt_get(iface->tx)) != NULL) {
		ctx->snd_buf = ctx->snd_data.pkt->buf;
		timer_add(&ctx->timer, RF_PULSE_WIDTH, rf_snd_sync_cb,
			  iface);
	}
#endif
}

static void rf_snd_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
	uint8_t bit;
	unsigned long delay;

	if (byte_is_empty(&ctx->snd_data.byte)) {
		uint8_t byte;

		if (buf_getc(&ctx->snd_buf, &byte) < 0) {
#ifdef CONFIG_IFACE_STATS
			iface->tx_packets++;
#endif
			if_schedule_tx_pkt_free(&ctx->snd_data.pkt);
			RF_SND_PORT |= 1 << RF_SND_PIN_NB;

			timer_add(&ctx->timer, RF_PULSE_WIDTH, rf_snd_finish,
				  iface);
			return;
		}
		byte_init(&ctx->snd_data.byte, byte);
	}
	RF_SND_PORT |= 1 << RF_SND_PIN_NB;

	bit = __byte_get_bit(&ctx->snd_data.byte);
#ifdef CONFIG_RF_CHECKS
	rf_add_bit(&rf_check_recv_ctx, bit);
#endif

	if (bit) {
		delay = RF_PULSE_WIDTH * RF_HI_TICKS;
		ctx->flags = RF_FLAG_SENDING_HI;
	} else {
		delay = RF_PULSE_WIDTH;
		ctx->flags = RF_FLAG_NONE;
	}
	timer_add(&ctx->timer, delay, rf_snd_low_cb, iface);
}

static void rf_start_sending(iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	if (timer_is_pending(&ctx->timer))
		return;

	if ((ctx->snd_data.pkt = pkt_get(iface->tx)) == NULL) {
		/* nothing to send */
#ifdef CONFIG_RF_RECEIVER
		rf_init_receiver(iface);
#endif
		return;
	}
	ctx->snd_buf = ctx->snd_data.pkt->buf;

	ctx->cnt = 0;
	timer_add(&ctx->timer, RF_PULSE_WIDTH * 8, rf_snd_calibrate_cb, iface);
}

#ifdef RF_DEBUG
static int rf_debug_output(iface_t *iface, pkt_t *pkt)
{
	/* We need to allocate a new pkt as this one is used to store seqid
	 * for retransmissions. The seqid would be overrwitten otherwise. */
	pkt_t *new_pkt = pkt_alloc();

	if (new_pkt == NULL) {
		DEBUG_LOG("RF debug: cannot alloc pkt\n");
		return -1;
	}
	__buf_addbuf(&new_pkt->buf, &pkt->buf);
	if (iface == rf_debug_iface1)
		if_schedule_receive(rf_debug_iface2, &new_pkt);
	else
		if_schedule_receive(rf_debug_iface1, &new_pkt);
	pkt_free(pkt);
	return 0;
}
#endif

int rf_output(iface_t *iface, pkt_t *pkt)
{
#ifdef RF_DEBUG
	return rf_debug_output(iface, pkt);
#endif
	if (pkt_put(iface->tx, pkt) < 0) {
#ifdef CONFIG_IFACE_STATS
		iface->tx_dropped++;
#endif
		return -1;
	}
#if !defined(CONFIG_RF_RECEIVER) || defined(CONFIG_RF_CHECKS)
	rf_start_sending(iface);
#endif
	return 0;
}
#endif

void rf_init(iface_t *iface, rf_ctx_t *ctx)
{
	/* ctx must be memset() to 0 by the caller */

	STATIC_ASSERT(CONFIG_TIMER_RESOLUTION_US <= RF_PULSE_WIDTH);
	iface->priv = ctx;
	timer_init(&ctx->timer);
#ifdef CONFIG_RF_RECEIVER
	rf_init_receiver(iface);
#endif
#ifdef RF_WAIT_BEFORE_SENDING
	timer_init(&ctx->wait_before_sending_timer);
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
