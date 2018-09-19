#include <stdio.h>
#include <scheduler.h>
#include <net/event.h>
#ifdef CONFIG_SWEN_ROLLING_CODES
#include <net/swen.h>
#else
#include <net/swen-l3.h>
#endif
#include <adc.h>
#include <sys/array.h>
#include <power-management.h>
#include <watchdog.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "../module.h"
#include "../rf-common.h"

#define ONE_SECOND 1000000
#define ONE_HOUR (3600 * 1000000U)
#define HUMIDITY_ANALOG_PIN 1
#define FAN_ON_DURATION_H 4 /* max 4h of activity */
#define HUMIDITY_SAMPLING 30 /* sample every 30s */
#define GLOBAL_HUMIDITY_ARRAY_LENGTH 30
#define DEFAULT_HUMIDITY_THRESHOLD 20
#define MAX_HUMIDITY_VALUE 676 /* => 80% RH */
#define HIH_4000_TO_RH(mv_val) (((mv_val) - 826) / 31)

#define SIREN_ON_DURATION (1 * 60 * 1000000U) /* 1 minute */
/* inactivity timeout in seconds */
#define INACTIVITY_TIMEOUT 15

extern iface_t rf_iface;
extern uint32_t rf_enc_defkey[4];
static uint8_t state;
static uint8_t fan_sensor_off = 1;
static uint8_t fan_off_hour_cnt = FAN_ON_DURATION_H;

static tim_t fan_timer;
static tim_t siren_timer;
static tim_t timer_1sec;
static int global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];
static uint8_t prev_tendency;
static uint8_t humidity_sampling_update;
static uint16_t humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
#ifdef CONFIG_SWEN_ROLLING_CODES
static swen_rc_ctx_t rc_ctx;
static uint32_t local_counter;
static uint32_t remote_counter;
static uint8_t connected = 1;
#else
static swen_l3_assoc_t assoc;
static uint8_t connected;
#endif

typedef struct __attribute__((__packed__)) persistent_data {
	uint8_t armed : 1;
	uint8_t fan_enabled : 1;
	uint16_t humidity_threshold;
} persistent_data_t;
static persistent_data_t EEMEM persistent_data;

typedef enum tendency {
	STABLE,
	RISING,
	FALLING,
} tendency_t;

typedef struct humidity_info {
	int val;
	uint8_t tendency;
} humidity_info_t;

#define ARRAY_LENGTH 20
static unsigned get_humidity_cur_value(void)
{
	uint8_t i;
	int arr[ARRAY_LENGTH];

	/* get rid of the extrem values */
	for (i = 0; i < ARRAY_LENGTH; i++)
		arr[i] = analog_read(HUMIDITY_ANALOG_PIN);
	return array_get_median(arr, ARRAY_LENGTH);
}
#undef ARRAY_LENGTH

static void humidity_sampling_task_cb(void *arg);

#ifdef CONFIG_POWER_MANAGEMENT
void watchdog_on_wakeup(void *arg)
{
	static uint8_t sampling_cnt;

	DEBUG_LOG("WD interrupt\n");

	/* sample every 30 seconds ((8-sec sleep + 2 secs below) * 3) */
	if (sampling_cnt >= 3) {
		schedule_task(humidity_sampling_task_cb, NULL);
		sampling_cnt = 0;
	} else
		sampling_cnt++;

	/* stay active for 2 seconds in order to catch incoming RF packets */
	power_management_set_inactivity(INACTIVITY_TIMEOUT - 2);
}

static void pwr_mgr_on_sleep(void *arg)
{
	DEBUG_LOG("sleeping...\n");
	PORTD &= ~(1 << PD4);
	watchdog_enable_interrupt(watchdog_on_wakeup, NULL);
}
#endif

static void timer_1sec_cb(void *arg)
{
	if (humidity_sampling_update++ >= HUMIDITY_SAMPLING)
		schedule_task(humidity_sampling_task_cb, NULL);

	/* toggle the LED */
	PORTD ^= 1 << PD4;

#ifdef CONFIG_POWER_MANAGEMENT
	/* do not sleep if the siren is on */
	if (!!(PORTB & 1 << PB0))
		power_management_pwr_down_reset();
#endif
	timer_reschedule(&timer_1sec, ONE_SECOND);
}

static void send_rf_msg(uint8_t cmd, const module_status_t *status)
{
	buf_t buf = BUF(sizeof(uint8_t) + sizeof(module_status_t));
	sbuf_t sbuf;

	__buf_addc(&buf, cmd);
	if (status)
		__buf_add(&buf, status, sizeof(module_status_t));
	sbuf = buf2sbuf(&buf);
#ifdef CONFIG_SWEN_ROLLING_CODES
	if (swen_rc_sendto(&rc_ctx, &sbuf) >= 0)
		remote_counter++;
#else
	if (swen_l3_send(&assoc, &sbuf) < 0)
		connected = 0;
#endif
}

static inline void set_fan_off(void)
{
	PORTD &= ~(1 << PD3);
	timer_del(&fan_timer);
	fan_off_hour_cnt = FAN_ON_DURATION_H;
}

static void fan_off_cb(void *arg)
{
	if (fan_off_hour_cnt) {
		fan_off_hour_cnt--;
		timer_reschedule(&fan_timer, ONE_HOUR);
		return;
	}
	set_fan_off();
	fan_off_hour_cnt = FAN_ON_DURATION_H;
}

static void set_fan_on(void)
{
	PORTD |= 1 << PD3;
	timer_del(&fan_timer);
	timer_add(&fan_timer, ONE_HOUR, fan_off_cb, NULL);
}

static void set_siren_off(void)
{
	PORTB &= ~(1 << PB0);
}

static void siren_tim_cb(void *arg)
{
	set_siren_off();
}

static void set_siren_on(uint8_t force)
{
	if (PORTB & (1 << PB0))
		return;
	if (state == MODULE_STATE_ARMED || force) {
		PORTB |= 1 << PB0;
		timer_del(&siren_timer);
		timer_add(&siren_timer, SIREN_ON_DURATION, siren_tim_cb, NULL);
		if (state == MODULE_STATE_ARMED && !force)
			send_rf_msg(CMD_NOTIF_ALARM_ON, NULL);
	}
}

void pir_on_action(void *arg)
{
	set_siren_on(0);
}

ISR(PCINT2_vect) {
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
	if (PIND & (1 << PD1))
		schedule_task(pir_on_action, NULL);
}

static uint8_t get_hum_tendency(void)
{
	int val = global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
		- global_humidity_array[0];

	if (abs(val) < DEFAULT_HUMIDITY_THRESHOLD)
		return STABLE;
	if (val < 0)
		return FALLING;
	return RISING;
}

static void get_humidity(humidity_info_t *info)
{
	unsigned val = get_humidity_cur_value();
	uint8_t tendency;

	array_left_shift(global_humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH, 1);
	global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1] = val;
	tendency = get_hum_tendency();
	if (info->tendency != tendency)
		prev_tendency = info->tendency;
	info->tendency = tendency;
}

static void humidity_sampling_task_cb(void *arg)
{
	humidity_info_t info;

	humidity_sampling_update = 0;
	get_humidity(&info);
	if (fan_sensor_off || global_humidity_array[0] == 0)
		return;
	if (info.tendency == RISING ||
	    global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
	    >= MAX_HUMIDITY_VALUE) {
		set_fan_on();
	} else if (info.tendency == STABLE)
		set_fan_off();
}

static void get_status(module_status_t *status)
{
	int humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];

	status->humidity_val = get_humidity_cur_value();

	array_copy(humidity_array, global_humidity_array,
		   GLOBAL_HUMIDITY_ARRAY_LENGTH);
	status->global_humidity_val =
		array_get_median(humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH);

	status->humidity_tendency = get_hum_tendency();
	status->state = state;
	status->fan_on = !!(PORTD & 1 << PD3);
	status->siren_on = !!(PORTB & 1 << PB0);
	status->fan_enabled = !fan_sensor_off;
	status->rf_up = connected;
	status->humidity_threshold = humidity_threshold;
}

static void update_storage(void)
{
	persistent_data_t data = {
		.armed = (state == MODULE_STATE_ARMED),
		.fan_enabled = !fan_sensor_off,
		.humidity_threshold = humidity_threshold,
	};
	eeprom_write_block(&data , &persistent_data, sizeof(persistent_data_t));
}

static void reload_cfg_from_storage(void)
{
	persistent_data_t data;
	uint8_t i;
	uint8_t invalid = 1;
	uint8_t *bytes = (uint8_t *)&data;

	eeprom_read_block(&data, &persistent_data, sizeof(persistent_data_t));
	for (i = 0; i < sizeof(data); i++) {
		if (bytes[i] != 0xFF) {
			invalid = 0;
			break;
		}
	}
	if (invalid)
		return;

	if (data.armed)
		state = MODULE_STATE_ARMED;
	fan_sensor_off = !data.fan_enabled;
	humidity_threshold = data.humidity_threshold;
}

#ifdef DEBUG
void module_print_status(void)
{
	int humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];
	long hval = get_humidity_cur_value();

	array_copy(humidity_array, global_humidity_array,
		   GLOBAL_HUMIDITY_ARRAY_LENGTH);
	array_print(humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH);
	LOG("\nStatus:\n");
	LOG(" State:  %s\n",
	    state == MODULE_STATE_ARMED ? "armed" : "disarmed");
	LOG(" Humidity value:  %ld%%\n"
	    " Global humidity value:  %d\n"
	    " Humidity tendency:  %u\n"
	    " Humidity threshold:  %u\n",
	    HIH_4000_TO_RH(analog_to_millivolt(hval)),
	    array_get_median(humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH),
	    get_hum_tendency(),
	    humidity_threshold);
	LOG(" Fan: %d\n Fan enabled: %d\n", !!(PORTD & 1 << PD3),
	    !fan_sensor_off);
	LOG(" Siren:  %d\n", !!(PORTB & 1 << PB0));
	LOG(" RF:  %d\n", connected);
}
#endif

static void arm_cb(void *arg)
{
	static uint8_t on;

	if (on == 0) {
		if (PORTB & (1 << PB0))
			return;
		PORTB |= 1 << PB0;
		timer_reschedule(&siren_timer, 10000);
		on = 1;
	} else {
		PORTB &= ~(1 << PB0);
		on = 0;
	}
}

void module_arm(uint8_t on)
{
	if (on)
		state = MODULE_STATE_ARMED;
	else {
		state = MODULE_STATE_DISARMED;
		set_siren_off();
	}
	update_storage();
	timer_del(&siren_timer);
	timer_add(&siren_timer, 10000, arm_cb, NULL);
}

void handle_rf_commands(uint8_t cmd, uint16_t value)
{
	module_status_t status;

	switch (cmd) {
	case CMD_ARM:
		module_arm(1);
		return;
	case CMD_DISARM:
		module_arm(0);
		return;
	case CMD_STATUS:
	case CMD_NOTIF_ALARM_ON:
		/* unsupported */
		return;
	case CMD_GET_STATUS:
		cmd = CMD_STATUS;
		get_status(&status);
		break;
	case CMD_RUN_FAN:
		set_fan_on();
		return;
	case CMD_STOP_FAN:
		set_fan_off();
		return;
	case CMD_DISABLE_FAN:
		fan_sensor_off = 1;
		update_storage();
		return;
	case CMD_ENABLE_FAN:
		fan_sensor_off = 0;
		update_storage();
		return;
	case CMD_SIREN_ON:
		set_siren_on(1);
		return;
	case CMD_SIREN_OFF:
		set_siren_off();
		return;
	case CMD_SET_HUM_TH:
		if (value) {
			humidity_threshold = value;
			update_storage();
		}
		return;
	}
	send_rf_msg(cmd, &status);
}

static void handle_rf_buf_commands(buf_t *buf)
{
	uint8_t cmd = buf_data(buf)[0];
	uint16_t value = 0;

	if (buf_len(buf) >= 3)
		value = *(uint16_t *)(buf_data(buf) + 1);

	handle_rf_commands(cmd, value);
}
static void rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
#ifdef CONFIG_SWEN_ROLLING_CODES
	if (events & EV_READ) {
		local_counter++;
		handle_rf_buf_commands(buf);
	}
#else
	if (events & EV_READ) {
		if (buf == NULL) {
			connected = 1;
			if (state == MODULE_STATE_INIT)
				state = MODULE_STATE_DISARMED;
			return;
		}
		handle_rf_buf_commands(buf);
	} else if (events & EV_ERROR) {
		connected = 0;
	}
#endif
}

#ifdef CONFIG_SWEN_ROLLING_CODES
static void set_rc_cnt(uint32_t *counter, uint8_t value)
{
	*counter += value;
	// eeprom ...
}
#endif

void module_init(void)
{
	reload_cfg_from_storage();
	timer_add(&timer_1sec, ONE_SECOND, timer_1sec_cb, NULL);
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_power_down_init(INACTIVITY_TIMEOUT, pwr_mgr_on_sleep,
					 NULL);
#endif
	swen_ev_set(rf_event_cb);
#ifdef CONFIG_SWEN_ROLLING_CODES
	swen_rc_init(&rc_ctx, &rf_iface, RF_MOD0_HW_ADDR,
		     &local_counter, &remote_counter, set_rc_cnt,
		     rf_enc_defkey);
#else
	swen_l3_assoc_init(&assoc, rf_enc_defkey);
	swen_l3_assoc_bind(&assoc, RF_MOD0_HW_ADDR, &rf_iface);
	swen_l3_set_events(&assoc, EV_READ | EV_WRITE);
	if (swen_l3_associate(&assoc) < 0)
		__abort();
#endif
}
