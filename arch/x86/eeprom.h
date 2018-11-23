#ifndef _EEPROM_H_
#define _EEPROM_H_

static inline void eeprom_update(void *dst, const void *src, uint8_t size)
{
	memcpy(dst, src, size);
}

static inline void eeprom_load(void *dst, const void *src, uint8_t size)
{
	eeprom_update(dst, src, size);
}

static inline int8_t
eeprom_update_and_check(void *dst, const void *src, uint8_t size)
{
	eeprom_update(dst, src, size);
	return 0;
}
#endif
