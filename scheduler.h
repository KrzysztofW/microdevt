#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#ifdef DEBUG
void __schedule_task(void (*cb)(void *arg), void *arg,
		     const char *func, int line);
#define schedule_task(cb, arg) __schedule_task(cb, arg, __func__, __LINE__)
#else
void schedule_task(void (*cb)(void *arg), void *arg);
#endif

/* run bottom halves */
void scheduler_run_tasks(void);

#endif
