#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdlib.h>

#ifdef X86
#define __LOG__ LOG
#else
#define __LOG__ DEBUG_LOG
#endif

#define __abort() do {							\
		__LOG__("%s:%d aborted\n", __func__, __LINE__);		\
		abort();						\
	} while (0)

#ifdef DEBUG
#define assert(cond) do {			\
		if (!(cond)) {			\
			DEBUG_LOG("%s:%d assert failed\n",		\
				  __func__, __LINE__);			\
			abort();					\
		}							\
	} while (0)
#else
#define assert(cond)
#endif

#endif
