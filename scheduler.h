#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

typedef struct __attribute__((__packed__)) task {
	void (*cb)(void *arg);
	void *arg;
} task_t;

void schedule_task(void (*cb)(void *arg), void *arg);
void scheduler_init(void);
void scheduler_shutdown(void);

/* run bottom halves */
void scheduler_run_tasks(void);

#endif
