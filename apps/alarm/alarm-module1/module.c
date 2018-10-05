#include <stdio.h>
#include <scheduler.h>
#include <net/event.h>
#include <net/swen-l3.h>
#include <net/swen-cmds.h>
#include <adc.h>
#include <sys/array.h>
#include <power-management.h>
#include <watchdog.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "../module.h"
#include "../rf-common.h"
#include "../module-common.h"

#define ONE_SECOND 1000000
#define ONE_HOUR (3600 * 1000000U)
#define HUMIDITY_ANALOG_PIN 1
#define FAN_ON_DURATION (4 * 3600) /* max 4h of activity */
#define HUMIDITY_SAMPLING 30 /* sample every 30s */
#define GLOBAL_HUMIDITY_ARRAY_LENGTH 30
#define DEFAULT_HUMIDITY_THRESHOLD 3
#define MAX_HUMIDITY_VALUE 80
#define HIH_4000_TO_RH(mv_val) (((mv_val) - 826) / 31)

#define SIREN_ON_DURATION (1 * 60 * 1000000U) /* 1 minute */
/* inactivity timeout in seconds */
#define INACTIVITY_TIMEOUT 15

static uint8_t rf_addr = RF_MOD1_HW_ADDR;
static iface_t rf_iface;
#ifdef RF_DEBUG
iface_t *rf_debug_iface2 = &rf_iface;
#endif

static module_cfg_t module_cfg;
static uint16_t fan_sec_cnt;
static tim_t siren_timer;
static tim_t timer_1sec;
static int global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];
static uint8_t prev_tendency;
static uint8_t humidity_sampling_update;
static swen_l3_assoc_t mod1_assoc;
static uint16_t report_hval_interval;
static uint16_t report_hval_elapsed_secs;

static module_cfg_t EEMEM persistent_data;

typedef struct humidity_info {
	int val;
	uint8_t tendency;
} humidity_info_t;

#define ARRAY_LENGTH 20
static uint8_t get_humidity_cur_value(void)
{
	uint8_t i;
	int arr[ARRAY_LENGTH];
	unsigned val;

	/* get rid of the extrem values */
	for (i = 0; i < ARRAY_LENGTH; i++)
		arr[i] = analog_read(HUMIDITY_ANALOG_PIN);
	val = array_get_median(arr, ARRAY_LENGTH);
	return HIH_4000_TO_RH(analog_to_millivolt(val));
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

	if (fan_sec_cnt > 8)
		fan_sec_cnt -= 8;
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

static void report_hum_value(void)
{
	if (report_hval_interval == 0)
		return;
	if (report_hval_elapsed_secs >= report_hval_interval) {
		uint8_t cur_op;

		report_hval_elapsed_secs = 0;
		if (module_get_op(&cur_op) >= 0 && cur_op == CMD_REPORT_HUM_VAL)
			return;
		module_add_op(CMD_REPORT_HUM_VAL, 0);
		swen_l3_event_set_mask(&mod1_assoc, EV_READ | EV_WRITE);
	} else
		report_hval_elapsed_secs++;
}

static inline void set_fan_off(void)
{
	PORTD &= ~(1 << PD3);
}

static void set_fan_on(void)
{
	PORTD |= 1 << PD3;
	fan_sec_cnt = FAN_ON_DURATION;
}

static void timer_1sec_cb(void *arg)
{
	timer_reschedule(&timer_1sec, ONE_SECOND);
	/* skip sampling when the siren is on */
	if (!timer_is_pending(&siren_timer)
	    && humidity_sampling_update++ >= HUMIDITY_SAMPLING)
		schedule_task(humidity_sampling_task_cb, NULL);

	/* toggle the LED */
	PORTD ^= 1 << PD4;

#ifdef CONFIG_POWER_MANAGEMENT
	/* do not sleep if the siren is on */
	if (timer_is_pending(&siren_timer))
		power_management_pwr_down_reset();
#endif
	report_hum_value();
	if (fan_sec_cnt) {
		if (fan_sec_cnt == 1)
			set_fan_off();
		fan_sec_cnt--;
	}
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
	if (module_cfg.state != MODULE_STATE_ARMED && !force)
		return;

	PORTB |= 1 << PB0;
	timer_del(&siren_timer);
	timer_add(&siren_timer, SIREN_ON_DURATION, siren_tim_cb, NULL);
	if (module_cfg.state == MODULE_STATE_ARMED || force) {
		module_add_op(CMD_NOTIF_ALARM_ON, 1);
		swen_l3_event_set_mask(&mod1_assoc, EV_READ | EV_WRITE);
	}
}

#ifndef CONFIG_AVR_SIMU
static void pir_on_action(void *arg)
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
#endif

static uint8_t get_hum_tendency(void)
{
	int val = global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
		- global_humidity_array[0];

	if (abs(val) < DEFAULT_HUMIDITY_THRESHOLD)
		return HUMIDITY_TENDENCY_STABLE;
	if (val < 0)
		return HUMIDITY_TENDENCY_FALLING;
	return HUMIDITY_TENDENCY_RISING;
}

static void get_humidity(humidity_info_t *info)
{
	uint8_t val = get_humidity_cur_value();
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
	if (!module_cfg.fan_enabled || global_humidity_array[0] == 0)
		return;
	if (info.tendency == HUMIDITY_TENDENCY_RISING ||
	    global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
	    >= MAX_HUMIDITY_VALUE) {
		set_fan_on();
	} else if (info.tendency == HUMIDITY_TENDENCY_STABLE)
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
	status->fan_on = !!(PORTD & 1 << PD3);
	status->siren_on = !!(PORTB & 1 << PB0);
	status->cfg = module_cfg;
	status->rf_up = swen_l3_get_state(&mod1_assoc) == S_STATE_CONNECTED;
}

static void update_storage(void)
{
	eeprom_write_block(&module_cfg , &persistent_data, sizeof(module_cfg_t));
}

static void reload_cfg_from_storage(void)
{
	eeprom_read_block(&module_cfg, &persistent_data, sizeof(module_cfg_t));
	if (module_cfg.humidity_threshold == 0xFF ||
	    module_cfg.humidity_threshold == 0)
		module_cfg.humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
	if (module_cfg.state == 0)
		module_cfg.state = MODULE_STATE_DISARMED;
}

static void handle_rx_commands(uint8_t cmd, uint16_t value);

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	uint8_t cmd;

	DEBUG_LOG("received kerui cmd %d\n", nb);
	switch (nb) {
	case 0:
		cmd = CMD_ARM;
		break;
	case 1:
		cmd = CMD_DISARM;
		break;
	case 2:
		cmd = CMD_RUN_FAN;
		break;
	case 3:
		cmd = CMD_STOP_FAN;
		break;
	default:
		return;
	}
	handle_rx_commands(cmd, 0);
}
#endif
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

static void module_arm(uint8_t on)
{
	if (on)
		module_cfg.state = MODULE_STATE_ARMED;
	else {
		module_cfg.state = MODULE_STATE_DISARMED;
		set_siren_off();
	}
	update_storage();
	timer_del(&siren_timer);
	timer_add(&siren_timer, 10000, arm_cb, NULL);
}

static int handle_tx_commands(uint8_t cmd)
{
	module_status_t status;
	void *data = NULL;
	int len = 0;
	uint8_t hum_val;

	switch (cmd) {
	case CMD_STATUS:
		get_status(&status);
		data = &status;
		len = sizeof(module_status_t);
		break;
	case CMD_REPORT_HUM_VAL:
		hum_val = get_humidity_cur_value();
		data = &hum_val;
		len = sizeof(hum_val);
		break;
	case CMD_NOTIF_ALARM_ON:
		break;
	default:
		return 0;
	}
	return send_rf_msg(&mod1_assoc, cmd, data, len);
}

static void handle_rx_commands(uint8_t cmd, uint16_t value)
{
	DEBUG_LOG("mod1: got cmd:0x%X\n", cmd);
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
	case CMD_RUN_FAN:
		set_fan_on();
		return;
	case CMD_STOP_FAN:
		set_fan_off();
		return;
	case CMD_DISABLE_FAN:
		module_cfg.fan_enabled = 0;
		update_storage();
		return;
	case CMD_ENABLE_FAN:
		module_cfg.fan_enabled = 1;
		update_storage();
		return;
	case CMD_SIREN_ON:
		set_siren_on(1);
		return;
	case CMD_SIREN_OFF:
		set_siren_off();
		return;
	case CMD_SET_HUM_TH:
		if (value && value <= 100) {
			module_cfg.humidity_threshold = value;
			update_storage();
		}
		return;
	case CMD_GET_STATUS:
		module_add_op(CMD_STATUS, 0);
		swen_l3_event_set_mask(&mod1_assoc, EV_READ | EV_WRITE);
		return;
	case CMD_GET_REPORT_HUM_VAL:
		module_cfg.humidity_report_interval = value;
		update_storage();
		report_hval_interval = value;
		return;
	}
}

static void module1_parse_commands(buf_t *buf)
{
	uint8_t cmd;
	uint16_t value;

	if (buf_len(buf) == 0)
		return;

	cmd = buf_data(buf)[0];
	if (buf_len(buf) >= 3)
		value = *(uint16_t *)(buf_data(buf) + 1);
	else if (buf_len(buf) == 2)
		value = *(uint8_t *)(buf_data(buf) + 1);
	else
		value = 0;

	handle_rx_commands(cmd, value);
}

#ifdef DEBUG
static void module_print_status(void)
{
	int humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];
	uint8_t hval = get_humidity_cur_value();

	array_copy(humidity_array, global_humidity_array,
		   GLOBAL_HUMIDITY_ARRAY_LENGTH);

	LOG("\nStatus:\n");
	LOG(" State:  %s\n",
	    module_cfg.state == MODULE_STATE_ARMED ? "armed" : "disarmed");
	LOG(" Humidity value:  %ld%%\n"
	    " Global humidity value:  %d%%\n"
	    " Humidity tendency:  %u\n"
	    " Humidity threshold:  %u%%\n",
	    hval > 100 ? 0 : hval,
	    array_get_median(humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH),
	    get_hum_tendency(),
	    module_cfg.humidity_threshold);
	LOG(" Fan: %d\n Fan enabled: %d\n", !!(PORTD & 1 << PD3),
	    module_cfg.fan_enabled);
	LOG(" Siren:  %d\n", !!(PORTB & 1 << PB0));
	LOG(" RF:  %s\n",
	    swen_l3_get_state(&mod1_assoc) == S_STATE_CONNECTED ? "on" : "off");
}

void module1_parse_uart_commands(buf_t *buf)
{
	sbuf_t s;

	if (buf_get_sbuf_upto_and_skip(buf, &s, "rf buf") >= 0) {
		LOG("\nifce pool: %d rx: %d tx:%d\npkt pool: %d\n",
		    ring_len(rf_iface.pkt_pool), ring_len(rf_iface.rx),
		    ring_len(rf_iface.tx), pkt_pool_get_nb_free());
		return;
	}

	if (buf_get_sbuf_upto_and_skip(buf, &s, "get status") >= 0) {
		module_print_status();
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "disarm") >= 0) {
		module_arm(0);
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "arm") >= 0) {
		module_arm(1);
		return;
	}
	LOG("unsupported command\n");
}
#endif

static void rf_connecting_on_event(event_t *ev, uint8_t events);

static void rf_event_cb(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
#ifdef DEBUG
	uint8_t id = addr_to_module_id(assoc->dst);
#endif

#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif

	if (events & EV_READ) {
		pkt_t *pkt = swen_l3_get_pkt(assoc);

		DEBUG_LOG("got pkt of len:%d from mod%d\n", buf_len(&pkt->buf),
			  id);
		module1_parse_commands(&pkt->buf);
		pkt_free(pkt);
	}
	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("mod%d disconnected\n", id);
		swen_l3_event_unregister(assoc);
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
	}
	if (events & EV_WRITE) {
		uint8_t op;

		if (module_get_op(&op) < 0) {
			swen_l3_event_set_mask(assoc, EV_READ);
			return;
		}
		DEBUG_LOG("mod1: sending op:0x%X to mod%d\n", op, id);
		if (handle_tx_commands(op) >= 0)
			module_skip_op();
	}
}

static void rf_connecting_on_event(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
#ifdef DEBUG
	uint8_t id = addr_to_module_id(assoc->dst);
#endif
	if (events & EV_ERROR) {
		DEBUG_LOG("failed to connect to mod%d\n", id);
		swen_l3_event_unregister(assoc);
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		return;
	}
	if (events & EV_WRITE) {
		DEBUG_LOG("connected to mod%d\n", id);
		swen_l3_event_register(assoc, EV_READ, rf_event_cb);
	}
}

void module1_init(void)
{
#ifdef CONFIG_RF_CHECKS
	if (rf_checks(&rf_iface) < 0)
		__abort();
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
	reload_cfg_from_storage();
	module_init_op_queues();
	timer_add(&timer_1sec, ONE_SECOND, timer_1sec_cb, NULL);
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_power_down_init(INACTIVITY_TIMEOUT, pwr_mgr_on_sleep,
					 NULL);
#endif
	module_init_iface(&rf_iface, &rf_addr);
	swen_l3_assoc_init(&mod1_assoc, rf_enc_defkey);
	swen_l3_assoc_bind(&mod1_assoc, RF_MOD0_HW_ADDR, &rf_iface);
	swen_l3_event_register(&mod1_assoc, EV_WRITE, rf_connecting_on_event);
	if (module_cfg.state == MODULE_STATE_DISABLED)
		return;
#ifndef RF_DEBUG
	if (swen_l3_associate(&mod1_assoc) < 0)
		__abort();
#endif
}
