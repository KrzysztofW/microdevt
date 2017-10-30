#ifndef _TIMER_H_
#define _TIMER_H_

#include <timer.h>
#include "sys/list.h"

typedef enum status {
	TIMER_STOPPED,
	TIMER_SCHEDULED,
} status_t;

struct timer {
	struct list_head list;
	void (*cb)(void *);
	void *arg;
	unsigned int remaining_loops;
	status_t status;
} __attribute__((__packed__));
typedef struct timer tim_t;

int timer_subsystem_init(void);
void timer_add(tim_t *timer, unsigned long expiry_us, void (*cb)(void *),
	       void *arg);
void timer_del(tim_t *timer);
void timer_reschedule(tim_t *timer, unsigned long expiry_us);
int timer_is_pending(tim_t *timer);
void timer_checks(void);
void timer_process(void);

#endif
