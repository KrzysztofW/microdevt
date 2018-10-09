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
#define CONFIG_SCHEDULER_TASK_WATER_MARK 8
#endif
#define SCHEDULER_TASK_WATER_MARK				\
	(CONFIG_SCHEDULER_TASK_WATER_MARK * sizeof(task_t))

#define RING_SIZE (CONFIG_SCHEDULER_MAX_TASKS * sizeof(task_t))

static ring_t *ring;
static ring_t *ring_irq;

#ifdef CONFIG_POWER_MANAGEMENT
static uint8_t idle;
#endif

void schedule_task(void (*cb)(void *arg), void *arg)
{
	task_t task = {
		.cb = cb,
		.arg = arg,
	};
	ring_t *r = IRQ_CHECK() ? ring : ring_irq;

	while (ring_add(r, &task, sizeof(task_t)) < 0) {
		if (r == ring_irq) {
			DEBUG_LOG("cannot schedule: lower TASK_WATER_MARK "
				  "option\n");
			__abort();
		}
		scheduler_run_tasks();
	}
}

static void __scheduler_run_tasks(ring_t *r)
{
	task_t task;
	buf_t buf = BUF_INIT(&task, sizeof(task_t));

	assert(ring_len(r) >= sizeof(task_t));
	__ring_get_buf(r, &buf);
	task.cb(task.arg);
#ifdef CONFIG_POWER_MANAGEMENT
	idle = 0;
#endif
}

void scheduler_run_tasks(void)
{
	int irq_rlen = ring_len(ring_irq);

#ifdef CONFIG_POWER_MANAGEMENT
	idle = 1;
#endif
	if (irq_rlen) {
		if (irq_rlen >= SCHEDULER_TASK_WATER_MARK)
			irq_disable();
		__scheduler_run_tasks(ring_irq);
	} else
		irq_enable();

	if (ring_len(ring))
		__scheduler_run_tasks(ring);

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
