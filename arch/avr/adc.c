#include <avr/io.h>
#include "adc.h"

#if 0
void init_adc(void)
{
	/* Select Vref=AVcc */
	ADMUX |= (1<<REFS0);

	/* set prescaller to 128 and enable ADC */
	/* ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN); */

	/* set prescaller to 64 and enable ADC */
	ADCSRA |= (1<<ADPS1)|(1<<ADPS0);

	/* set PORTC (analog) for input */
	DDRC &= 0xFB; // pin 2 b1111 1011
	DDRC &= 0xFD; // pin 1 b1111 1101
	DDRC &= 0xFE; // pin 0 b1111 1110
	PORTC = 0xFF; // pullup resistors
}
#endif
uint16_t analog_read(uint8_t adc_channel)
{
	/* enable ADC */
	ADCSRA |= (1<<ADEN);

	/* select ADC channel with safety mask */
	ADMUX = (ADMUX & 0xF0) | (adc_channel & 0x0F);

	/* single conversion mode */
	ADCSRA |= (1<<ADSC);

	/* wait until ADC conversion is complete */
	while (ADCSRA & (1<<ADSC));

	/* shutdown the ADC */
	ADCSRA &= ~(1<<ADEN);
	return ADC;
}
