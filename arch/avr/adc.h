#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>

#define ADC_EXTERNAL_AVCC 0
#define ADC_INTERNAL_AVCC 1

/* Note that changing reference voltage may lead to incorrect
 * results. The user must discard first results after AREF change
 * as specified in chapter 24.6 p243 of Atmega328p datasheet.
 */
#define ADC_5V_REF_VOLTAGE 5000U
#define ADC_1_1V_REF_VOLTAGE 1100U

#define ADC_SET_PRESCALER_64() ADCSRA = (1 << ADPS1) | (1 << ADPS0)
#define ADC_SET_PRESCALER_128()					\
	ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))

#define adc_init_internal_64prescaler() do {		\
		ADMUX = (1 << REFS0) | (1 << REFS1);	\
		ADC_SET_PRESCALER_64();			\
	} while (0)

#define adc_init_internal_128prescaler() do {		\
		ADMUX = (1 << REFS0) | (1 << REFS1);	\
		ADC_SET_PRESCALER_128();		\
	} while (0)

#define adc_init_external_64prescaler() do {			\
		ADMUX = 0;					\
		ADC_SET_PRESCALER_64();				\
	} while (0)

#define adc_init_external_128prescaler() do {			\
		ADMUX = 0;					\
		ADC_SET_PRESCALER_128();			\
	} while (0)

#define adc_init_avcc_64prescaler() do {		\
		ADMUX = 1 << REFS0;			\
		ADC_SET_PRESCALER_64();			\
	} while (0)

#define adc_init_avcc_128prescaler() do {		\
		ADMUX = 1 << REFS0;			\
		ADC_SET_PRESCALER_128();		\
	} while (0)

#define adc_enable() ADCSRA |= 1 << ADEN

static inline uint16_t adc_read(uint8_t channel)
{
	adc_enable();

	/* select ADC channel */
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

	/* single conversion mode */
	ADCSRA |= 1 << ADSC;

	/* wait until ADC conversion is complete */
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

#define adc_shutdown() ADCSRA &= ~(1 << ADEN)

static inline uint16_t adc_to_millivolt(uint16_t ref_voltage, uint32_t val)
{
	return (val * ref_voltage) / 1023;
}

static inline uint16_t adc_read_mv(uint16_t ref_voltage, uint8_t channel)
{
	return adc_to_millivolt(ref_voltage, adc_read(channel));
}

#endif
