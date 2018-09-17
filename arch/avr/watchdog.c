#include "watchdog.h"

static void (*wd_cb)(void *arg);
static void *wd_arg;

void watchdog_enable_interrupt(void (*cb)(void *arg), void *arg)
{
	WDTCSR |= _BV(WDIE);
	wd_cb = cb;
	wd_arg = arg;
}

ISR(WDT_vect) {
	wd_cb(wd_arg);
}
