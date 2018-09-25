#include "event.h"
#include "pkt-mempool.h"

static LIST_HEAD(retry_list);

static void event_cb(void *arg)
{
	event_t *ev = arg;

	while (ev->available && (ev->available & ev->wanted)) {
		uint8_t events;

		if (list_empty(ev->rx_queue))
			ev->available &= ~EV_READ;
		if (pkt_pool_get_nb_free() == 0) {
			ev->available &= ~EV_WRITE;
			if (ev->wanted & EV_WRITE)
				list_add_tail(&ev->list, &retry_list);
		}
		events = ev->available & ev->wanted;
		if (events)
			ev->cb(ev, events);
	}
}

static void __event_call(event_t *ev, uint8_t events, uint8_t schedule)
{
	if (events == 0 || ev->cb == NULL)
		return;

	ev->available |= events;
	if (ev->wanted & events) {
		if (schedule)
			schedule_task(event_cb, ev);
		else
			ev->cb(ev, ev->available);
	}
}

void event_schedule_event(event_t *ev, uint8_t events)
{
	__event_call(ev, events, 1);
}

void event_call(event_t *ev, uint8_t events)
{
	__event_call(ev, events, 0);
}

void event_unregister(event_t *ev)
{
	ev->available = ev->wanted = 0;

	if (!list_empty(&ev->list))
		__list_del_entry(&ev->list);
}

void event_schedule_event_error(event_t *ev)
{
	uint8_t events = (ev->available | EV_ERROR) & ~EV_WRITE;

	if (!list_empty(&ev->list))
		__list_del_entry(&ev->list);
	event_schedule_event(ev, events);
}

void event_resume_write_events(void)
{
	while (pkt_pool_get_nb_free() && !list_empty(&retry_list)) {
		event_t *ev = list_first_entry(&retry_list, event_t, list);

		__list_del_entry(&ev->list);
		__event_call(ev, ev->available | EV_WRITE, 1);
	}
}
