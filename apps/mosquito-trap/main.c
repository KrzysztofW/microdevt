/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 3, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#include <log.h>
#include <_stdio.h>
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/utils.h>

static tim_t temp_timer;
static tim_t led_timer;

static void pwm(void)
{
	//DDRD |= (1 << DDD3);
	DDRD |= (1 << PD3);
	// PD6 is now an output

	OCR2B = 128;
	//OCR0A = 64;
	// set PWM for 50% duty cycle

	TCCR0A |= (1 << COM0A1);
	// set none-inverting mode

	TCCR2A |= (1 << WGM01) | (1 << WGM00);
	// set fast PWM Mode

	TCCR2B |= (1 << CS01);
	// set prescaler to 8 and start PWM
}

static void temp_task(void *arg)
{
	uint8_t i;
	uint16_t temperature = 0;

	timer_reschedule(&temp_timer, 500000);
	for (i = 0; i < 10; i++)
		temperature += adc_read(0);
	temperature /= 10;

	if (temperature < 36)
		PORTD |= (1 << PD1);
	else if (temperature > 36.6)
		PORTD &= ~(1 << PD1);
}

static void temp_timer_cb(void *arg)
{
	schedule_task(&temp_task, NULL);
}

static void fan_task(void *arg)
{
	uint16_t potentiometer;

	potentiometer = adc_read(1);
	OCR0A = map(potentiometer, 0, 1023, 0, 255);
	schedule_task(&fan_task, NULL);
}

static void led_timer_cb(void *arg)
{
	PORTD ^= 1 << PD0;
	timer_reschedule(&led_timer, 1000000);
}

int main(void)
{
	init_stream0(&stdout, &stdin, 0);
	DEBUG_LOG("KW mosquito trap v0.1\n");

	ADC_SET_PRESCALER_64();
	ADC_SET_PRESCALER_64();
	timer_subsystem_init();

	DDRD = (1 << PD0); /* LED PIN */
	timer_add(&temp_timer, 0, temp_timer_cb, NULL);
	timer_add(&led_timer, 0, led_timer_cb, NULL);

	/* PC0 - RFRX input, PC1 - charger output, PC2 - PIR input */
	//DDRC = 1;
	//DDRC &= ~(1 << PC0);
	/* enable pull-up resistors */
	//PORTC = 1;
	/* configure the siren pin as output */
	//DDRB = 0x07;

	watchdog_enable(WATCHDOG_TIMEOUT_8S);
	pwm();
	sei();
	schedule_task(&fan_task, NULL);

	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
	}
	return 0;
}
