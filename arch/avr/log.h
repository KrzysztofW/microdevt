#ifndef _SYS_LOG_H_
#define _SYS_LOG_H_

#include <avr/pgmspace.h>

#define LOG(s, ...) printf_P(PSTR(s), ##__VA_ARGS__)

#endif
