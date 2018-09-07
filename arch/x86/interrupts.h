#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

extern uint8_t irq_lock;

#define irq_disable() irq_lock = 1
#define irq_enable() irq_lock = 0

#define irq_save(flags) do {			\
		flags = irq_lock;		\
		irq_disable();			\
	} while (0)

#define irq_restore(flags) do {			\
		irq_lock = flags;		\
	} while (0)

#define IRQ_CHECK() 0

#endif
