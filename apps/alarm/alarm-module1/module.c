#include <stdio.h>
#include <scheduler.h>
#include <net/event.h>
#include <net/swen-l3.h>
#include <adc.h>
#include <sys/array.h>
#include "../module.h"
#include "../rf-common.h"

#define HUMIDITY_ANALOG_PIN 1
#define FAN_ON_DURATION ((3600 * 4) * 1000000UL) /* max 4h of activity */
#define HUMIDITY_SAMPLING (10 * 1000000UL) /* sample every 10s */
#define GLOBAL_HUMIDITY_ARRAY_LENGTH 60
#define DEFAULT_HUMIDITY_THRESHOLD 600
#define SIREN_ON_DURATION (15 * 60 * 1000000UL)

extern iface_t rf_iface;
extern uint32_t rf_enc_defkey[4];
static swen_l3_assoc_t assoc;
static uint8_t connected;
static uint8_t state;
static uint8_t fan_sensor_off;

static tim_t fan_timer;
static tim_t humidity_timer;
static tim_t siren_timer;
static int global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];
static uint8_t gha_pos;
static uint16_t humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;

typedef enum tendency {
	RINSING,
	FALLING,
} tendency_t;

typedef struct humidity_info {
	int val;
	uint8_t tendency;
} humidity_info_t;

#define ARRAY_LENGTH 20
static unsigned get_humidity_value(void)
{
	uint8_t i;
	int arr[ARRAY_LENGTH];

	/* get rid of the exterm values */
	for (i = 0; i < ARRAY_LENGTH; i++)
		arr[i] = analog_read(HUMIDITY_ANALOG_PIN);
	return array_get_median(arr, ARRAY_LENGTH);
}
#undef ARRAY_LENGTH

static void send_rf_msg(uint8_t cmd, const module_status_t *status)
{
	buf_t buf = BUF(sizeof(uint8_t) + sizeof(module_status_t));
	sbuf_t sbuf;

	__buf_addc(&buf, cmd);
	if (status)
		__buf_add(&buf, status, sizeof(module_status_t));
	sbuf = buf2sbuf(&buf);
	if (swen_l3_send(&assoc, &sbuf) < 0)
		connected = 0;
}

static inline void set_fan_off(void)
{
	PORTD &= ~(1 << PD3);
	timer_del(&fan_timer);
}

static void fan_off_cb(void *arg)
{
	set_fan_off();
}

static void set_fan_on(void)
{
	PORTD |= 1 << PD3;
	timer_del(&fan_timer);
	timer_add(&fan_timer, FAN_ON_DURATION, fan_off_cb, NULL);
}

static void set_siren_off(void)
{
	DDRB &= ~(1 << PB0);
}

static void siren_tim_cb(void *arg)
{
	set_siren_off();
}

static void set_siren_on(uint8_t force)
{
	if (state == MODULE_STATE_ARMED || force) {
		DDRB |= 1 << PB0;
		timer_del(&siren_timer);
		timer_add(&siren_timer, SIREN_ON_DURATION, siren_tim_cb, NULL);
		send_rf_msg(CMD_NOTIF_ALARM_ON, NULL);
	}
}

void pir_on_action(void *arg)
{
	set_siren_on(0);
}

ISR(PCINT2_vect) {
	if (PIND & (1 << PD1))
		schedule_task(pir_on_action, NULL);
	/* else if (!(PIND & (1 << PD1))) */
	/* 	schedule_task(pir_off_action, NULL); */
}

static uint8_t get_hum_tendency(void)
{
	return global_humidity_array[0]
		< global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
		? RINSING : FALLING;
}

static void get_humidity(humidity_info_t *info)
{
	unsigned val = get_humidity_value();

	if (gha_pos == GLOBAL_HUMIDITY_ARRAY_LENGTH) {
		array_left_shift(global_humidity_array,
				 GLOBAL_HUMIDITY_ARRAY_LENGTH, 1);
		global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1] = val;
	} else {
		global_humidity_array[gha_pos] = val;
		gha_pos++;
	}
	info->val = array_get_median(global_humidity_array,
				     GLOBAL_HUMIDITY_ARRAY_LENGTH);
	info->tendency = get_hum_tendency();
}

static void humidity_sampling(void *arg)
{
	humidity_info_t info;

	if (!fan_sensor_off) {
		get_humidity(&info);
		if (info.val > humidity_threshold && info.tendency == RINSING)
			set_fan_on();
		else
			set_fan_off();
	}
	timer_reschedule(&humidity_timer, HUMIDITY_SAMPLING);
}

static void get_status(module_status_t *status)
{
	status->humidity_val = get_humidity_value();
	status->global_humidity_val =
		global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH / 2];
	status->humidity_tendency = get_hum_tendency();
	status->state = state;
	status->fan_on = !!(PORTD | 1 << PD3);
	status->siren_on = !!(PORTB | 1 << PB0);
	status->fan_enabled = !fan_sensor_off;
	status->rf_up = connected;
	status->humidity_threshold = humidity_threshold;
}

static void handle_rf_commands(buf_t *buf)
{
	uint8_t cmd = buf_data(buf)[0];
	module_status_t status;

	switch (cmd) {
	case CMD_ARM:
		state = MODULE_STATE_ARMED;
		return;
	case CMD_DISARM:
		state = MODULE_STATE_DISARMED;
		set_siren_off();
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
		return;
	case CMD_ENABLE_FAN:
		fan_sensor_off = 0;
		return;
	case CMD_SIREN_ON:
		set_siren_on(1);
		return;
	case CMD_SIREN_OFF:
		set_siren_off();
		return;
	case CMD_SET_HUM_TH:
		if (buf_len(buf) >= 3)
			humidity_threshold = *(uint16_t *)(buf_data(buf) + 1);
		return;
	}
	send_rf_msg(cmd, &status);
}

static void rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ) {
		if (buf == NULL) {
			printf("%X connected\n", from);
			connected = 1;
			if (state == MODULE_STATE_INIT)
				state = MODULE_STATE_DISARMED;
			return;
		}
		handle_rf_commands(buf);
	} else if (events & EV_ERROR) {
		printf("%X error\n", from);
		connected = 0;
	}
}

void module_init(void)
{
	timer_add(&humidity_timer, HUMIDITY_SAMPLING, humidity_sampling, NULL);
	swen_ev_set(rf_event_cb);
	swen_l3_assoc_init(&assoc);
	swen_l3_assoc_bind(&assoc, RF_MOD0_HW_ADDR, &rf_iface, rf_enc_defkey);
	if (swen_l3_associate(&assoc) < 0)
		__abort();
}
