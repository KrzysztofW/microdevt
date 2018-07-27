#include <stdlib.h>
#include "timer.h"

/* 8-bit timer 300us resolution
 * 255-0.0003/(1/(16000000/64.)) = 180
 * where 255 is the 8-bit counter max value
 *       0.0003 = 300us
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       64 = prescaler value
 *
 * 16-bit timer 150us resolution
 * 65535-0.00015/(1/(16000000/8.)) = 65235
 * where 65535 is the 16-bit counter max value
 *       0.00015 = 150us
 *       1/(16000000) = 1/CONFIG_AVR_F_CPU
 *       8 = prescaler value
 *       <=> (65535 - (150*(16000000/8))/1000000)
 */
#define TIM_COUNTER (UINT16_MAX - \
		     (CONFIG_TIMER_RESOLUTION_US*(CONFIG_AVR_F_CPU/8))/1000000)

/* 8-bit timer */
#if 0
ISR(TIMER0_OVF_vect)
{
	TCNT0 = TIM_COUNTER;
	timer_process();
}
#endif

/* 16-bit timer */
ISR(TIMER1_OVF_vect)
{
	TCNT1H = (TIM_COUNTER >> 8) & 0xFF;
	TCNT1L = TIM_COUNTER & 0xFF;
	timer_process();
}

void __timer_subsystem_init(void)
{
#if 0
	/* 8-bit timer */
	TCNT0 = TIM_COUNTER;
	TCCR0A = 0;
	TCCR0B = (1 << CS00) | (1 << CS01); /* Timer mode with 64 prescler */
	TIMSK0 = 1 << TOIE0; /* Enable timer0 overflow interrupt(TOIE0) */
#endif
	/* 16-bit timer */
	TCNT1H = TIM_COUNTER >> 8;
	TCNT1L = TIM_COUNTER & 0xFF;

	TCCR1A = 0;
	TCCR1B = 1 << CS11;    /* Timer mode with 8 prescler */
	TIMSK1 = 1 << TOIE1; /* Enable timer0 overflow interrupt(TOIE1) */

	sei();
}
