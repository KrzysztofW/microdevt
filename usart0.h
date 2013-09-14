#ifndef USART0_H
#define USART0_H

#include <stdint.h>

void usart0_init( uint16_t baud );

void usart0_put( uint8_t b );

uint8_t usart0_get( void );

int16_t usart0_get_delay( uint16_t max_delay );

#define BAUD_RATE(clockFreq, baud) ((uint16_t)(clockFreq/(16L*(baud))-1))

#endif
