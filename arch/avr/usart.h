#ifndef _USART0_H_
#define _USART0_H_

#include <stdint.h>

#define BAUD_RATE(clockFreq, baud) ((uint16_t)(clockFreq/(16L*(baud))-1))

void usart0_init(uint16_t baud, uint8_t interrupt);
void usart0_put(uint8_t b);
int usart0_get(uint8_t *c);
void usart1_init(uint16_t baud, uint8_t interrupt);
void usart1_put(uint8_t b);
int usart1_get(uint8_t *c);

#endif
