#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "timer.h"

uint8_t irq_lock;

static inline void process_timers(int signo)
{
	(void)signo;
	if (!irq_lock)
		timer_process();
}

void __timer_subsystem_init(void)
{
	struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &process_timers;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		fprintf(stderr, "\ncan't initialize timers (%m)\n");
		abort();
	}

	/* configure the timer to first expire after 3 seconds */
	timer.it_value.tv_sec = 3;
	timer.it_value.tv_usec = 0;

	if (CONFIG_TIMER_RESOLUTION_US > 1000000) {
		timer.it_interval.tv_sec = CONFIG_TIMER_RESOLUTION_US / 1000000;
		timer.it_interval.tv_usec = CONFIG_TIMER_RESOLUTION_US % 1000000;
	} else {
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = CONFIG_TIMER_RESOLUTION_US;
	}

	if (setitimer(ITIMER_REAL, &timer, NULL) < 0)
		fprintf(stderr, "\n can'\t set itimer (%m)\n");
}

void __timer_subsystem_shutdown(void)
{
	struct itimerval timer;

	memset(&timer, 0, sizeof(timer));
	if (setitimer(ITIMER_REAL, &timer, NULL) < 0)
		fprintf(stderr, "\n can'\t disable itimer (%m)\n");
}
