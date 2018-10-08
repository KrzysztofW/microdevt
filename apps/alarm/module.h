#ifndef _MODULE_H_
#define _MODULE_H_
#include <net/swen-l3.h>
#include <net/swen-rc.h>
#include "features.h"

typedef enum module_state {
	MODULE_STATE_DISARMED = 1,
	MODULE_STATE_ARMED,
	MODULE_STATE_DISABLED = 3, /* 0xFF => default blank eeprom value */
} module_state_t;

typedef struct __attribute__((__packed__)) module_cfg {
	uint8_t  state : 2;
	uint8_t  fan_enabled : 1;
	uint16_t siren_duration;
	uint16_t humidity_report_interval;
	uint8_t  humidity_threshold;
} module_cfg_t;

enum humidity_tendency {
	HUMIDITY_TENDENCY_STABLE,
	HUMIDITY_TENDENCY_RISING,
	HUMIDITY_TENDENCY_FALLING,
};

static inline const char *humidity_tendency_to_str(uint8_t tendency)
{
	switch (tendency) {
	case HUMIDITY_TENDENCY_STABLE:
		return "STABLE";
	case HUMIDITY_TENDENCY_RISING:
		return "RISING";
	case HUMIDITY_TENDENCY_FALLING:
		return "FALLING";
	default:
		return NULL;
	}
}

typedef struct __attribute__((__packed__)) module_status {
	module_cfg_t cfg;
	uint8_t humidity_val;
	uint8_t global_humidity_val;
	int8_t  temperature;
	uint8_t humidity_tendency : 2;
	uint8_t fan_on : 1;
	uint8_t siren_on : 1;
	uint8_t lan_up : 1;
	uint8_t rf_up : 1;
} module_status_t;

typedef struct module {
	const module_features_t *features;
#ifdef CONFIG_SWEN_ROLLING_CODES
	swen_rc_ctx_t rc_ctx;
	uint32_t local_counter;
	uint32_t remote_counter;
#else
	swen_l3_assoc_t assoc;
#endif
#define OP_QUEUE_SIZE 4
	ring_t  op_queue;
	uint8_t op_queue_data[OP_QUEUE_SIZE];
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
	CMD_SET_SIREN_DURATION,
	CMD_GET_STATUS,
	CMD_STATUS,
	CMD_SET_HUM_TH,
	CMD_NOTIF_ALARM_ON,
	CMD_DISCONNECT,
	CMD_CONNECT,
	CMD_GET_REPORT_HUM_VAL,
	CMD_REPORT_HUM_VAL,
} commands_t;

void master_module_init(void);
void module1_init(void);

#endif
