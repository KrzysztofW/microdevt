#include <stdio.h>
#include <scheduler.h>
#include <net/event.h>
#include <net/swen-l3.h>
#include <power-management.h>
#include <interrupts.h>
#include <drivers/sensors.h>
#include <eeprom.h>
#include "../module-common.h"
#include "gpio.h"

#define THIS_MODULE_FEATURES MODULE_FEATURE_HUMIDITY |	  \
	MODULE_FEATURE_TEMPERATURE | MODULE_FEATURE_FAN | \
	MODULE_FEATURE_SIREN | MODULE_FEATURE_RF

#define FEATURE_TEMPERATURE
#define FEATURE_HUMIDITY
#define FEATURE_SIREN
#define FEATURE_PWR

#include "../module-common.inc.c"

static uint8_t rf_addr = RF_GENERIC_MOD_HW_ADDR;
static iface_t rf_iface;
#ifdef RF_DEBUG
iface_t *rf_debug_iface2 = &rf_iface;
#endif

static module_cfg_t module_cfg;
static tim_t timer_1sec = TIMER_INIT(timer_1sec);
static swen_l3_assoc_t mod1_assoc;

#define TX_QUEUE_SIZE 4
/* TX_QUEUE_SIZE must be equal to the number of urgent ops + 1:
 * NOTIF_MAIN_PWR_UP, NOTIF_MAIN_PWR_DOWN and NOTIF_ALARM_ON
 * (FEATURES is only sent once at boot).
 */
STATIC_RING_DECL(tx_queue, TX_QUEUE_SIZE);
STATIC_RING_DECL(urgent_tx_queue, TX_QUEUE_SIZE);

static module_cfg_t EEMEM persistent_cfg;

static inline void cfg_update(void)
{
	if (eeprom_update_and_check(&persistent_cfg, &module_cfg,
				    sizeof(module_cfg_t)))
		return;
	module_add_op(tx_queue, CMD_STORAGE_ERROR, &mod1_assoc.event);
}

static void cfg_load(void)
{
#ifndef CONFIG_AVR_SIMU
	if (!module_check_magic()) {
#endif
		module_set_default_cfg(&module_cfg);
		if (!module_update_magic())
			module_add_op(tx_queue, CMD_STORAGE_ERROR,
				      &mod1_assoc.event);
		else
			cfg_update();
		return;
#ifndef CONFIG_AVR_SIMU
	}
#endif
	eeprom_load(&module_cfg, &persistent_cfg, sizeof(module_cfg_t));
}

static void power_action(uint8_t state)
{
	module_add_op(urgent_tx_queue, state, &mod1_assoc.event);
}

static void sensor_action(void)
{
	module_add_op(tx_queue, CMD_SENSOR_REPORT, &mod1_assoc.event);
}

static void timer_1sec_cb(void *arg)
{
	timer_reschedule(&timer_1sec, ONE_SECOND);
	cron_func(&module_cfg, power_action, sensor_action);
}

#ifndef CONFIG_AVR_SIMU
ISR(PCINT0_vect)
{
	power_interrupt(&module_cfg);
}

static void pir_interrupt_cb(void)
{
	module_add_op(urgent_tx_queue, CMD_NOTIF_ALARM_ON, &mod1_assoc.event);
}

ISR(PCINT2_vect)
{
	pir_interrupt(&module_cfg, pir_interrupt_cb);
}
#endif

static void handle_rx_commands(uint8_t cmd, uint16_t val1, uint16_t val2);

#if defined(CONFIG_RF_RECEIVER) && defined(CONFIG_RF_GENERIC_COMMANDS)
static void generic_cmds_cb(uint16_t cmd, uint8_t status)
{
	if (status == GENERIC_CMD_STATUS_RCV) {
		DEBUG_LOG("mod1: received generic cmd %u\n", cmd);
		handle_rx_commands(cmd, 0, 0);
		return;
	}
	send_rf_msg(&mod1_assoc, CMD_RECORD_GENERIC_COMMAND_ANSWER, &status,
		    sizeof(status));
}
#endif

static int handle_tx_commands(uint8_t cmd)
{
	module_status_t status;
	sensor_value_t sensor_value;
	void *data = NULL;
	int len = 0;
	uint8_t features;
#ifdef CONFIG_RF_GENERIC_COMMANDS
	buf_t buf;
#endif

#ifdef CONFIG_AVR_SIMU
	(void)pir_interrupt;
	(void)power_interrupt;
#endif
	switch (cmd) {
	case CMD_STATUS:
		get_module_status(&status, &module_cfg, &mod1_assoc);
		data = &status;
		len = sizeof(module_status_t);
		break;
	case CMD_SENSOR_VALUES:
		get_sensor_values(&sensor_value, &module_cfg);
		data = &sensor_value;
		len = sizeof(sensor_value_t);
		break;
	case CMD_SENSOR_REPORT:
		send_sensor_report(&rf_iface, module_cfg.sensor.notif_addr);
		return 0;
	case CMD_NOTIF_ALARM_ON:
	case CMD_NOTIF_MAIN_PWR_DOWN:
	case CMD_NOTIF_MAIN_PWR_UP:
	case CMD_STORAGE_ERROR:
		break;
	case CMD_FEATURES:
		features = THIS_MODULE_FEATURES;
		data = &features;
		len = sizeof(features);
		break;
#ifdef CONFIG_RF_GENERIC_COMMANDS
	case CMD_GENERIC_COMMANDS_LIST:
		buf = BUF(sizeof(recordable_cmds) * 2 + 2);
		__buf_addc(&buf, CMD_RECORD_GENERIC_COMMAND_ANSWER);
		swen_generic_cmds_get_list(&buf);
		return swen_l3_send_buf(&mod1_assoc, &buf);
#endif
	default:
		return 0;
	}
	return send_rf_msg(&mod1_assoc, cmd, data, len);
}

static void handle_rx_commands(uint8_t cmd, uint16_t val1, uint16_t val2)
{
	DEBUG_LOG("mod1: got cmd:0x%X\n", cmd);
	switch (cmd) {
	case CMD_GET_FEATURES:
		module_add_op(urgent_tx_queue, CMD_FEATURES, &mod1_assoc.event);
		return;
	case CMD_ARM:
		module_arm(&module_cfg, 1);
		break;
	case CMD_DISARM:
		module_arm(&module_cfg, 0);
		break;
	case CMD_STATUS:
	case CMD_NOTIF_ALARM_ON:
		/* unsupported */
		return;
	case CMD_RUN_FAN:
		set_fan_on();
		return;
	case CMD_STOP_FAN:
		gpio_fan_off();
		return;
	case CMD_DISABLE_FAN:
		module_cfg.fan_enabled = 0;
		break;
	case CMD_ENABLE_FAN:
		module_cfg.fan_enabled = 1;
		break;
	case CMD_SIREN_ON:
		set_siren_on(&module_cfg, 1);
		return;
	case CMD_SIREN_OFF:
		gpio_siren_off();
		return;
	case CMD_SET_HUM_TH:
		if (val1 && val1 <= 100) {
			module_cfg.sensor.humidity_threshold = val1;
			break;
		}
		return;
	case CMD_SET_SIREN_DURATION:
		if (val1) {
			module_cfg.siren_duration = val1;
			break;
		}
		return;
	case CMD_SET_SIREN_TIMEOUT:
		module_cfg.siren_timeout = val1;
		break;
	case CMD_GET_STATUS:
		module_add_op(tx_queue, CMD_STATUS, &mod1_assoc.event);
		return;
	case CMD_GET_SENSOR_VALUES:
		module_add_op(tx_queue, CMD_SENSOR_VALUES, &mod1_assoc.event);
		return;
	case CMD_GET_SENSOR_REPORT:
		if (val2 == rf_addr)
			return;
		module_cfg.sensor.report_interval = val1;
		module_cfg.sensor.notif_addr = val2;
		break;
#ifdef CONFIG_POWER_MANAGEMENT
	case CMD_DISABLE_PWR_DOWN:
		power_management_power_down_disable();
		break;
	case CMD_ENABLE_PWR_DOWN:
		power_management_power_down_enable();
		break;
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS
	case CMD_RECORD_GENERIC_COMMAND:
		if (!cmd_is_recordable(val1))
			return;
		DEBUG_LOG("recording (val:%u)\n", val1);
		swen_generic_cmds_start_recording(val1);
		return;
	case CMD_GENERIC_COMMANDS_LIST:
		module_add_op(tx_queue, CMD_GENERIC_COMMANDS_LIST,
			      &mod1_assoc.event);
		return;
	case CMD_GENERIC_COMMANDS_DELETE:
		swen_generic_cmds_delete_recorded_cmd(val1);
		return;
#endif
	}
	cfg_update();
}

static void rf_connecting_on_event(event_t *ev, uint8_t events);

static void module_parse_commands(buf_t *buf)
{
	uint8_t cmd;
	uint8_t v8 = 0;
	uint16_t v16 = 0;
	uint8_t v8_2 = 0;
	uint16_t v16_2 = 0;

	if (buf_getc(buf, &cmd) < 0)
		return;

	if (buf_get_u16(buf, &v16) < 0)
		if (buf_getc(buf, &v8) >= 0)
			v16 = v8;
	if (buf_get_u16(buf, &v16_2) < 0)
		if (buf_getc(buf, &v8_2) >= 0)
			v16_2 = v8_2;

	handle_rx_commands(cmd, v16, v16_2);
}

#if defined(DEBUG) && defined(CONFIG_AVR_SIMU)
static void module_print_status(void)
{
	int humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];

	array_copy(humidity_array, global_humidity_array,
		   GLOBAL_HUMIDITY_ARRAY_LENGTH);

	LOG("\nStatus:\n");
	LOG(" State:  %s\n",
	    module_cfg.state == MODULE_STATE_ARMED ? "armed" : "disarmed");
	LOG(" Humidity value:  %u%%\n"
	    " Global humidity value:  %d%%\n"
	    " Humidity tendency:  %u\n"
	    " Humidity threshold:  %u%%\n",
	    sensor_report.humidity,
	    array_get_median(humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH),
	    get_hum_tendency(module_cfg.sensor.humidity_threshold),
	    module_cfg.sensor.humidity_threshold);
	LOG(" Temperature:  %d\n", sensor_report.temperature);
	LOG(" Fan: %d\n Fan enabled: %d\n", gpio_is_fan_on(),
	    module_cfg.fan_enabled);
	LOG(" Siren:  %d\n", gpio_is_siren_on()),
	LOG(" Siren duration:  %u secs\n", module_cfg.siren_duration);
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
		module_arm(&module_cfg, 0);
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "arm") >= 0) {
		module_arm(&module_cfg, 1);
		return;
	}
	LOG("unsupported command\n");
}
#endif

static void rf_event_cb(event_t *ev, uint8_t events)
{
#ifdef DEBUG
	uint8_t id = addr_to_module_id(mod1_assoc.dst);
#endif

#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
	if (events & EV_READ) {
		pkt_t *pkt;

		while ((pkt = swen_l3_get_pkt(&mod1_assoc)) != NULL) {
			DEBUG_LOG("got pkt of len:%d from mod%d\n",
				  pkt->buf.len, id);
			module_parse_commands(&pkt->buf);
			pkt_free(pkt);
		}
	}
	if (events & EV_ERROR) {
		swen_l3_associate(&mod1_assoc);
		goto error;
	}
	if (events & EV_HUNGUP)
		goto error;

	if (events & EV_WRITE) {
		uint8_t op;

		if (module_get_op2(&op, urgent_tx_queue, tx_queue) < 0) {
			swen_l3_event_set_mask(&mod1_assoc, EV_READ);
			return;
		}
		DEBUG_LOG("mod1: sending op:0x%X to mod%d\n", op, id);
		if (handle_tx_commands(op) >= 0) {
			module_skip_op2(urgent_tx_queue, tx_queue);
			return;
		}
		if (swen_l3_get_state(&mod1_assoc) != S_STATE_CONNECTED) {
			swen_l3_associate(&mod1_assoc);
			goto error;
		}
	}
	return;
 error:
	DEBUG_LOG("mod%d disconnected\n", id);
	swen_l3_event_unregister(&mod1_assoc);
	swen_l3_event_register(&mod1_assoc, EV_WRITE, rf_connecting_on_event);
}

static void rf_connecting_on_event(event_t *ev, uint8_t events)
{
#ifdef DEBUG
	uint8_t id = addr_to_module_id(mod1_assoc.dst);
#endif
	uint8_t op;

	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("failed to connect to mod%d\n", id);
		swen_l3_event_unregister(&mod1_assoc);
		swen_l3_associate(&mod1_assoc);
		swen_l3_event_register(&mod1_assoc, EV_WRITE,
				       rf_connecting_on_event);
		return;
	}
	if (events & EV_WRITE) {
		uint8_t flags = EV_READ;

		DEBUG_LOG("connected to mod%d\n", id);
		if (module_get_op2(&op, urgent_tx_queue, tx_queue) >= 0)
			flags |= EV_WRITE;
		swen_l3_event_register(&mod1_assoc, flags, rf_event_cb);
	}
}

void module1_init(void)
{
	if (gpio_is_main_pwr_on())
		pwr_state = CMD_NOTIF_MAIN_PWR_UP;

#ifdef RF_DEBUG
	module_init_debug_iface(&rf_iface, &rf_addr);
#else
	module_init_iface(&rf_iface, &rf_addr);
#endif
#ifdef CONFIG_RF_CHECKS
	if (rf_checks(&rf_iface) < 0)
		__abort();
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(generic_cmds_cb);
#endif
	cfg_load();
	timer_add(&timer_1sec, ONE_SECOND, timer_1sec_cb, NULL);

#ifdef CONFIG_POWER_MANAGEMENT
	power_management_power_down_init(INACTIVITY_TIMEOUT, pwr_mgr_on_sleep,
					 &module_cfg);
#endif
	swen_l3_assoc_init(&mod1_assoc, rf_enc_defkey);
	swen_l3_assoc_bind(&mod1_assoc, RF_MASTER_MOD_HW_ADDR, &rf_iface);
	swen_l3_event_register(&mod1_assoc, EV_WRITE, rf_connecting_on_event);

	if (module_cfg.state == MODULE_STATE_DISABLED)
		return;
	if (swen_l3_associate(&mod1_assoc) < 0)
		__abort();
}
