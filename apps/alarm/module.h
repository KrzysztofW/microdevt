#ifndef _MODULE_H_
#define _MODULE_H_
#include <net/swen-l3.h>
#include <net/swen-rc.h>
#include "features.h"

typedef enum module_state {
	MODULE_STATE_DISARMED,
	MODULE_STATE_ARMED,
} module_state_t;

typedef struct __attribute__((__packed__)) module_status {
	uint16_t humidity_threshold;
	uint16_t humidity_val;
	uint16_t global_humidity_val;
	int8_t  temperature;
	uint8_t humidity_tendency : 1;
	uint8_t fan_on : 1;
	uint8_t fan_enabled : 1;
	uint8_t siren_on : 1;
	uint8_t state : 2;
	uint8_t lan_up : 1;
	uint8_t rf_up : 1;
} module_status_t;

typedef struct module {
	module_status_t status;
	const module_features_t *features;
#ifdef CONFIG_SWEN_ROLLING_CODES
	swen_rc_ctx_t rc_ctx;
	uint32_t local_counter;
	uint32_t remote_counter;
#else
	swen_l3_assoc_t assoc;
#endif
	char name[6];
} module_t;

typedef enum commands {
	CMD_ARM,
	CMD_DISARM,
	CMD_RUN_FAN,
	CMD_STOP_FAN,
	CMD_DISABLE_FAN,
	CMD_ENABLE_FAN,
	CMD_SIREN_ON,
	CMD_SIREN_OFF,
	CMD_GET_STATUS,
	CMD_STATUS,
	CMD_SET_HUM_TH,
	CMD_NOTIF_ALARM_ON,
	CMD_DISCONNECT,
	CMD_CONNECT,
} commands_t;

void master_module_init(void);
void module1_init(void);

#endif
