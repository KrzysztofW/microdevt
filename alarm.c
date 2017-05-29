#include <avr/io.h>
#include <stdio.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>

#define DEBUG
#ifdef DEBUG
#include "usart0.h"
#define SERIAL_SPEED 57600
#define SYSTEM_CLOCK F_CPU
#define TIMER_RESOLUTION_US 150UL
#endif
#include "avr_utils.h"
#include "ring.h"
#include "timer.h"
#include "commands.h"

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

typedef enum state {
	WAIT,
	STARTED,
} state_t;

#define START_FRAME = 0xAA

typedef struct pkt_header {
	unsigned char  start_frame; /* 0 */
	unsigned short from;	    /* 1 */
	unsigned char  len;	    /* 3 */
	unsigned short chksum;	    /* 4 */
	unsigned char  data[];      /* 6 */
} pkt_header_t;

#if 0
static void pkt_parse(ring_t *ring)
{
	unsigned short addr;
	int len, chksum, chksum2 = 0, i;

	if (ring_is_empty(ring))
		return;

	if (ring_len(ring) < sizeof(pkt_head)) {
		return;
	}

	if (ring->data[ring->tail] != START_FRAME) {
		ring_reset(ring); // maybe skip 1?
		return;
	}

	/* check if we have enough data */
	len = ring->data[ring->tail + 3];
	if (len + sizeof(pkt_header_t) > MAX_SIZE) {
		ring_reset(ring);
		return;
	}
	if (ring_len(ring) < len + sizeof(pkt_head_t)) {
		/* not enough data */
		return;
	}

	addr = (unsigned short)ring->data[ring->tail + 1];
	chksum = (unsigned short)ring->data[ring->tail + 4];
	for (i = 0; i < len; i++) {
		chksum2 += ring->data[ring->tail + 6 + i];
	}
	if (checksum != chksum) {
		ring_skip(ring, len);
	}
}
#endif
void init_streams()
{
	// initialize the standard streams to the user defined one
	stdout = &my_stream;
	stdin = &my_stream;
	usart0_init(BAUD_RATE(SYSTEM_CLOCK, SERIAL_SPEED));
}

ring_t ring;

#define LED PD4
void tim_cb_led(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= (1 << LED);
	timer_reschedule(timer, TIMER_RESOLUTION_US);
}

#define FRAME_DELIMITER 40
#define ANALOG_LOW_VAL 170
#define ANALOG_HI_VAL  690

static inline int decode(int bit, int count)
{
	static int started;

	if (started && count >= 1 && count <= 5) {
		if (ring_is_empty(&ring) && ring.byte.pos == 0 && bit == 0) {
			goto error;
		}
		if (ring_add_bit(&ring, bit) < 0)
			goto error;

		if (count >= 3 &&
		    ring_add_bit(&ring, bit) < 0)
			goto error;

		return 0;
	error:
		puts("ring full or invalid entries");
		ring_print_bits(&ring);
		ring_reset(&ring);
		started = 0;
		return -1;
	}

	if (count >= 36 && count <= 40) {
		static int tmp;

		if (bit) {
			started = 0;
			ring_reset(&ring);
			return -1;
		}
#if 1
		if (started) {
			if (ring_cmp(&ring, remote1_btn1,
				     sizeof(remote1_btn1)) == 0) {
				PORTD |= (1 << LED);
			} else if (ring_cmp(&ring, remote1_btn2,
					    sizeof(remote1_btn2)) == 0) {
				PORTD &= ~(1 << LED);
			}
		}
#endif
		started = 1;
#if 1
		if (tmp++ == 20) {
			ring_print(&ring);
			tmp = 0;
		}
#endif
		ring_reset(&ring);
		return FRAME_DELIMITER;
	}

	ring_reset(&ring);
	started = 0;
	return -1;
}

static inline void rf_sample(void)
{
	int v = analogRead(0);
	static int zero_cnt;
	static int one_cnt;
	static int prev_val;

	if (v < ANALOG_LOW_VAL) {
		//PORTD &= ~(1 << LED);
		if (prev_val == 1) {
			prev_val = 0;
			decode(1, one_cnt);
			one_cnt = 0;
		}
		zero_cnt++;
	} else if (v > ANALOG_HI_VAL) {
		//PORTD |= (1 << LED);
		if (prev_val == 0) {
			prev_val = 1;
			decode(0, zero_cnt);
			zero_cnt = 0;
		}
		one_cnt++;
	}
}

void tim_cb(void *arg)
{
	tim_t *timer = arg;
	//PORTD ^= (1 << LED);
	rf_sample();
	timer_reschedule(timer, TIMER_RESOLUTION_US);
}

int main(void)
{
	tim_t timer;

	/* led */
	DDRB = 1<<3; // port B3, ATtiny13a pin 2

	/* siren */
	DDRB |= 1; // port B0, ATtiny13a pin 0

	//speaker_init(); // port B1 (0x2)
	//start_song();

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
	timer_subsystem_init(TIMER_RESOLUTION_US);
	DDRD = (0x01 << LED); //Configure the PORTD4 as output

	memset(&timer, 0, sizeof(timer));
	timer_add(&timer, TIMER_RESOLUTION_US, tim_cb, &timer);

	while (1) {}

	return 0;
}
