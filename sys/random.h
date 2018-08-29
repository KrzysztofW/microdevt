#ifndef _RANDOM_H_
#define _RANDOM_H_

#ifdef CONFIG_AVR_MCU
extern unsigned long rnd_seed;
#else
extern unsigned int rnd_seed;
#endif

#endif
