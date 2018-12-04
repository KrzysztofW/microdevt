#include <stdio.h>
#include <net/event.h>
#include <net/swen.h>
#include <net/swen-rc.h>
#include <net/swen-l3.h>
#include <power-management.h>
#include <interrupts.h>
#include <drivers/sensors.h>
#include <eeprom.h>
#include "module-common.h"
#include "gpio.h"

#define THIS_MODULE_FEATURES MODULE_FEATURE_TEMPERATURE |		\
	MODULE_FEATURE_SIREN | MODULE_FEATURE_LAN | MODULE_FEATURE_RF

#define FEATURE_TEMPERATURE
#define FEATURE_SIREN
#define FEATURE_PWR

#include "module-common.inc.c"

static uint8_t rf_addr = RF_MASTER_MOD_HW_ADDR;
static iface_t rf_iface;
#ifdef RF_DEBUG
iface_t *rf_debug_iface1 = &rf_iface;
#endif

#ifdef CONFIG_NETWORKING
extern iface_t eth_iface;
#endif

static tim_t timer_1sec = TIMER_INIT(timer_1sec);

static sbuf_t s_disarm = SBUF_INITS("disarm");
static sbuf_t s_arm = SBUF_INITS("arm");
static sbuf_t s_get_status = SBUF_INITS("get status");
static sbuf_t s_get_sensor_values = SBUF_INITS("get sensor values");
static sbuf_t s_fan_on = SBUF_INITS("fan on");
static sbuf_t s_fan_off = SBUF_INITS("fan off");
static sbuf_t s_fan_disable = SBUF_INITS("fan disable");
static sbuf_t s_fan_enable = SBUF_INITS("fan enable");
static sbuf_t s_siren_on = SBUF_INITS("siren on");
static sbuf_t s_siren_off = SBUF_INITS("siren off");
static sbuf_t s_siren_duration = SBUF_INITS("siren duration");
static sbuf_t s_siren_timeout = SBUF_INITS("siren timeout");
static sbuf_t s_humidity_th = SBUF_INITS("humidity th");
static sbuf_t s_sensor_report = SBUF_INITS("sensor report");
static sbuf_t s_disconnect = SBUF_INITS("disconnect");
static sbuf_t s_connect = SBUF_INITS("connect");
static sbuf_t s_disable_pwr_down = SBUF_INITS("disable pwr down");
static sbuf_t s_enable_pwr_down = SBUF_INITS("enable pwr down");
static sbuf_t s_disable = SBUF_INITS("disable");
#ifdef CONFIG_RF_GENERIC_COMMANDS
static sbuf_t s_record_cmd = SBUF_INITS("record cmd");
static sbuf_t s_delete_cmd = SBUF_INITS("delete cmd");
static sbuf_t s_list_cmds = SBUF_INITS("list cmd");
#endif

#define MAX_ARGS 8
typedef enum arg_type {
	ARG_TYPE_NONE,
	ARG_TYPE_INT8,
	ARG_TYPE_INT16,
	ARG_TYPE_STRING,
} arg_type_t;

typedef struct cmd {
	const sbuf_t *s;
	uint8_t args[MAX_ARGS];
	uint8_t cmd;
} cmd_t;

static cmd_t cmds[] = {
	{ .s = &s_disarm, .args = { ARG_TYPE_NONE }, .cmd = CMD_DISARM },
	{ .s = &s_arm, .args = { ARG_TYPE_NONE }, .cmd = CMD_ARM },
	{ .s = &s_get_status, .args = { ARG_TYPE_NONE }, .cmd = CMD_GET_STATUS },
	{ .s = &s_get_sensor_values, .args = { ARG_TYPE_NONE },
	  .cmd = CMD_GET_SENSOR_VALUES },
	{ .s = &s_fan_on, .args = { ARG_TYPE_NONE }, .cmd = CMD_RUN_FAN },
	{ .s = &s_fan_off, .args = { ARG_TYPE_NONE }, .cmd = CMD_STOP_FAN },
	{ .s = &s_fan_disable, .args = { ARG_TYPE_NONE },
	  .cmd = CMD_DISABLE_FAN },
	{ .s = &s_fan_enable, .args = { ARG_TYPE_NONE },
	  .cmd = CMD_ENABLE_FAN },
	{ .s = &s_siren_on, .args = { ARG_TYPE_NONE }, .cmd = CMD_SIREN_ON },
	{ .s = &s_siren_off, .args = { ARG_TYPE_NONE }, .cmd = CMD_SIREN_OFF },
	{ .s = &s_humidity_th, .args = { ARG_TYPE_INT8, ARG_TYPE_NONE },
	  .cmd = CMD_SET_HUM_TH },
	{ .s = &s_sensor_report, .args = { ARG_TYPE_INT16, ARG_TYPE_INT8,
					   ARG_TYPE_NONE },
	  .cmd = CMD_GET_SENSOR_REPORT },
	{ .s = &s_siren_duration, .args = { ARG_TYPE_INT16, ARG_TYPE_NONE },
	  .cmd = CMD_SET_SIREN_DURATION },
	{ .s = &s_siren_timeout, .args = { ARG_TYPE_INT8, ARG_TYPE_NONE },
	  .cmd = CMD_SET_SIREN_TIMEOUT },
	{ .s = &s_disconnect, .args = { ARG_TYPE_NONE }, .cmd = CMD_DISCONNECT },
	{ .s = &s_disable_pwr_down, .args = { ARG_TYPE_NONE },
	  .cmd = CMD_DISABLE_PWR_DOWN },
	{ .s = &s_enable_pwr_down, .args = { ARG_TYPE_NONE },
	  .cmd = CMD_ENABLE_PWR_DOWN },
	{ .s = &s_connect, .args = { ARG_TYPE_NONE }, .cmd = CMD_CONNECT },
	{ .s = &s_disable, .args = { ARG_TYPE_NONE }, .cmd = CMD_DISABLE },
#ifdef CONFIG_RF_GENERIC_COMMANDS
	{ .s = &s_record_cmd, .args = { ARG_TYPE_INT8, ARG_TYPE_NONE },
	  .cmd = CMD_RECORD_GENERIC_COMMAND },
	{ .s = &s_delete_cmd, .args = { ARG_TYPE_INT8, ARG_TYPE_NONE },
	  .cmd = CMD_GENERIC_COMMANDS_DELETE, },
	{ .s = &s_list_cmds, .args = { ARG_TYPE_NONE },
	  .cmd = CMD_GENERIC_COMMANDS_LIST, },
#endif
};

#define NB_MODULES 2
static module_t modules[NB_MODULES];
static module_cfg_t module_cfg;
static module_cfg_t EEMEM module_cfgs[NB_MODULES];

static void cfg_load(module_cfg_t *cfg, uint8_t id)
{
	assert(id < NB_MODULES);
	eeprom_load(cfg, &module_cfgs[id], sizeof(module_cfg_t));
}

static void cfg_update(module_cfg_t *cfg, uint8_t id)
{
	assert(id < NB_MODULES);
	eeprom_update(&module_cfgs[id], cfg, sizeof(module_cfg_t));
}

static inline void cfg_update_master(void)
{
	cfg_update(&module_cfg, 0);
}

static void power_action(uint8_t state)
{
	LOG("mod0 power %s\n", state == CMD_NOTIF_MAIN_PWR_DOWN ? "down":"up");
}

static void sensor_action(void)
{
	send_sensor_report(&rf_iface, module_cfg.sensor.notif_addr);
}

static void timer_1sec_cb(void *arg)
{
	timer_reschedule(&timer_1sec, ONE_SECOND);
	cron_func(&module_cfg, power_action, sensor_action);
}

ISR(PCINT0_vect)
{
	power_interrupt(&module_cfg);
}

static void pir_interrupt_cb(void) {}

ISR(PCINT1_vect)
{
	pir_interrupt(&module_cfg, pir_interrupt_cb);
}

static const char *state_to_string(uint8_t state)
{
	switch (state) {
	case MODULE_STATE_DISABLED:
		return "DISABLED";
	case MODULE_STATE_DISARMED:
		return "DISARMED";
	case MODULE_STATE_ARMED:
		return "ARMED";
	default:
		return NULL;
	}
}

static const char *yes_no(uint8_t val)
{
	if (val)
		return "yes";
	return "no";
}

static const char *on_off(uint8_t val)
{
	if (val)
		return "on";
	return "off";
}

/* We have to loop over all modules to find out how many of them
 * are connected. Maintaining a static variable for each connect/disconnect
 * is not reliable as if a module has a connected state and reconnects,
 * we will not see it going through the disconnected state.
 */
static uint8_t get_connected_rf_devices(void)
{
	uint8_t i, connected = 0;

	for (i = 1; i < NB_MODULES; i++)
		if (swen_l3_get_state(&modules[i].assoc) == S_STATE_CONNECTED)
			connected++;
	return connected;
}

static void print_sensor_values(const module_cfg_t *cfg, uint8_t id,
				const sensor_value_t *value)
{
	uint8_t features = cfg->features;

	LOG("\nModule %d:\n", id);
	if (features & MODULE_FEATURE_HUMIDITY) {
		LOG(" Humidity value: %u%%\n"
		    " Global humidity value: %u%%\n"
		    " Humidity tendency: %s\n"
		    " Humidity threshold: %u%%\n",
		    value->humidity.val,
		    value->humidity.global_val,
		    humidity_tendency_to_str(value->humidity.tendency),
		    cfg->sensor.humidity_threshold);
	}
	if (features & MODULE_FEATURE_TEMPERATURE)
		LOG(" Temperatue: %d\n", value->temperature);
}

static void print_module_status(const module_cfg_t *cfg, uint8_t id,
				const module_status_t *status)
{
	uint8_t features = cfg->features;

	LOG("\nModule %d:\n", id);
	LOG(" State:  %s\n", state_to_string(status->state));
	if (features & (MODULE_FEATURE_HUMIDITY | MODULE_FEATURE_TEMPERATURE)) {
		LOG(" Sensor report interval: %u secs\n"
		    " Sensor report module: %u\n",
		    status->sensor.report_interval,
		    status->sensor.notif_addr ?
		    addr_to_module_id(status->sensor.notif_addr): 0);
	}
	if (features & MODULE_FEATURE_FAN)
		LOG(" Fan:  %s\n Fan enabled: %s\n",
		    on_off(status->flags & STATUS_FLAGS_FAN_ON),
		    yes_no(status->flags & STATUS_FLAGS_FAN_ENABLED));
	if (features & MODULE_FEATURE_SIREN) {
		LOG(" Siren:  %s\n",
		    on_off(status->flags & STATUS_FLAGS_SIREN_ON));
		LOG(" Siren duration: %u secs\n", status->siren.duration);
		LOG(" Siren timeout:  %u secs\n", status->siren.timeout);
	}
	if (features & MODULE_FEATURE_LAN)
		LOG(" LAN:  %s\n",
		    on_off(status->flags & STATUS_FLAGS_CONN_LAN_UP));
	if (features & MODULE_FEATURE_RF) {
		if (id == 0)
			LOG(" RF connected devices: %u\n",
			    get_connected_rf_devices());
		else
			LOG(" RF: %s\n",
			    on_off(status->flags & STATUS_FLAGS_CONN_RF_UP));
	}
	LOG(" Main power: %s\n",
	    on_off(status->flags & STATUS_FLAGS_MAIN_PWR_ON));
}

static void rf_connecting_on_event(event_t *ev, uint8_t events);

#ifdef CONFIG_RF_GENERIC_COMMANDS
static void show_recordable_cmds(void)
{
	uint8_t i;

	LOG("List of recordable commands:\n");
	for (i = 0; i < sizeof(cmds) / sizeof(cmd_t); i++) {
		if (cmd_is_recordable(cmds[i].cmd))
			LOG("%2u %s\n", cmds[i].cmd, cmds[i].s->data);
	}
}
#endif

static void handle_rx_commands(uint8_t id, uint8_t cmd, buf_t *args)
{
	module_cfg_t c;
	module_cfg_t *cfg;
	swen_l3_assoc_t *assoc;
	uint8_t mod_id;

	if (id == 0)
		cfg = &module_cfg;
	else
		cfg = &c;
	cfg_load(cfg, id);

	if (cmd != CMD_CONNECT && cfg->state == MODULE_STATE_UNINITIALIZED) {
		LOG("module %u not initilized\n", id);
		return;
	}

	if ((!(cfg->features & MODULE_FEATURE_FAN)
	     && (cmd == CMD_RUN_FAN || cmd == CMD_STOP_FAN ||
		 cmd == CMD_DISABLE_FAN || cmd == CMD_ENABLE_FAN)) ||
	    (!(cfg->features & MODULE_FEATURE_SIREN)
	     && (cmd == CMD_SIREN_ON || cmd == CMD_SIREN_OFF ||
		 cmd == CMD_SET_SIREN_DURATION ||
		 cmd == CMD_SET_SIREN_TIMEOUT || cmd == CMD_ARM ||
		 cmd == CMD_DISARM)) ||
	    (!(cfg->features & MODULE_FEATURE_HUMIDITY)
	     && cmd == CMD_SET_HUM_TH)) {
		LOG("unsupported feature\n");
		return;
	}

	switch (cmd) {
	case CMD_ARM:
		if (id == 0)
			module_arm(&module_cfg, 1);
		else
			cfg->state = MODULE_STATE_ARMED;
		break;
	case CMD_DISARM:
		cfg->state = MODULE_STATE_DISARMED;
		if (id == 0)
			module_arm(&module_cfg, 0);
		break;
	case CMD_SET_HUM_TH:
		__buf_getc(args, &cfg->sensor.humidity_threshold);
		break;
	case CMD_GET_SENSOR_REPORT:
		__buf_get_u16(args, &cfg->sensor.report_interval);
		__buf_getc(args, &mod_id);
		cfg->sensor.notif_addr = module_id_to_addr(mod_id);
		break;
	case CMD_SET_SIREN_DURATION:
		__buf_get_u16(args, &cfg->siren_duration);
		break;
	case CMD_SET_SIREN_TIMEOUT:
		__buf_getc(args, &cfg->siren_timeout);
		break;
	case CMD_ENABLE_FAN:
		cfg->fan_enabled = 1;
		break;
	case CMD_DISABLE_FAN:
		cfg->fan_enabled = 0;
		break;
	case CMD_GET_STATUS:
		if (id == 0) {
			module_status_t master_status;

			get_module_status(&master_status, &module_cfg, NULL);
			print_module_status(cfg, 0, &master_status);
			return;
		}
		break;
	case CMD_GET_SENSOR_VALUES:
		if (id == 0) {
			sensor_value_t master_sensor_value;

			get_sensor_values(&master_sensor_value);
			print_sensor_values(cfg, 0, &master_sensor_value);
			return;
		}
		break;

	case CMD_SIREN_ON:
		if (id == 0)
			set_siren_on(&module_cfg, 1);
		break;
	case CMD_SIREN_OFF:
		if (id == 0)
			gpio_siren_off();
		break;
	case CMD_DISABLE:
		if (id == 0)
			return;
		cfg->state = MODULE_STATE_DISABLED;
		cmd = CMD_DISCONNECT;
		break;
#ifdef CONFIG_RF_GENERIC_COMMANDS
	case CMD_GENERIC_COMMANDS_LIST:
		show_recordable_cmds();
		break;
#endif
	default:
		break;
	}

	cfg_update(cfg, id);
	if (id == 0)
		return;

	if (!(cfg->features & MODULE_FEATURE_RF)) {
		if (cfg->features == 0 && cmd == CMD_CONNECT) {
			LOG("enabling module\n");
			cfg->state = MODULE_STATE_DISARMED;
			cfg_update(cfg, id);
		} else {
			LOG("module does not support RF commands\n");
			return;
		}
	}
	assoc = &modules[id].assoc;

	if (cmd == CMD_CONNECT) {
		if (swen_l3_get_state(assoc) != S_STATE_CLOSED) {
			LOG("disconnect first\n");
			return;
		}
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		if (!swen_l3_is_assoc_bound(assoc))
			swen_l3_assoc_bind(assoc, module_id_to_addr(id),
					   &rf_iface);
		swen_l3_associate(&modules[id].assoc);
		return;
	}
	if (swen_l3_get_state(assoc) != S_STATE_CONNECTED) {
		LOG("module not connected\n");
		return;
	}
#ifdef CONFIG_RF_GENERIC_COMMANDS
	if (cmd == CMD_RECORD_GENERIC_COMMAND ||
	    cmd == CMD_GENERIC_COMMANDS_DELETE) {
		uint8_t rcmd;

		if (buf_getc(args, &rcmd) < 0) {
			LOG("invalid argument\n");
			return;
		}
		if (cmd == CMD_RECORD_GENERIC_COMMAND &&
		    !cmd_is_recordable(rcmd)) {
			LOG("command %u is not recordable\n", rcmd);
			return;
		}
		send_rf_msg(assoc, cmd, &rcmd, sizeof(rcmd));
		return;
	}
#endif
	if (cmd == CMD_DISCONNECT) {
		swen_l3_disassociate(assoc);
		return;
	}

	module_add_op(&modules[id].op_queue, cmd, &assoc->event);
}

static int fill_args(uint8_t arg, buf_t *args, buf_t *buf)
{
	buf_t b;
	sbuf_t sbuf;
	int16_t v16;
	sbuf_t space = SBUF_INITS(" ");
	sbuf_t end;
	char c_end;

	buf_skip_spaces(buf);
	if (arg == ARG_TYPE_NONE || buf->len <= 1)
		return -1;

	if (arg == ARG_TYPE_STRING) {
		/* take the first string from buf */
		if (buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, &space) < 0)
			sbuf_init(&sbuf, buf->data, buf->len);

		return buf_addsbuf(args, &sbuf);
	}

	b = BUF(8);

	c_end = '\0';
	end = SBUF_INIT(&c_end, 1);

	if (buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, &space) < 0
	    && buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, &end) < 0)
		return -1;

	if (sbuf.len == 0 || buf_addsbuf(&b, &sbuf) < 0
	    || buf_addc(&b, c_end) < 0)
		return -1;

	v16 = atoi((char *)b.data);

	switch (arg) {
	case ARG_TYPE_INT8:
		if (v16 > 127 || v16 < -127)
			return -1;
		return buf_addc(args, (int8_t)v16);
	case ARG_TYPE_INT16:
		return buf_add(args, &v16, sizeof(int16_t));
	default:
		return -1;
	}
}

static void print_usage(void)
{
	uint8_t i;

	LOG("");
	for (i = 0; i < sizeof(cmds) / sizeof(cmd_t); i++) {
		cmd_t *cmd = &cmds[i];
		uint8_t *args = cmd->args;

		LOG("mod[id] %s", cmd->s->data);
		while (args[0] != ARG_TYPE_NONE) {
			if (args[0] == ARG_TYPE_STRING)
				LOG(" <string>");
			else
				LOG(" <number>");
			args++;
		}
		LOG("\n");
	}
	LOG("\n");
}

void alarm_parse_uart_commands(buf_t *buf)
{
	buf_t tmp = BUF(10);
	uint8_t id;
	uint8_t i;
	sbuf_t s;
	iface_t *ifce = NULL;

	if (buf_get_sbuf_upto_and_skip(buf, &s, "help") >= 0) {
		print_usage();
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "rf buf") >= 0)
		ifce = &rf_iface;
	else if (buf_get_sbuf_upto_and_skip(buf, &s, "eth buf") >= 0) {
#ifdef CONFIG_NETWORK
		ifce = &eth_iface;
#else
		LOG(" unsupported\n");
#endif
	}
	if (ifce) {
		LOG("\nifce pool: %d rx: %d tx:%d\npkt pool: %d\n",
		    ring_len(ifce->pkt_pool), ring_len(ifce->rx),
		    ring_len(ifce->tx), pkt_pool_get_nb_free());
		return;
	}

	if (buf_get_sbuf_upto_and_skip(buf, &s, "mod") < 0
	    || buf_get_sbuf_upto_and_skip(buf, &s, " ") < 0
	    || buf_addsbuf(&tmp, &s) < 0 || buf_addc(&tmp, '\0') < 0)
		goto usage;

	id = atoi((char *)tmp.data);
	if (id >= NB_MODULES)
		goto usage;

	buf_reset(&tmp);

	for (i = 0; i < sizeof(cmds) / sizeof(cmd_t); i++) {
		const cmd_t *cmd = &cmds[i];
		const uint8_t *a;
		uint8_t c;
		sbuf_t sbuf;

		if (buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, cmd->s) < 0)
			continue;
		if (buf->len > tmp.size)
			goto usage;

		a = cmd->args;
		c = cmd->cmd;
		while (a[0] != ARG_TYPE_NONE) {
			if (fill_args(a[0], &tmp, buf) < 0)
				goto usage;
			a++;
		}
		handle_rx_commands(id, c, &tmp);
		return;
	}
 usage:
	LOG("usage: [help] | mod[0-%d] command\n", NB_MODULES - 1);
}

static int
module_parse_sensor_value(const module_cfg_t *cfg, uint8_t id, buf_t *buf,
			  sensor_value_t *value)
{
	uint8_t features = cfg->features;

	memset(value, 0, sizeof(sensor_value_t));
	if (features & MODULE_FEATURE_HUMIDITY) {
		if (buf_get(buf, &value->humidity,
			    sizeof(value->humidity)) < 0)
			return -1;
	}
	if ((features & MODULE_FEATURE_TEMPERATURE)
	    && buf_getc(buf, (uint8_t *)&value->temperature) < 0)
		return -1;

	return 0;
}

static int
module_parse_status(const module_cfg_t *cfg, uint8_t id, buf_t *buf,
		    module_status_t *status)
{
	uint8_t features = cfg->features;

	memset(status, 0, sizeof(module_status_t));
	if (buf_getc(buf, &status->flags) < 0 ||
	    buf_getc(buf, &status->state) < 0)
		return -1;

	if (features & MODULE_FEATURE_SIREN) {
		if (buf_get_u16(buf, &status->siren.duration) < 0)
			return -1;
		if (buf_getc(buf, &status->siren.timeout) < 0)
			return -1;
	}
	if ((features & (MODULE_FEATURE_HUMIDITY)) &&
	    (buf_getc(buf, &status->sensor.humidity_threshold) < 0 ||
	     buf_get_u16(buf, &status->sensor.report_interval) < 0 ||
	     buf_getc(buf, &status->sensor.notif_addr) < 0))
		return -1;

	return 0;
}

static void module_check_slave_status(uint8_t id, const module_cfg_t *cfg,
				      const module_status_t *status)
{
	ring_t *op_queue = &modules[id].op_queue;
	swen_l3_assoc_t *assoc = &modules[id].assoc;
	uint8_t op;
#ifdef DEBUG
	uint8_t queue_len = ring_len(op_queue);
#endif

	if (cfg->state != status->state) {
		switch (cfg->state) {
		case MODULE_STATE_DISARMED:
			op = CMD_DISARM;
			break;
		case MODULE_STATE_ARMED:
			op = CMD_ARM;
			break;
		case MODULE_STATE_DISABLED:
			op = CMD_DISABLE;
			break;
		default:
			/* should never happen */
			assert(0);
			swen_l3_disassociate(assoc);
			return;
		}
		if (module_add_op(op_queue, op, &assoc->event) < 0)
			goto error;
	}

	if ((cfg->sensor.report_interval != status->sensor.report_interval
	     || cfg->sensor.notif_addr != status->sensor.notif_addr)
	    && module_add_op(op_queue, CMD_GET_SENSOR_REPORT, &assoc->event) < 0)
		goto error;
	if (cfg->sensor.humidity_threshold != status->sensor.humidity_threshold
	    && module_add_op(op_queue, CMD_SET_HUM_TH, &assoc->event) < 0)
		goto error;
	if (!cfg->fan_enabled && (status->flags & STATUS_FLAGS_FAN_ENABLED) &&
	    module_add_op(op_queue, CMD_DISABLE_FAN, &assoc->event) < 0)
		goto error;
	if (cfg->fan_enabled && !(status->flags & STATUS_FLAGS_FAN_ENABLED) &&
	    module_add_op(op_queue, CMD_ENABLE_FAN, &assoc->event) < 0)
		goto error;

	if (cfg->siren_duration != status->siren.duration &&
	    module_add_op(op_queue, CMD_SET_SIREN_DURATION, &assoc->event) < 0)
		goto error;
	if (cfg->siren_timeout != status->siren.timeout &&
	    module_add_op(op_queue, CMD_SET_SIREN_TIMEOUT, &assoc->event) < 0)
		goto error;

	if (modules[id].main_pwr_state == -1)
		modules[id].main_pwr_state = !!(status->flags &
						STATUS_FLAGS_MAIN_PWR_ON);
	else {
		if (modules[id].main_pwr_state == POWER_STATE_ON &&
		    !(status->flags & STATUS_FLAGS_MAIN_PWR_ON)) {
			DEBUG_LOG("Power down action\n");
		} else if (modules[id].main_pwr_state == POWER_STATE_OFF &&
			   status->flags & STATUS_FLAGS_MAIN_PWR_ON) {
			DEBUG_LOG("Power up action\n");
		}
		if (status->flags & STATUS_FLAGS_MAIN_PWR_ON)
			modules[id].main_pwr_state = POWER_STATE_ON;
		else
			modules[id].main_pwr_state = POWER_STATE_OFF;
	}

#ifdef DEBUG
	if (ring_len(op_queue) > queue_len) {
		DEBUG_LOG("wrong status, updating...\n");
		return;
	}
	DEBUG_LOG("status OK\n");
#endif
	return;
 error:
	DEBUG_LOG("failed checking mod%d status\n", id);
	swen_l3_assoc_shutdown(assoc);
	swen_l3_event_unregister(assoc);
	swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
	swen_l3_associate(assoc);
}

#ifdef CONFIG_RF_GENERIC_COMMANDS
static void generic_cmds_print_status(uint8_t status)
{
	LOG("Generic cmd status: ");
	switch (status) {
	case GENERIC_CMD_STATUS_OK:
		LOG("OK\n");
		break;
	case GENERIC_CMD_STATUS_ERROR_FULL:
		LOG("FULL\n");
		break;
	case GENERIC_CMD_STATUS_ERROR_DUPLICATE:
		LOG("DUPLICATE\n");
		break;
	case GENERIC_CMD_STATUS_ERROR_TIMEOUT:
		LOG("TIMEOUT\n");
		break;
	default:
		assert(0);
	}
}

static void generic_cmds_handle_answer(buf_t *buf)
{
	uint8_t status;

	if (buf_getc(buf, &status) < 0)
		return;
	if (status == GENERIC_CMD_STATUS_LIST) {
		LOG("Generic cmd list:\n");
		while (buf->len) {
			uint8_t n, c;

			__buf_getc(buf, &n);
			if (buf_getc(buf, &c) >= 0)
				LOG("id: %u cmd: %u\n", n, c);
		}
		return;
	}
	generic_cmds_print_status(status);
}
#endif

static void module_parse_commands(uint8_t addr, buf_t *buf)
{
	uint8_t cmd;
	uint8_t id = addr_to_module_id(addr);
	module_status_t status;
	sensor_value_t sensor_value;
	module_cfg_t cfg;

	if (addr < RF_MASTER_MOD_HW_ADDR || id > NB_MODULES)
		return;

	cfg_load(&cfg, id);
	if (buf == NULL || buf_getc(buf, &cmd) < 0)
		return;

	switch (cmd) {
	case CMD_STATUS:
		if (module_parse_status(&cfg, id, buf, &status) < 0)
			goto error;
		print_module_status(&cfg, id, &status);
		module_check_slave_status(id, &cfg, &status);
		return;
	case CMD_SENSOR_VALUES:
		if (module_parse_sensor_value(&cfg, id, buf,
					      &sensor_value) < 0)
			goto error;
		print_sensor_values(&cfg, id, &sensor_value);
		return;
	case CMD_NOTIF_ALARM_ON:
		LOG("mod%d: alarm on\n", id);
		return;
	case CMD_NOTIF_MAIN_PWR_DOWN:
		modules[id].main_pwr_state = POWER_STATE_OFF;
		LOG("mod%d: power down\n", id);
		return;
	case CMD_NOTIF_MAIN_PWR_UP:
		modules[id].main_pwr_state = POWER_STATE_ON;
		LOG("mod%d: power up\n", id);
		return;
	case CMD_FEATURES:
		if (buf->len < sizeof(uint8_t))
			goto error;
		if (cfg.features == 0)
			module_add_op(&modules[id].op_queue, CMD_GET_STATUS,
				      &modules[id].assoc.event);
		cfg.features = buf->data[0];
		cfg_update(&cfg, id);
		LOG("mod%d: features updated\n", id);
		return;
	case CMD_STORAGE_ERROR:
		if (!modules[id].faulty) {
			LOG("mod%d has a corrupted storage\n", id);
			modules[id].faulty = 1;
		}
		return;
#ifdef CONFIG_RF_GENERIC_COMMANDS
	case CMD_RECORD_GENERIC_COMMAND_ANSWER:
		generic_cmds_handle_answer(buf);
		return;
#endif
	default:
		return;
	}
 error:
	LOG("bad cmd (0x%X) of len %d from 0x%X\n", cmd, buf->len + 1,
	    addr);
}

#if defined(CONFIG_RF_RECEIVER) && defined(CONFIG_RF_GENERIC_COMMANDS) \
	&& !defined(CONFIG_AVR_SIMU)
static void generic_cmds_cb(uint16_t cmd, uint8_t status)
{
	buf_t args = BUF_INIT(NULL, 0);

	if (status == GENERIC_CMD_STATUS_RCV) {
		DEBUG_LOG("mod0: received generic cmd %u\n", cmd);
		handle_rx_commands(0, cmd, &args);
		return;
	}
	generic_cmds_print_status(status);
}
#endif

static void sensor_report_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	sensor_report_t sensor_report;
	uint8_t id = addr_to_module_id(from);
	module_cfg_t cfg;
	uint8_t cmd;

	if (id > NB_MODULES || buf_getc(buf, &cmd) < 0 ||
	    cmd != CMD_SENSOR_REPORT)
		goto error;

	cfg_load(&cfg, id);
	if (cfg.sensor.report_interval == 0 ||
	    cfg.sensor.notif_addr != RF_MASTER_MOD_HW_ADDR)
		goto error;

	if (cfg.features & MODULE_FEATURE_HUMIDITY) {
		if (buf_getc(buf, &sensor_report.humidity) < 0)
			goto error;
		LOG("humidity value: %u%%\n", sensor_report.humidity);
	}
	if (cfg.features & MODULE_FEATURE_TEMPERATURE) {
		if (buf_getc(buf, (uint8_t *)&sensor_report.temperature) < 0)
			goto error;
		LOG("temperature: %d C\n", sensor_report.temperature);
	}
	return;
 error:
	LOG("received unsolicited sensor report from 0x%X\n", from);
}

static int handle_tx_commands(module_t *module, uint8_t cmd)
{
	buf_t buf;
	void *data = NULL;
	uint8_t len = 0;
	module_cfg_t cfg;

	cfg_load(&cfg, addr_to_module_id(module->assoc.dst));

	switch (cmd) {
	case CMD_SET_HUM_TH:
		data = &cfg.sensor.humidity_threshold;
		len = sizeof(uint8_t);
		break;
	case CMD_GET_SENSOR_REPORT:
		buf = BUF(sizeof(sensor_status_t));
		__buf_add(&buf, &cfg.sensor.report_interval,
			  sizeof(cfg.sensor.report_interval));
		__buf_add(&buf, &cfg.sensor.notif_addr,
			  sizeof(cfg.sensor.notif_addr));
		len = buf.len;
		data = buf.data;
		break;
	case CMD_SET_SIREN_DURATION:
		data = &cfg.siren_duration;
		len = sizeof(uint16_t);
		break;
	case CMD_SET_SIREN_TIMEOUT:
		data = &cfg.siren_timeout;
		len = sizeof(uint8_t);
		break;
	default:
		break;
	}

	return send_rf_msg(&module->assoc, cmd, data, len);
}

static void rf_event_cb(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
	uint8_t id = addr_to_module_id(assoc->dst);
	module_t *module = container_of(assoc, module_t, assoc);

	if (events & EV_READ) {
		pkt_t *pkt;

		while ((pkt = swen_l3_get_pkt(assoc)) != NULL) {
			DEBUG_LOG("got pkt of len:%d from mod%d\n",
				  pkt->buf.len, id);
			module_parse_commands(assoc->dst, &pkt->buf);
			pkt_free(pkt);
		}
	}
	if (events & (EV_ERROR | EV_HUNGUP)) {
		module_cfg_t cfg;

		DEBUG_LOG("mod%d disconnected\n", id);
		if ((events & EV_ERROR)) {
			swen_l3_associate(assoc);
			goto error;
		}
		cfg_load(&cfg, id);
		if (cfg.state == MODULE_STATE_UNINITIALIZED ||
		    cfg.state == MODULE_STATE_DISABLED) {
			swen_l3_event_unregister(assoc);
			swen_l3_assoc_shutdown(assoc);
			return;
		}
		goto error;
	}
	if (events & EV_WRITE) {
		uint8_t op;

		if (module_get_op(&module->op_queue, &op) < 0) {
			swen_l3_event_set_mask(assoc, EV_READ);
			return;
		}
		if (handle_tx_commands(module, op) >= 0) {
			DEBUG_LOG("mod0: sending op:0x%X to mod%d\n", op, id);
			module_skip_op(&module->op_queue);
		} else if (swen_l3_get_state(assoc) != S_STATE_CONNECTED) {
			swen_l3_associate(assoc);
			goto error;
		}
	}
	return;
 error:
	swen_l3_event_unregister(assoc);
	swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
}

static void rf_connecting_on_event(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
	uint8_t id = addr_to_module_id(assoc->dst);

	if (events & EV_ERROR) {
		swen_l3_associate(assoc);
		goto error;
	}
	if (events & EV_HUNGUP)
		goto error;

	if (events & EV_WRITE) {
		module_cfg_t cfg;
		uint8_t op;

		cfg_load(&cfg, id);
		DEBUG_LOG("connected to mod%d\n", id);

		/* get slave statuses */
		if (cfg.features == 0)
			op = CMD_GET_FEATURES;
		else
			op = CMD_GET_STATUS;
		DEBUG_LOG("sending cmd:%X\n", op);
		if (send_rf_msg(assoc, op, NULL, 0) < 0) {
			if (swen_l3_get_state(assoc) != S_STATE_CONNECTED)
				goto error;
			return;
		}
		swen_l3_event_register(assoc, EV_READ|EV_WRITE, rf_event_cb);
	}
	return;
 error:
	DEBUG_LOG("connection to mod%d failed\n", id);
	swen_l3_event_unregister(assoc);
	swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
}

void master_module_init(void)
{
	uint8_t i;
	uint8_t initialized = module_check_magic();

	if (initialized) {
		/* load master module configuration */
		cfg_load(&module_cfg, 0);
	} else {
		module_set_default_cfg(&module_cfg);
		module_cfg.state = MODULE_STATE_DISARMED;
		module_cfg.features = THIS_MODULE_FEATURES;
		cfg_update_master();
		module_update_magic();
	}
	module_init_iface(&rf_iface, &rf_addr);
	swen_ev_set(sensor_report_cb);
	timer_add(&timer_1sec, ONE_SECOND, timer_1sec_cb, NULL);
#ifdef RF_DEBUG
	module1_init();
#endif

#if defined(CONFIG_RF_GENERIC_COMMANDS) && !defined(CONFIG_AVR_SIMU)
	swen_generic_cmds_init(generic_cmds_cb);
#endif
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_power_down_init(INACTIVITY_TIMEOUT, pwr_mgr_on_sleep,
					 &module_cfg);
#endif

	for (i = 0; i < NB_MODULES; i++) {
		module_t *module = &modules[i];
		module_cfg_t cfg;

		if (i == 0)
			continue;

		ring_init(&module->op_queue, OP_QUEUE_SIZE);
		swen_l3_assoc_init(&module->assoc, rf_enc_defkey);

		if (initialized)
			cfg_load(&cfg, i);
		else {
			module_set_default_cfg(&cfg);
			cfg.state = MODULE_STATE_UNINITIALIZED;
			cfg_update(&cfg, i);
			cfg_load(&cfg, i);
		}

		if (cfg.state == MODULE_STATE_DISABLED ||
		    cfg.state == MODULE_STATE_UNINITIALIZED)
			continue;
		swen_l3_event_register(&module->assoc, EV_WRITE,
				       rf_connecting_on_event);
		swen_l3_assoc_bind(&module->assoc, module_id_to_addr(i),
				   &rf_iface);
		if (swen_l3_associate(&module->assoc) < 0)
			__abort();
	}
}
