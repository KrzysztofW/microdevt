#ifndef _COMMON_H_
#define _COMMON_H_

#include <avr/io.h>
#include <stdio.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "avr-utils.h"

#define __abort() do {							\
		DEBUG_LOG("%s:%d aborted\n", __func__, __LINE__);	\
		while (1) {}						\
	} while (0)

#define assert(cond) do {			\
		if (!(cond)) {			\
			DEBUG_LOG("%s:%d assert failed\n",		\
				  __func__, __LINE__);			\
			while (1) {}					\
		}							\
	} while (0)

#endif
