#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "timer.h"

#define TIM_COUNTER 1

static inline void process_timers(int signo)
{
	(void)signo;
	timer_process();
	__timer_subsystem_init();
}

void __timer_subsystem_init(void)
{
	if (signal(SIGALRM, process_timers) == SIG_ERR) {
		fprintf(stderr, "\ncan't catch SIGALRM\n");
		return;
	}

	alarm(1);
}
