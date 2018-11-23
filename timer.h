#ifndef _TIMER_H_
#define _TIMER_H_

#include <timer.h>
#include <stdint.h>
#include "sys/list.h"

struct timer {
	struct list_head list;
	void (*cb)(void *);
	void *arg;
	uint32_t ticks;
} __attribute__((__packed__));
typedef struct timer tim_t;

#define TIMER_INIT(__tim) { .list = LIST_HEAD_INIT((__tim).list) }
void timer_subsystem_init(void);
void timer_subsystem_shutdown(void);
void timer_init(tim_t *timer);
void timer_add(tim_t *timer, uint32_t expiry, void (*cb)(void *), void *arg);

void timer_del(tim_t *timer);
void timer_reschedule(tim_t *timer, uint32_t expiry);
void timer_process(void);
static inline uint8_t timer_is_pending(tim_t *timer)
{
	return !list_empty(&timer->list);
}

void timer_checks(void);
void timer_dump(void);
#endif
