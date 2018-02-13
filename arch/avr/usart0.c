#include <avr/io.h>
#include "usart0.h"

#define HI(X) (X >> 8)
#define LO(X) (X & 0xFF)

void usart0_init(uint16_t baud)
{
	DDRD |= _BV(1) ; /* enable tx output */
	/* UBRR0H = HI(baud); UBRR0L = LO(baud) */
	UBRR0 = baud;
	/* Asynchronous mode, no parity, 1-stop bit, 8-bit data */
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	/* no 2x mode, no multi-processor mode */
	UCSR0A = 0x00;

	/* interrupts disabled, rx and tx enabled, 8-bit data */
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
}

void usart0_put(uint8_t b)
{
	/* wait for data register to be ready */
	while ((UCSR0A & _BV(UDRE0)) == 0) {}
	/* load b for transmission */
	UDR0 = b;
}

uint8_t usart0_get(void)
{
	/* poll for available data */
	while ((UCSR0A & _BV(RXC0)) == 0) {}
	return UDR0;
}
#if 0 /* unsued so far */
int16_t usart0_get_delay(uint16_t max_delay)
{
	/* poll for available data, with timeout */
	while ((UCSR0A & _BV(RXC0)) == 0  && max_delay != 0)
		max_delay--;

	if ((UCSR0A & _BV(RXC0)) == 0 )
		return -1;

	return UDR0;
}
#endif
