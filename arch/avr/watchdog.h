#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define WATCHDOG_TIMEOUT_15MS WDTO_15Ms
#define WATCHDOG_TIMEOUT_30MS WDTO_30MS
#define WATCHDOG_TIMEOUT_60MS WDTO_60MS
#define WATCHDOG_TIMEOUT_120MS WDTO_120MS
#define WATCHDOG_TIMEOUT_250MS WDTO_250MS
#define WATCHDOG_TIMEOUT_500MS WDTO_500MS
#define WATCHDOG_TIMEOUT_1S WDTO_1S
#define WATCHDOG_TIMEOUT_2S WDTO_2S
#define WATCHDOG_TIMEOUT_4S WDTO_4S
#define WATCHDOG_TIMEOUT_8S WDTO_8S

void watchdog_enable_interrupt(void (*cb)(void *arg), void *arg);

static inline void watchdog_enable_reset(void)
{
	WDTCSR |= _BV(WDE);
}

static inline void watchdog_disable_reset(void)
{
	WDTCSR &= ~(_BV(WDE));
}

static inline void watchdog_shutdown(void)
{
	wdt_reset();
	MCUSR = 0;
	WDTCSR |= _BV(WDCE);
	WDTCSR = 0;
}

static inline void watchdog_enable(uint8_t timeout)
{
	wdt_enable(timeout);
}

static inline void watchdog_reset(void)
{
	wdt_reset();
}

#endif
