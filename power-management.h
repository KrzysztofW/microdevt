#ifndef _POWER_MANAGEMENT_H_
#define _POWER_MANAGEMENT_H_
#include <power-management.h>

extern uint16_t power_management_inactivity;

#define power_management_pwr_down_reset() power_management_inactivity = 0
#define power_management_pwr_down_set_idle() power_management_inactivity++
#define power_management_set_inactivity(value)	\
	power_management_inactivity = value

void
power_management_power_down_init(uint16_t inactivity_timeout,
				 void (*on_sleep)(void *arg), void *arg);

#endif
