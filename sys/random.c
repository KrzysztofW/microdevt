#include "random.h"

#ifdef CONFIG_AVR_MCU
unsigned long rnd_seed = 0xAAFF;
#else
extern unsigned int rnd_seed = 0xAAFF;
#endif
