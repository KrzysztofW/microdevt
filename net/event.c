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
		if (pkt_pool_get_nb_free() == 0) {
			ev->available &= ~EV_WRITE;
			if (ev->wanted & EV_WRITE)
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
	if (events == 0 || ev->cb == NULL)
		return;

	ev->available |= events;
	if (events & (EV_ERROR | EV_HUNGUP)) {
		ev->available &= ~EV_WRITE;
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
		event_schedule_event(ev, ev->available | EV_WRITE);
	}
}
