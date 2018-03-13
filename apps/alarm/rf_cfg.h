#ifndef _RF_CFG_H_
#define _RF_CFG_H_
#include "rf_common.h"

#define RF_HW_ADDR RF_MOD0_HW_ADDR
/* #define RF_ANALOG_SAMPLING */
#ifdef RF_ANALOG_SAMPLING
#define RF_RCV_PIN_NB 0
#else
#define RF_RCV_PIN_NB PC0
#define RF_RCV_PORT PORTC
#define RF_RCV_PIN PINC
#endif
#define RF_SND_PIN_NB PF1
#define RF_SND_PORT PORTF

#endif
