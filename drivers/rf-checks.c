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

#include <sys/timer.h>
#include <sys/scheduler.h>
#include <net/event.h>
#include "rf.h"

pkt_t *pkt_recv;
rf_ctx_t rf_check_recv_ctx;

#ifdef CONFIG_RF_SENDER
static int rf_send_checks(iface_t *iface)
{
	uint8_t data[] = {
		0x69, 0x70, 0x00, 0x10, 0xC8, 0xA0, 0x4B, 0xF7,
		0x17, 0x7F, 0xE7, 0x81, 0x92, 0xE4, 0x0E, 0xA3,
		0x83, 0xE7, 0x29, 0x74, 0x34, 0x03
	};
	rf_ctx_t *ctx = iface->priv;
	pkt_t *pkt;

	pkt = pkt_alloc();
	pkt_recv = pkt_alloc();

	if (pkt == NULL || pkt_recv == NULL) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}
	rf_check_recv_ctx.recv_data.pkt = pkt_recv;

	buf_add(&pkt->buf, data, sizeof(data));
	timer_del(&ctx->timer);
	if (rf_output(iface, pkt) < 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}
	timer_del(&ctx->timer);
	while (ctx->snd_data.pkt) {
		rf_snd(iface);
		timer_del(&ctx->timer);
	}
	if (buf_cmp(&pkt->buf, &pkt_recv->buf) != 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}

	memset(&ctx->snd_data, 0, sizeof(ctx->snd_data));

	/* free pkt */
	scheduler_run_task();
	pkt_free(pkt_recv);

	return 0;
}
#endif

int rf_checks(iface_t *iface)
{
#ifdef CONFIG_RF_SENDER
	if (rf_send_checks(iface) < 0)
		return -1;
#endif
#ifndef TEST
	DEBUG_LOG("RF-send checks succeeded\n");
#endif
	return 0;
}
