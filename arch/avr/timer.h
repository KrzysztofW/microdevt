#ifndef _AVR_TIMER_H_
#define _AVR_TIMER_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../timer.h"

static inline void disable_timer_int(void)
{
	TCCR1B = 0;
}

static inline void enable_timer_int(void)
{
	TCCR1B = 1 << CS11;
}

void __timer_subsystem_init(void);

#endif
