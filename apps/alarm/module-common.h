/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#ifndef _MODULE_COMMON_H_
#define _MODULE_COMMON_H_
#include <stdint.h>
#include <net/if.h>
#include <net/swen-l3.h>
#include <interrupts.h>

#define RF_MASTER_MOD_HW_ADDR 0x01
#define RF_GENERIC_MOD_HW_ADDR 0x02
#define RF_INIT_ADDR 0xFE

#ifndef CONFIG_AVR_SIMU
#define ONE_SECOND 1000000
#else
/* accelerate the processing on simulator */
#define ONE_SECOND 100000
#endif
#define ONE_MINUTE (60 * ONE_SECOND)

#define FAN_ON_DURATION (3 * 3600) /* 3 hours of activity */
#define SENSOR_SAMPLING 60
#define GLOBAL_HUMIDITY_ARRAY_LENGTH 30
#define MIN_HUMIDITY_VALUE 60
#define MAX_HUMIDITY_VALUE 80
#define INIT_TIME 60
#define WD_TIMEOUT 8 /* watchdog timeout set in main.c */
#define INACTIVITY_TIMEOUT 120

enum features {
	MODULE_FEATURE_HUMIDITY =       1,
	MODULE_FEATURE_TEMPERATURE =   (1 << 1),
	MODULE_FEATURE_FAN =           (1 << 2),
	MODULE_FEATURE_SIREN =         (1 << 3),
	MODULE_FEATURE_LAN =           (1 << 4),
	MODULE_FEATURE_RF =            (1 << 5),
	MODULE_FEATURE_ROLLING_CODES = (1 << 6),
};
typedef enum __PACKED__ features features_t;

#define DEFAULT_HUMIDITY_THRESHOLD 2
#define DEFAULT_SIREN_ON_DURATION 60
#define DEFAULT_SIREN_ON_TIMEOUT 5

typedef enum module_state {
	MODULE_STATE_DISARMED,
	MODULE_STATE_ARMED,
	MODULE_STATE_DISABLED,
	MODULE_STATE_UNINITIALIZED,
} module_state_t;

typedef struct __PACKED__ sensor_status {
	uint8_t  humidity_threshold;
	uint16_t report_interval;
	uint8_t  notif_addr;
} sensor_status_t;

typedef struct __PACKED__ module_cfg {
	features_t features;
	uint8_t  state : 2;
	uint8_t  fan_enabled : 1;
	uint16_t siren_duration;
	uint8_t  siren_timeout;
	sensor_status_t sensor;
	uint8_t  reboot_counter;
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
	STATUS_FLAGS_CONN_LAN_UP = 1,
	STATUS_FLAGS_CONN_RF_UP  = (1 << 1),
	STATUS_FLAGS_FAN_ON      = (1 << 2),
	STATUS_FLAGS_FAN_ENABLED = (1 << 3),
	STATUS_FLAGS_SIREN_ON = (1 << 5),
	STATUS_FLAGS_MAIN_PWR_ON = (1 << 6),
} status_flags_t;

typedef struct __PACKED__ sensor_report {
	uint8_t humidity;
	int8_t  temperature;
} sensor_report_t;

typedef struct __PACKED__ humidity_value {
	uint8_t val;
	uint8_t global_val;
	uint8_t tendency;
} humidity_value_t;

typedef struct __PACKED__ sensor_value_t {
	humidity_value_t humidity;
	int8_t temperature;
} sensor_value_t;

typedef struct __PACKED__ siren_status {
	uint16_t duration;
	uint8_t  timeout;
} siren_status_t;

typedef struct __PACKED__ module_status {
	uint8_t  flags;
	uint8_t  state;
	uint8_t  reboot_counter;
	siren_status_t siren;
	sensor_status_t sensor;
} module_status_t;

#define POWER_STATE_NONE 0
#define POWER_STATE_ON   1
#define POWER_STATE_OFF  2

typedef struct module {
#ifdef CONFIG_SWEN_ROLLING_CODES
	swen_rc_ctx_t rc_ctx;
	uint32_t local_counter;
	uint32_t remote_counter;
#else
	swen_l3_assoc_t assoc;
#endif
	/* the size of the queue should be >= sizeof(module_cfg_t) */
#define OP_QUEUE_SIZE 8
	RING_DECL_IN_STRUCT(op_queue, OP_QUEUE_SIZE);
	uint8_t main_pwr_state : 2;
	uint8_t faulty : 1;
} module_t;

typedef enum commands {
	CMD_GET_FEATURES,
	CMD_FEATURES,
	CMD_ARM,
	CMD_DISARM,
	CMD_RUN_FAN,
	CMD_STOP_FAN,
	CMD_DISABLE_FAN,
	CMD_ENABLE_FAN,
	CMD_SIREN_ON,
	CMD_SIREN_OFF,
	CMD_SET_SIREN_DURATION, // 10
	CMD_SET_SIREN_TIMEOUT,
	CMD_GET_STATUS,
	CMD_STATUS,
	CMD_SET_HUM_TH,
	CMD_NOTIF_ALARM_ON,
	CMD_DISCONNECT,
	CMD_CONNECT,
	CMD_DISABLE,
	CMD_GET_SENSOR_REPORT,
	CMD_SENSOR_REPORT, // 20
	CMD_NOTIF_MAIN_PWR_DOWN,
	CMD_NOTIF_MAIN_PWR_UP,
	CMD_DISABLE_PWR_DOWN,
	CMD_ENABLE_PWR_DOWN,
	CMD_STORAGE_ERROR,
	CMD_RECORD_GENERIC_COMMAND,
	CMD_RECORD_GENERIC_COMMAND_ANSWER,
	CMD_GENERIC_COMMANDS_LIST,
	CMD_GENERIC_COMMANDS_DELETE,
	CMD_GET_SENSOR_VALUES,
	CMD_SENSOR_VALUES,
} commands_t;

#ifdef CONFIG_RF_GENERIC_COMMANDS
static uint8_t recordable_cmds[] = {
	CMD_DISARM, CMD_ARM, CMD_RUN_FAN, CMD_STOP_FAN, CMD_SIREN_ON,
	CMD_SIREN_OFF,
};

static inline uint8_t cmd_is_recordable(uint8_t cmd)
{
	uint8_t i;

	for (i = 0; i < sizeof(recordable_cmds); i++) {
		if (recordable_cmds[i] == cmd)
			return 1;
	}
	return 0;
}
#endif

extern const uint32_t rf_enc_defkey[4];

static inline uint8_t module_id_to_addr(uint8_t id)
{
	return RF_MASTER_MOD_HW_ADDR + id;
}

static inline uint8_t addr_to_module_id(uint8_t addr)
{
	return addr - RF_MASTER_MOD_HW_ADDR;
}

void module_init_iface(iface_t *iface, uint8_t *addr);
void module_init_debug_iface(iface_t *iface, uint8_t *addr);
int module_check_magic(void);
int8_t module_update_magic(void);

int module_add_op(ring_t *queue, uint8_t op, event_t *event);
int module_get_op(ring_t *queue, uint8_t *op);
void module_skip_op(ring_t *queue);
int module_get_op2(uint8_t *op, ring_t *q1, ring_t *q2);
void module_skip_op2(ring_t *q1, ring_t *q2);
static inline int module_op_pending(ring_t *queue)
{
	return !ring_is_empty(queue);
}
int
send_rf_msg(swen_l3_assoc_t *assoc, uint8_t cmd, const void *data, int len);
void module_set_default_cfg(module_cfg_t *cfg);
void master_module_init(void);
void module1_init(void);

#endif
