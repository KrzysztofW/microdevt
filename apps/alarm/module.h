#ifndef _MODULE_H_
#define _MODULE_H_
#include <net/swen-l3.h>
#include <net/swen-rc.h>
#include <interrupts.h>
#include "module-common.h"

typedef enum module_state {
	MODULE_STATE_DISARMED,
	MODULE_STATE_ARMED,
	MODULE_STATE_DISABLED,
	MODULE_STATE_UNINITIALIZED,
} module_state_t;

typedef struct __attribute__((__packed__)) module_cfg {
	features_t features;
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

typedef enum status_flags {
	STATUS_STATE_CONN_LAN_UP = 1,
	STATUS_STATE_CONN_RF_UP  = (1 << 1),
	STATUS_STATE_FAN_ON      = (1 << 2),
	STATUS_STATE_FAN_ENABLED = (1 << 3),
	STATUS_STATE_SIREN_ON = (1 << 5),
} status_flags_t;

typedef struct __attribute__((__packed__)) humidity_status {
	uint16_t report_interval;
	uint8_t  threshold;
	uint8_t  val;
	uint8_t  global_val;
	uint8_t  tendency;
} humidity_status_t;

typedef struct __attribute__((__packed__)) module_status {
	uint8_t  flags;
	uint8_t  state;
	humidity_status_t humidity;
	int8_t   temperature;
	uint16_t siren_duration;
} module_status_t;

typedef struct __attribute__((__packed__)) module_register {
	uint8_t cmd;
	uint8_t features;
	uint16_t req_id;
} module_register_t;

typedef struct __attribute__((__packed__)) module_addr_t {
	uint8_t cmd;
	uint8_t addr;
	uint16_t req_id;
} module_addr_t;

typedef struct module {
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
	CMD_REGISTER_DEVICE,
	CMD_SET_ADDR,
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
