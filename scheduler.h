#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

typedef struct __attribute__((__packed__)) task {
	void (*cb)(void *arg);
	void *arg;
} task_t;

int schedule_task(const task_t *task);
int scheduler_init(void);
void scheduler_shutdown(void);

/* run bottom halves */
void bh(void);

#endif
