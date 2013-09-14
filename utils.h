#include <util/delay.h>

static inline void delay_us(int us)
{
	while (us--) {
		_delay_us(1);
	}
}

static inline void delay_ms(int ms)
{
	while (ms--) {
		_delay_ms(1);
	}
}
