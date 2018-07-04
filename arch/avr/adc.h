#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>

#define INIT_ADC_DECL(SUFFIX, DDR, PORT)				\
	void init_adc_##SUFFIX(void)					\
	{								\
		ADMUX |= (1<<REFS0);					\
		/* set prescaller to 128 and enable ADC */		\
		/* ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN); */ \
		/* set prescaller to 64 and enable ADC */		\
		ADCSRA |= (1<<ADPS1)|(1<<ADPS0);			\
		DDR &= 0xFF;						\
		/* pullup resistors */					\
		PORT = 0xFF;						\
	}

uint16_t analog_read(uint8_t adc_channel);

#endif
