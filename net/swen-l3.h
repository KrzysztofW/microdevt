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

#ifndef _SWEN_L3_H_
#define _SWEN_L3_H_

#include <sys/timer.h>
#include "pkt-mempool.h"
#include "if.h"
#include "swen.h"
#include "event.h"

typedef enum swen_l3_state {
	S_STATE_CLOSED,
	S_STATE_CLOSING,
	S_STATE_CONNECTING,
	S_STATE_CONN_COMPLETE,
	S_STATE_CONNECTED,
} swen_l3_state_t;

typedef struct swen_l3_assoc {
	uint8_t dst;
	uint8_t seq_id;
	uint8_t ack;
	uint8_t new_ack;
	uint8_t ack_needed;
	uint8_t state;
	/* uint8_t win_size; */
	tim_t timer;
	list_t list;
	list_t retrn_pkts;
	list_t incoming_pkts;
#ifdef CONFIG_EVENT
	event_t event;
#endif
	const iface_t *iface;
	const uint32_t *enc_key;
} swen_l3_assoc_t;

/** Get association state
 *
 * @param[in] assoc   association
 * @return association state
 */
static inline uint8_t swen_l3_get_state(swen_l3_assoc_t *assoc)
{
	return assoc->state;
}

/** Initialize an association
 *
 * @param[in] assoc   association
 * @param[in] enc_key encryption key
 */
void swen_l3_assoc_init(swen_l3_assoc_t *assoc, const uint32_t *enc_key);

/** Shutdown association
 *
 * @param[in] assoc   association
 */
void swen_l3_assoc_shutdown(swen_l3_assoc_t *assoc);

/** Associate with peer
 *
 * @param[in] assoc   association
 * @return 0 on success, -1 on failure
 */
int swen_l3_associate(swen_l3_assoc_t *assoc);

/** Disassociate from peer
 *
 * @param[in] assoc   association
 */
void swen_l3_disassociate(swen_l3_assoc_t *assoc);

/** Check if an association is made
 *
 * @param[in] assoc   association
 * @return 0 if not bound, 1 if bound
 */
static inline uint8_t swen_l3_is_assoc_bound(swen_l3_assoc_t *assoc)
{
	return !list_empty(&assoc->list);
}

/** Bind
 *
 * Setup destination address and interface
 * @param[in] assoc   association
 * @param[in] to      remote address
 * @param[in] iface   interface
 */
void
swen_l3_assoc_bind(swen_l3_assoc_t *assoc, uint8_t to, const iface_t *iface);

void swen_l3_input(uint8_t from, pkt_t *pkt, const iface_t *iface);

/** Send static buffer
 *
 * @param[in] assoc   association
 * @param[in] sbuf    static buffer to send
 * @return 0 on success, -1 on failure
 */
int swen_l3_send(swen_l3_assoc_t *assoc, const sbuf_t *sbuf);

/** Send buffer
 *
 * @param[in] assoc   association
 * @param[in] buf     buffer to send
 * @return 0 on success, -1 on failure
 */
static inline int
swen_l3_send_buf(swen_l3_assoc_t *assoc, const buf_t *buf)
{
	sbuf_t sbuf = buf2sbuf(buf);

	return swen_l3_send(assoc, &sbuf);
}
#ifdef TEST
void swen_l3_retransmit_pkts(swen_l3_assoc_t *assoc);
#endif

/** Get received packets
 *
 * @param[in] assoc   association
 * @return received packet or NULL if no packet has been received
 */
pkt_t *swen_l3_get_pkt(swen_l3_assoc_t *assoc);

#ifdef CONFIG_EVENT

/** Register events
 *
 * @param[in] assoc   association
 * @param[in] events  events to wake up on
 * @param[in] ev_cb   function to be called on wake up
 */
static inline void
swen_l3_event_register(swen_l3_assoc_t *assoc, uint8_t events,
		       void (*ev_cb)(event_t *ev, uint8_t events))
{
	event_register(&assoc->event, events, &assoc->incoming_pkts, ev_cb);
}

/** Unregister events
 *
 * @param[in] assoc   association
 */
static inline void
swen_l3_event_unregister(swen_l3_assoc_t *assoc)
{
	event_unregister(&assoc->event);
}

/** Set event mask
 *
 * @param[in] assoc   association
 * @param[in] events  event mask to wake up on
 */
static inline void
swen_l3_event_set_mask(swen_l3_assoc_t *assoc, uint8_t events)
{
	event_set_mask(&assoc->event, events);
}

/** Clear event mask
 *
 * @param[in] assoc   association
 * @param[in] events  events to be cleared
 */
static inline void
swen_l3_event_clear_mask(swen_l3_assoc_t *assoc, uint8_t events)
{
	event_clear_mask(&assoc->event, events);
}

/** Get association from event
 *
 * @param[in] ev   event
 * @return association
 */
static inline swen_l3_assoc_t *swen_l3_event_get_assoc(event_t *ev)
{
	return container_of(ev, swen_l3_assoc_t, event);
}
#endif
#endif
