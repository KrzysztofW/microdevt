#ifndef _SYS_LOG_H_
#define _SYS_LOG_H_

#include <stdio.h>
#include <avr/pgmspace.h>

#define LOG(s, ...) printf_P(PSTR(s), ##__VA_ARGS__)

#ifdef DEBUG
#define DEBUG_LOG LOG
#else
#define DEBUG_LOG(s, ...)
#endif

#endif
