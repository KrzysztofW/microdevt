#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <avr/eeprom.h>
#include "interrupts.h"

static inline void eeprom_update(void *dst, const void *src, uint8_t size)
{
	irq_disable();
	eeprom_update_block(src, dst, size);
	irq_enable();
}

static inline void eeprom_load(void *dst, const void *src, uint8_t size)
{
	eeprom_read_block(dst, src, size);
}

#endif
