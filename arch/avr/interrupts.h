#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_
#include <avr/interrupt.h>

#define irq_disable cli
#define irq_enable sei

#define irq_save(flags) do {			\
		flags = SREG;			\
		irq_disable();			\
	} while (0)

#define irq_restore(flags) do {			\
		SREG = flags;			\
	} while (0)

#define IRQ_CHECK() (!!(SREG & 0x80))

#endif
