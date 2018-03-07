#ifndef _USART0_H_
#define _USART0_H_

#include <stdint.h>

#define BAUD_RATE(clockFreq, baud) ((uint16_t)(clockFreq/(16L*(baud))-1))

void usart0_init(uint16_t baud);
void usart0_put(uint8_t b);
uint8_t usart0_get(void);
void usart1_init(uint16_t baud);
void usart1_put(uint8_t b);
uint8_t usart1_get(void);

#endif
