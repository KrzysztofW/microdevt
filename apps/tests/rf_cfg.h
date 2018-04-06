#ifndef _RF_CFG_H_
#define _RF_CFG_H_
#include "rf_common.h"

#define RF_SAMPLING_US 150
#define RF_RING_SIZE   256

static unsigned rcv_port;
static unsigned snd_port;

/* #define RF_ANALOG_SAMPLING */
#ifdef RF_ANALOG_SAMPLING
#define RF_RCV_PIN_NB 0
#define ANALOG_LOW_VAL 160
#define ANALOG_HI_VAL  630
#else
#define RF_RCV_PIN_NB 0
#define RF_RCV_PORT rcv_port
#define RF_RCV_PIN 0
#endif
#define RF_SND_PIN_NB 0
#define RF_SND_PORT snd_port

#endif
