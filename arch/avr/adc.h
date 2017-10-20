#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>

void init_adc(void);
uint16_t analog_read(uint8_t adc_channel);

#endif
