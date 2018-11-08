#ifndef _COMMON_H_
#define _COMMON_H_

#include <avr/io.h>
#include <stdlib.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "utils.h"

#define __abort() do {							\
		DEBUG_LOG("%s:%d aborted\n", __func__, __LINE__);	\
		exit(1);						\
	} while (0)

#define assert(cond) do {			\
		if (!(cond)) {			\
			DEBUG_LOG("%s:%d assert failed\n",		\
				  __func__, __LINE__);			\
			exit(1);					\
		}							\
	} while (0)

#endif
