#include <sys/ring.h>
#include "scheduler.h"

#define MAX_TASKS 32
/* will be rounded to a power of 2 */
#define RING_SIZE (MAX_TASKS * sizeof(task_t))

static ring_t *ring;

int schedule_task(const task_t *task)
{
	return ring_add(ring, task, sizeof(task_t));
}

void bh(void)
{
	task_t task;
	buf_t buf;

	if (ring_len(ring) < sizeof(task_t))
		return;

	buf = BUF_INIT(&task, sizeof(task_t));
	__ring_get_buf(ring, &buf);
	task.cb(task.arg);
}

int scheduler_init(void)
{
	int size = roundup_pwr2(RING_SIZE);

#ifndef CONFIG_RING_STATIC_ALLOC
	if ((ring = ring_create(size)) == NULL) {
		DEBUG_LOG("%s: cannot create BH ring\n", __func__);
		return -1;
	}
#else
	ring = ring_create(size);
#endif
	return 0;
}

#if 0
void scheduler_shutdown(void)
{
	ring_free(ring);
}
#endif
