#include <sys/ring.h>
#include <interrupts.h>
#include "scheduler.h"

#define MAX_TASKS 16
#define HI_WATER_MARK 14

/* must a power of 2 */
#define RING_SIZE (MAX_TASKS * sizeof(task_t))

static ring_t *ring;
static ring_t *ring_irq;


void schedule_task(void (*cb)(void *arg), void *arg)
{
	task_t task = {
		.cb = cb,
		.arg = arg,
	};
	ring_t *r = IRQ_CHECK() ? ring_irq : ring;

	ring_add(r, &task, sizeof(task_t));
}

void bh(void)
{
	task_t task;
	buf_t buf = BUF_INIT(&task, sizeof(task_t));
	int irq_rlen = ring_len(ring_irq);
	if (irq_rlen < sizeof(task_t))
		irq_enable();
	else {
		if (irq_rlen / sizeof(task_t) >= HI_WATER_MARK)
			irq_disable();
		__ring_get_buf(ring_irq, &buf);
		task.cb(task.arg);
	}

	if (ring_len(ring) >= sizeof(task_t)) {
		__ring_get_buf(ring, &buf);
		task.cb(task.arg);
	}
}

void scheduler_init(void)
{
	ring_irq = ring_create(RING_SIZE);
	ring = ring_create(RING_SIZE);
}

#ifdef TEST
void scheduler_shutdown(void)
{
	ring_free(ring);
	ring_free(ring_irq);
}
#endif
