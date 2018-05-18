#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdlib.h>

#define __abort() do {							\
		DEBUG_LOG("%s:%d aborted\n", __func__, __LINE__);	\
		abort();						\
	} while (0)

#define assert(cond) do {			\
		if (!(cond)) {			\
			DEBUG_LOG("%s:%d assert failed\n",		\
				  __func__, __LINE__);			\
			abort();					\
		}							\
	} while (0)

#endif
