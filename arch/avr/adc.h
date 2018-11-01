#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>

static inline void analog_init(void)
{
	/* AVCC reference voltage */
	ADMUX = 1 << REFS0;

	/* set prescaller to 128 and enable ADC */
	/* ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN); */

	/* set prescaller to 64 and enable ADC */
	ADCSRA = (1 << ADPS1) | (1 << ADPS0);
}

static inline uint16_t analog_read(uint8_t adc_channel)
{
	/* enable ADC */
	ADCSRA |= (1<<ADEN);

	/* select ADC channel with safety mask */
	ADMUX = (ADMUX & 0xF0) | (adc_channel & 0x0F);

	/* single conversion mode */
	ADCSRA |= 1 << ADSC;

	/* wait until ADC conversion is complete */
	while (ADCSRA & (1 << ADSC));

	/* shutdown the ADC */
	ADCSRA &= ~(1 << ADEN);

	return ADC;
}

static inline uint16_t analog_to_millivolt(uint16_t val)
{
	return (val * 1000UL * 5UL) / 1023;
}

#endif
