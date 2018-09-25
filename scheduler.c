#include <sys/ring.h>
#include <interrupts.h>
#include <power-management.h>
#include "scheduler.h"

typedef struct __attribute__((__packed__)) task {
	void (*cb)(void *arg);
	void *arg;
} task_t;

#ifndef CONFIG_SCHEDULER_MAX_TASKS
#define CONFIG_SCHEDULER_MAX_TASKS 16
#endif
#ifndef CONFIG_SCHEDULER_TASK_WATER_MARK
#define CONFIG_SCHEDULER_TASK_WATER_MARK 14
#endif

#define RING_SIZE (CONFIG_SCHEDULER_MAX_TASKS * sizeof(task_t))

static ring_t *ring;
static ring_t *ring_irq;

void schedule_task(void (*cb)(void *arg), void *arg)
{
	task_t task = {
		.cb = cb,
		.arg = arg,
	};
	ring_t *r = IRQ_CHECK() ? ring_irq : ring;

	if (ring_add(r, &task, sizeof(task_t)) < 0)
		__abort();
}

void scheduler_run_tasks(void)
{
	task_t task;
	buf_t buf = BUF_INIT(&task, sizeof(task_t));
	int irq_rlen = ring_len(ring_irq);
#ifdef CONFIG_POWER_MANAGEMENT
	uint8_t idle = 1;
#endif
	if (irq_rlen < sizeof(task_t))
		irq_enable();
	else {
		if (irq_rlen / sizeof(task_t) >= CONFIG_SCHEDULER_TASK_WATER_MARK)
			irq_disable();
		__ring_get_buf(ring_irq, &buf);
		task.cb(task.arg);
#ifdef CONFIG_POWER_MANAGEMENT
		idle = 0;
#endif
	}

	if (ring_len(ring) >= sizeof(task_t)) {
		__ring_get_buf(ring, &buf);
		task.cb(task.arg);
#ifdef CONFIG_POWER_MANAGEMENT
		idle = 0;
#endif
	}
#ifdef CONFIG_POWER_MANAGEMENT
	if (idle) {
		power_management_set_mode(PWR_MGR_SLEEP_MODE_IDLE);
		power_management_sleep();
	}
#endif
}

void scheduler_init(void)
{
	int size = roundup_pwr2(RING_SIZE);

	ring_irq = ring_create(size);
	ring = ring_create(size);
}

#ifdef TEST
void scheduler_shutdown(void)
{
	ring_free(ring);
	ring_free(ring_irq);
}
#endif
