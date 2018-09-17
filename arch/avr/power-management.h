#ifndef _AVR_POWER_MANAGEMENT_H_
#define _AVR_POWER_MANAGEMENT_H_
#include <avr/sleep.h>
#include "../../power-management.h"
#include "common.h"

#define PWR_MGR_SLEEP_MODE_IDLE SLEEP_MODE_IDLE
#define PWR_MGR_SLEEP_MODE_ADC SLEEP_MODE_ADC
#define PWR_MGR_SLEEP_MODE_PWR_DOWN SLEEP_MODE_PWR_DOWN
#define PWR_MGR_SLEEP_MODE_SAVE SLEEP_MODE_PWR_SAVE
#define PWR_MGR_SLEEP_MODE_STANDBY SLEEP_MODE_STANDBY
#define PWR_MGR_SLEEP_MODE_EXT_STANDY SLEEP_MODE_EXT_STANDBY

#define power_management_set_mode(mode) set_sleep_mode(mode)
#define power_management_sleep() sleep_mode()

#endif
