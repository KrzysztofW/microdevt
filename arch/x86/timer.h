#ifndef _X86_TIMER_H_
#define _X86_TIMER_H_

#include "atomic.h"
#include "../../timer.h"

extern atomic_t timer_disabled;

static inline void disable_timer_int(void)
{
	atomic_store(&timer_disabled, 1);
}

static inline void enable_timer_int(void)
{
	atomic_store(&timer_disabled, 0);
}

void __timer_subsystem_init(void);
void __timer_subsystem_shutdown(void);
#endif
