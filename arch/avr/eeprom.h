#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <string.h>
#include <avr/eeprom.h>
#include <log.h>
#include <common.h>
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

static inline int8_t
eeprom_update_and_check(void *dst, const void *src, uint8_t size)
{
	uint8_t data[size];
	int8_t ret;

	eeprom_update(dst, src, size);
	eeprom_load(data, dst, size);
	ret = (memcmp(data, src, size) == 0);
	assert(ret);
	return ret;
}
#endif
