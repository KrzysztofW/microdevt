#include <stdlib.h>
#include <avr/interrupt.h>
#include "log.h"
#include "common.h"
#include "timer.h"
#include "avr-utils.h"

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

#if CONFIG_TIMER_RESOLUTION_US > 1000U
#define TIMER_DEVIDER 1000
#else
#define TIMER_DEVIDER 1000000
#endif

#define TIM_COUNTER16							\
	(65535 - (CONFIG_TIMER_RESOLUTION_US*(CONFIG_AVR_F_CPU/8))/TIMER_DEVIDER)
#define TIM_COUNTER8							\
	(255 - (CONFIG_TIMER_RESOLUTION_US*(CONFIG_AVR_F_CPU/64))/TIMER_DEVIDER)

/* 8-bit timer */
#if 0
ISR(TIMER0_OVF_vect)
{
	TCNT0 = TIM_COUNTER;
	timer_process();
}
#endif

/* 16-bit timer */
#ifdef ATTINY85
ISR(TIMER0_OVF_vect)
{
	TCNT0 = (uint16_t)TIM_COUNTER8;
	timer_process();
}
#else
ISR(TIMER1_OVF_vect)
{
	TCNT1H = (TIM_COUNTER16 >> 8) & 0xFF;
	TCNT1L = TIM_COUNTER16 & 0xFF;
	timer_process();
}
#endif
void __timer_subsystem_init(void)
{
#ifdef ATTINY85
	/* 8-bit timer */
	if (TIM_COUNTER8 < 0) {
		DEBUG_LOG("timer resolution too high\n");
		__abort();
	}
#if 1
	TCNT0 = TIM_COUNTER8;
	TCCR0A = 0;
	TCCR0B = (1 << CS00) | (1 << CS01); /* Timer mode with 64 prescler */
	TIMSK |= 1 << TOIE0; /* Enable timer0 overflow interrupt */
#else
	TCNT1 = TIM_COUNTER8;
	/* Timer mode with 64 prescler */
	TCCR1 = (1 << CS10) | (1 << CS11) | (1 << CS12);
	/* Enable timer0 overflow interrupt */
	TIMSK |= 1 << TOIE1;
#endif
#else
	/* 16-bit timer */
#if TIM_COUNTER16 < 0
#error "timer resolution too high"
#endif
	if (TIM_COUNTER16 < 0) {
		DEBUG_LOG("timer resolution too high\n");
		__abort();
	}

	TCNT1H = TIM_COUNTER16 >> 8;
	TCNT1L = TIM_COUNTER16 & 0xFF;

	TCCR1A = 0;
	TCCR1B = 1 << CS11;  /* Timer mode with 8 prescler */
	TIMSK1 = 1 << TOIE1; /* Enable timer0 overflow interrupt */
#endif
}
