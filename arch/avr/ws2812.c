#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <interrupts.h>
#include "ws2812.h"

/* Learn gcc inline assembler:
 * https://www.nongnu.org/avr-libc/user-manual/inline_asm.html
 */

#if defined(TRANSFER_ABORT) && F_CPU < 8000000
#error "TRANSFER_ABORT feature cannot be set for frequencies under 8MHz"
#endif

#define SHORT_PULSE_DELAY 350
#define LONG_PULSE_DELAY 900
#define TOTAL_TIME 1250

/* sbrs (1 cycle) + out (1 cycle) */
#define LOOP_SHORT_PULSE_CYCLES 2

/* sbrs (2 cycles) + add (1 cycle) + dec (1 cycle) + out (1 cycle) */
#ifndef TRANSFER_ABORT
#define LOOP_LONG_PULSE_CYCLES 5
#else
/* transfer_abort(lds + cpse) 1 cycle + 2 cycles */
#define LOOP_LONG_PULSE_CYCLES 8
#endif

/* sbrs 1 + out 1 + add 1 + dec 1 + out 1 + brne 2
 * + out (1 cycle) [start of the loop] */
#ifndef TRANSFER_ABORT
#define LOOP_TOTAL_TIME_CYCLES 8
#else
#define LOOP_TOTAL_TIME_CYCLES 11
#endif

#define SHORT_PULSE_CYCLES_NB (((F_CPU/1000000) * SHORT_PULSE_DELAY) / 1000)
#define LONG_PULSE_CYCLES_NB (((F_CPU/1000000) * LONG_PULSE_DELAY) / 1000)
#define BIT_TRANSFER_CYCLES_NB (((F_CPU/1000000) * TOTAL_TIME) / 1000)

#define _SHORT_PULSE_CYCLES_TO_ADD (SHORT_PULSE_CYCLES_NB - LOOP_SHORT_PULSE_CYCLES)
#define _LONG_PULSE_CYCLES_TO_ADD (LONG_PULSE_CYCLES_NB - LOOP_LONG_PULSE_CYCLES - _SHORT_PULSE_CYCLES_TO_ADD)
#define BIT_TRANSFER_CYCLES_NB_TO_ADD					\
	(BIT_TRANSFER_CYCLES_NB -					\
	 LOOP_TOTAL_TIME_CYCLES - _SHORT_PULSE_CYCLES_TO_ADD - _LONG_PULSE_CYCLES_TO_ADD)

#if (BIT_TRANSFER_CYCLES_NB_TO_ADD < 0)
#error "clock frequency too low"
#endif

#if _SHORT_PULSE_CYCLES_TO_ADD < 0
#define SHORT_PULSE_CYCLES_TO_ADD 0
#else
#define SHORT_PULSE_CYCLES_TO_ADD _SHORT_PULSE_CYCLES_TO_ADD
#endif

#if _LONG_PULSE_CYCLES_TO_ADD < 0
#define LONG_PULSE_CYCLES_TO_ADD 0
#else
#define LONG_PULSE_CYCLES_TO_ADD _LONG_PULSE_CYCLES_TO_ADD
#endif

/* notify that we don't insert more then 15 cycles */
#if (SHORT_PULSE_CYCLES_TO_ADD > 15) || (LONG_PULSE_CYCLES_TO_ADD > 15) ||	\
	(BIT_TRANSFER_CYCLES_NB_TO_ADD > 15)
#error "Clock frequency too hi"
#endif

#define ONE_CYCLE    "nop      \n"
#define TWO_CYCLES   "rjmp .+0 \n"
#define FOUR_CYCLES  TWO_CYCLES TWO_CYCLES
#define EIGHT_CYCLES FOUR_CYCLES FOUR_CYCLES

#define HI_VAL ((1 << WS2812_PIN) | WS2812_PORT)
#define LOW_VAL ((~(1 << WS2812_PIN)) & WS2812_PORT)

volatile uint8_t ws2812_n_transfer_abort = 0;

static inline void __ws2812_send(const void *data, unsigned len)
{
	while (len--) {
		uint8_t ctl, tmp;
		uint8_t byte = *(uint8_t *)(data++);

		asm volatile("ldi %0,8  \n"
			     "send_bit:    \n"
			     "out %5,%3 \n"
#if (SHORT_PULSE_CYCLES_TO_ADD & 1)
			     ONE_CYCLE
#endif
#if (SHORT_PULSE_CYCLES_TO_ADD & 2)
			     TWO_CYCLES
#endif
#if (SHORT_PULSE_CYCLES_TO_ADD & 4)
			     FOUR_CYCLES
#endif
#if (SHORT_PULSE_CYCLES_TO_ADD & 8)
			     EIGHT_CYCLES
#endif
			     "sbrs %2,7  \n"
			     "out  %5,%4 \n"
			     "add  %2,%2 \n"
			     "dec  %0    \n"
#ifdef TRANSFER_ABORT
			     "lds %1, ws2812_n_transfer_abort \n"
			     "sbrs %1,0  \n"
			     "rjmp end\n"
#endif
#if (LONG_PULSE_CYCLES_TO_ADD & 1)
			     ONE_CYCLE
#endif
#if (LONG_PULSE_CYCLES_TO_ADD & 2)
			     TWO_CYCLES
#endif
#if (LONG_PULSE_CYCLES_TO_ADD & 4)
			     FOUR_CYCLES
#endif
#if (LONG_PULSE_CYCLES_TO_ADD & 8)
			     EIGHT_CYCLES
#endif
			     "out %5,%4 \n"

#if (BIT_TRANSFER_CYCLES_NB_TO_ADD & 1)
			     ONE_CYCLE
#endif
#if (BIT_TRANSFER_CYCLES_NB_TO_ADD & 2)
			     TWO_CYCLES
#endif
#if (BIT_TRANSFER_CYCLES_NB_TO_ADD & 4)
			     FOUR_CYCLES
#endif
#if (BIT_TRANSFER_CYCLES_NB_TO_ADD & 8)
			     EIGHT_CYCLES
#endif
			     "brne send_bit\n"
			     : "=&d" (ctl),
			       "=&d" (tmp)
			     : "r" (byte),
			       "r" (HI_VAL),
			       "r" (LOW_VAL),
			       "I" (_SFR_IO_ADDR(WS2812_PORT))
			     );
	}
	__asm__ volatile("end:");
}

static inline void
__ws2812_send_multi(const void *data, unsigned length, uint8_t multi)
{
	while (multi--
#ifdef TRANSFER_ABORT
	       && ws2812_n_transfer_abort
#endif
	       )
		__ws2812_send(data, length);
}

void ws2812_send_rgb_multi(const rgb_t *leds, unsigned len, uint8_t multi)
{
	WS2812_ABORT_TRANSFER_RESET();
	__ws2812_send_multi(leds, len * 3, multi);
	WS2812_PORT &= ~(1 << WS2812_PIN);
}

void ws2812_send_rgb(const rgb_t *leds, unsigned len)
{
	WS2812_ABORT_TRANSFER_RESET();
	__ws2812_send(leds, len * 3);
	WS2812_PORT &= ~(1 << WS2812_PIN);
}

/* SK6812RGBW */
void ws2812_send_rgbw(const rgbw_t *leds, unsigned len)
{
	WS2812_ABORT_TRANSFER_RESET();
	__ws2812_send(leds, len * 4);
	WS2812_PORT &= ~(1 << WS2812_PIN);
}
