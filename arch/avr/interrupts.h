#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_
#include <avr/interrupt.h>

#define irq_disable cli
#define irq_enable sei

#define disable_spi_irq()
#define enable_spi_irq()

#define IRQ_CHECK() (!!(SREG & 0x80))

#endif
