#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdint.h>
#include <stdlib.h>
#include <sys/list.h>
#include <log.h>
#include <common.h>
#include <scheduler.h>

typedef enum event_flags {
	EV_NONE  = 0,
	EV_READ  = 1,
	EV_WRITE = 1 << 1,
	EV_ERROR = 1 << 2,
	EV_HUNGUP = 1 << 3,
	/* EV_LAST  = 1 << 7, */
} event_flags_t;

typedef struct event {
	void (*cb)(struct event *event_data, uint8_t events);
	uint8_t wanted;
	uint8_t available;
	list_t list;
	list_t *rx_queue;
} event_t;

void event_schedule_event(event_t *ev, uint8_t events);
void event_call(event_t *ev, uint8_t events);
void event_set(event_t *ev, uint8_t events,
	       void (*ev_cb)(event_t *ev, uint8_t events));
void event_schedule_event_error(event_t *event);
void event_resume_write_events(void);

static inline void event_init(event_t *ev)
{
	ev->wanted = ev->available = 0;
	INIT_LIST_HEAD(&ev->list);
}

static inline void event_set_mask(event_t *ev, uint8_t events)
{
	ev->wanted = events | EV_ERROR | EV_HUNGUP;
	if (events && (events & ev->available))
		event_schedule_event(ev, ev->available);
}

static inline void
event_register(event_t *ev, uint8_t events, list_t *rx_queue,
	       void (*ev_cb)(event_t *ev, uint8_t events))
{
	ev->wanted = events | EV_ERROR | EV_HUNGUP;
	ev->cb = ev_cb;
	ev->rx_queue = rx_queue;
}

void event_unregister(event_t *ev);
#endif
