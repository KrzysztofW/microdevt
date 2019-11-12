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

#include "event.h"
#include "pkt-mempool.h"

static LIST_HEAD(retry_list);

static void event_cb(void *arg)
{
	event_t *ev = arg;

	while (ev->available && (ev->available & ev->wanted)) {
		uint8_t events;

		assert(ev->available <= (EV_READ|EV_WRITE|EV_ERROR|EV_HUNGUP));
		assert(ev->wanted <= (EV_READ|EV_WRITE|EV_ERROR|EV_HUNGUP));

		if (list_empty(ev->rx_queue))
			ev->available &= ~EV_READ;
		if (ev->available & (EV_HUNGUP|EV_ERROR))
			ev->available &= ~EV_WRITE;
		else if (pkt_pool_get_nb_free() == 0) {
			ev->available &= ~EV_WRITE;
			if ((ev->wanted & EV_WRITE) && list_empty(&ev->list))
				list_add_tail(&ev->list, &retry_list);
		} else
			ev->available |= EV_WRITE;
		events = ev->available & ev->wanted;
		if (events)
			ev->cb(ev, events);
	}
}

void event_schedule_event(event_t *ev, uint8_t events)
{
	assert(events);
	if (ev->cb == NULL)
		return;

	ev->available |= events;
	if (events & (EV_ERROR | EV_HUNGUP)) {
		/* EV_WRITE will be removed in event_cb() */
		if (!list_empty(&ev->list))
			__list_del_entry(&ev->list);
	}

	if (ev->wanted & events)
		schedule_task(event_cb, ev);
}

void event_unregister(event_t *ev)
{
	ev->available = ev->wanted = 0;

	if (!list_empty(&ev->list))
		__list_del_entry(&ev->list);
}

void event_resume_write_events(void)
{
	while (pkt_pool_get_nb_free() && !list_empty(&retry_list)) {
		event_t *ev = list_first_entry(&retry_list, event_t, list);

		__list_del_entry(&ev->list);
		if (ev->wanted & EV_WRITE) {
			ev->available |= EV_WRITE;
			event_cb(ev);
		}
	}
}
