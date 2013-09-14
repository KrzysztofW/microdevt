#include <avr/io.h>
#include <stdio.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define DEBUG
#ifdef DEBUG
#include "usart0.h"
#define SERIAL_SPEED 38400
#define SYSTEM_CLOCK F_CPU
#endif
#include "sound.h"
#include "utils.h"

enum state_type {
	IDLE,
	SET_PASS,
	GETOUT,
	GETIN,
	WATCH,
	ALARM,
};

int pass[6] = { 0, 7, 0, 5};
#define PASS_LENGTH 4
#define TIMEOUT 20

enum state_type state;

#ifdef DEBUG
static int my_putchar(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r') {
		usart0_put('\r');
		usart0_put('\n');
	}
	usart0_put(c);
	return 0;
}

static int my_getchar(FILE * stream)
{
	(void)stream;
	return  usart0_get();
}

/*
 * Define the input and output streams.
 * The stream implemenation uses pointers to functions.
 */
static FILE my_stream =
	FDEV_SETUP_STREAM (my_putchar, my_getchar, _FDEV_SETUP_RW);
#endif


void initADC()
{
	/* Select Vref=AVcc */
	ADMUX |= (1<<REFS0);

	/* set prescaller to 128 and enable ADC */
	/* ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN); */

	/* set prescaller to 64 and enable ADC */
	ADCSRA |= (1<<ADPS1)|(1<<ADPS0);
}

uint16_t analogRead(uint8_t ADCchannel)
{
	/* enable ADC */
	ADCSRA |= (1<<ADEN);

	/* select ADC channel with safety mask */
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);

	/* single conversion mode */
	ADCSRA |= (1<<ADSC);

	// wait until ADC conversion is complete
	while (ADCSRA & (1<<ADSC));

	/* shutdown the ADC */
	ADCSRA &= ~(1<<ADEN);
	return ADC;
}

void print_buff(int *buffer)
{
	int i;
	for (i = 0; i < PASS_LENGTH; i++) {
		printf("%d", buffer[i]);
	}
	printf("\n");
}

int from_keycode(int keycode)
{
	int key;
	/*
	 * 972  872  795  728
	 * 649  608  574  543
	 * 408  391  384  368
	 * 358  345  335  327
	 */

	/* first line */
	if (keycode > 952 && keycode < 990)
		key = 1;
	else if (keycode > 852 && keycode < 892)
		key = 2;
	else if (keycode > 772 && keycode < 812)
		key = 3;
	else if (keycode > 708 && keycode < 748)
		key = 0xA;
	/* second line */
	else if (keycode > 629 && keycode < 669)
		key = 4;
	else if (keycode > 588 && keycode < 628)
		key = 5;
	else if (keycode > 554 && keycode < 594)
		key = 6;
	else if (keycode > 523 && keycode < 563)
		key = 0xB;
	/* third line */
	else if (keycode > 400 && keycode < 425)
		key = 7;
	else if (keycode > 387 && keycode < 400)
		key = 8;
	else if (keycode > 376 && keycode < 387)
		key = 9;
	else if (keycode > 364 && keycode < 376)
		key = 0xC;
	/* forth line */
	else if (keycode > 350 && keycode < 364)
		key = 0xE;
	else if (keycode > 340 && keycode < 350)
		key = 0;
	else if (keycode > 330 && keycode < 340)
		key = 0xF;
	else if (keycode > 320 && keycode < 330)
		key = 0xD;
	else
		key = 999;

	return key;
}

void reset_buffer(int *buffer)
{
	int i;

	for (i = 0; i < PASS_LENGTH; i++) {
		buffer[i] = 0;
	}
}
int check(int *buffer)
{
	int i;

	for (i = 0; i < PASS_LENGTH; i++) {
		if (pass[i] != buffer[i])
			return 1;
	}

	return 0;
}

void loop()
{
	int touch_count = 0;
	int buffer[PASS_LENGTH] = {0};
	int timeout = 0;

	while (1) {
		int light_tim;
		int move_sensor1 = 0;
		int move_sensor2 = 0;
		int key, keycode;

		keycode = analogRead(0);
		key = from_keycode(keycode);

		if (key != 999) { /* key pressed */
			printf("%d %d\n", key, keycode);

			if (key == 0xA) {
				if (check(buffer) == 0) {
					reset_buffer(buffer);
					state = GETOUT;
					bip_alter();
				} else
					bip_wrong();

				touch_count = 0;
				continue;
			} else if (key == 0xD) {
				if (check(buffer) == 0) {
					reset_buffer(buffer);
					state = IDLE;
					bip_alter();
				} else
					bip_wrong();
				touch_count = 0;
				continue;
			} else
				bip();


			buffer[touch_count++] = key;
			if (touch_count >= PASS_LENGTH) {
				touch_count = 0;
			}
			printf("key=%d\n", key);
			//printf("buffer:");
			//print_buff(buffer);
			//printf("pass:");
			//print_buff(pass);
		}

		switch (state) {
		case IDLE:
			printf("IDLE\n");
			light_tim = 500;
			PORTB &= ~1; /* siren off */
			timeout = 0;
			break;
		case GETOUT:
			printf("GETOUT\n");
			play_getout();
			if (timeout > TIMEOUT) {
				timeout = 0;
				state = WATCH;
			}
			timeout++;
			break;
		case WATCH:
			PORTB &= ~1; /* siren off */
			printf("WATCH\n");
			move_sensor1 = analogRead(1);
			move_sensor2 = analogRead(2);

#ifdef DEBUG
			printf("1: %d   2: %d\n", move_sensor1, move_sensor2);
#endif
			if (move_sensor1 > 100 || move_sensor2 > 100)
				state = GETIN;
			break;
		case GETIN:
			printf("GETIN\n");
			play_getin();
			if (timeout > TIMEOUT) {
				timeout = 0;
				state = ALARM;
			}
			timeout++;
			break;
		case ALARM:
			printf("ALARM\n");
			light_tim = 200;
			PORTB |= 1; /* siren on */

			PORTB |= 1<<3;
			delay_ms(light_tim);
			PORTB &= ~(1<<3);
			//delay_ms(100);
			if (timeout > TIMEOUT * 10) {
				timeout = 0;
				state = WATCH;
			}
			timeout++;

			break;
		default:
			/* nothing */
			break;
		}

		PORTB |= 1<<3;
		delay_ms(light_tim);
		PORTB &= ~(1<<3);
		delay_ms(100);
	}
}
void init_streams()
{
	// initialize the standard streams to the user defined one
	stdout = &my_stream;
	stdin = &my_stream;
	usart0_init(BAUD_RATE(SYSTEM_CLOCK, SERIAL_SPEED));
}

int main(void)
{
	/* led */
	DDRB = 1<<3; // port B3, ATtiny13a pin 2

	/* siren */
	DDRB |= 1; // port B0, ATtiny13a pin 0

	speaker_init(); // port B1 (0x2)
	start_song();

	initADC();
	/* set PORTC (analog) for input */
	DDRC &= 0xFB; // pin 2 b1111 1011
	DDRC &= 0xFD; // pin 1 b1111 1101
	DDRC &= 0xFE; // pin 0 b1111 1110
	PORTC = 0xFF; // pullup resistors

#ifdef DEBUG
	init_streams();
	printf_P(PSTR("KW alarm v0.1\n"));
#endif
	state = IDLE;
	loop();

	return 0;
}
