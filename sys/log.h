#ifndef _SYS_LOG_H_
#define _SYS_LOG_H_

#ifdef CONFIG_AVR_MCU
#include <avr/pgmspace.h>

#define LOG(s, ...) printf_P(PSTR(s), ##__VA_ARGS__)
#else
#define LOG(s, ...) printf(s, ##__VA_ARGS__)
#endif

#endif
