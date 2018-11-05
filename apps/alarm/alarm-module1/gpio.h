#ifndef _GPIO_H_
#define _GPIO_H_
#include <adc.h>

/* output pins */
#define LED PD7
#define LED_PORT PORTD
#define LED_DDR  DDRD

#define FAN PD3
#define FAN_PORT PORTD
#define FAN_DDR  DDRD

#define SIREN PB1
#define SIREN_PORT PORTB
#define SIREN_DDR  DDRB

/* input pins */
#define PIR PD2
#define PIR_PORT PORTD
#define PIR_IN_PORT PIND
#define PIR_DDR  DDRD

#define PWR_SENSOR PB0
#define PWR_SENSOR_PORT PORTB
#define PWR_SENSOR_IN_PORT PINB
#define PWR_SENSOR_DDR  DDRB

static inline void gpio_init(void)
{
	/* set output pins */
	LED_DDR |= 1 << LED;
	SIREN_DDR |= 1 << SIREN;

	/* set input pins */
	PWR_SENSOR_DDR &= ~(1 << PWR_SENSOR);
	PIR_DDR &= ~(1 << PIR);

	/* enable pull-up resistors on unconnected pins */
	PORTB = 0xFF & ~((1 << SIREN) | (1 << PWR_SENSOR));
	PORTD = 0xFF & ~((1 << LED) | (1 << FAN) | (1 << PIR));

	/* PORTC used RF sender (PC2) & receiver (PC0) */
	/* analog: PC1 - HIH, PC3 - tmp36gz */
	PORTC = 0xFF & ~((1 << PC0) | (1 << PC2) | (1 << PC1) | (1 << PC3));

	/* analog pins */
	DDRC = 0xFF & ~((1 << PC0) | (1 << PC1) | (1 << PC3));

#ifndef CONFIG_AVR_SIMU
	/* PCINT18 enabled (PIR) */
	PCMSK2 = 1 << PCINT18;

	/* PCINT0 enabled (PWR_SENSOR) */
	PCMSK0 = 1 << PCINT0;

	PCICR = (1 << PCIE2) | (1 << PCIE0);
#endif
}

/* LED */
static inline void gpio_led_on(void)
{
	LED_PORT |= 1 << LED;
}

static inline void gpio_led_off(void)
{
	LED_PORT &= ~(1 << LED);
}
static inline void gpio_led_toggle(void)
{
	LED_PORT ^= 1 << LED;
}

/* FAN */
static inline void gpio_fan_on(void)
{
	FAN_PORT |= 1 << FAN;
}

static inline void gpio_fan_off(void)
{
	FAN_PORT &= ~(1 << FAN);
}

static inline uint8_t gpio_is_fan_on(void)
{
	return !!(FAN_PORT & (1 << FAN));
}

/* SIREN */
static inline void gpio_siren_on(void)
{
	SIREN_PORT |= 1 << SIREN;
}

static inline void gpio_siren_off(void)
{
	SIREN_PORT &= ~(1 << SIREN);
}

static inline uint8_t gpio_is_siren_on(void)
{
	return !!(SIREN_PORT & (1 << SIREN));
}

/* PWR sensor */
static inline uint8_t gpio_is_main_pwr_on(void)
{
	/*
	 * The pin value must be inverted as the used NPN transistor
	 * gives 1 => power disconnected, 0 => power connected
	 */
	return !(PWR_SENSOR_IN_PORT & (1 << PWR_SENSOR));
}

/* PIR sensor */
static inline uint8_t gpio_is_pir_on(void)
{
	return !!(PIR_IN_PORT & (1 << PIR));
}

#endif
