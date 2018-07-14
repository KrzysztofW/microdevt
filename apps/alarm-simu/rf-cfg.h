#ifndef _RF_CFG_H_
#define _RF_CFG_H_
#include "rf-common.h"

#define RF_SAMPLING_US 150
#define RF_RING_SIZE   256

/* #define RF_ANALOG_SAMPLING */
#ifdef RF_ANALOG_SAMPLING
#define RF_RCV_PIN_NB 0
#define ANALOG_LOW_VAL 160
#define ANALOG_HI_VAL  630
#else
#define RF_RCV_PIN_NB PC0
#define RF_RCV_PORT PORTC
#define RF_RCV_PIN PINC
#endif
#define RF_SND_PIN_NB PD1
#define RF_SND_PORT PORTD

#endif
