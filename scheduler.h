#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

void schedule_task(void (*cb)(void *arg), void *arg);
void scheduler_init(void);
void scheduler_shutdown(void);

/* run bottom halves */
uint8_t scheduler_run_tasks(void);

#endif
