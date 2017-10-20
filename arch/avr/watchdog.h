#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

static inline void watchdog_shutdown(void)
{
	wdt_reset();
	MCUSR=0;
	WDTCSR|=_BV(WDCE) | _BV(WDE);
	WDTCSR=0;
}

static inline void watchdog_enable(void)
{
	wdt_enable(WDTO_8S);
}

static inline void watchdog_reset(void)
{
	wdt_reset();
}

#endif
