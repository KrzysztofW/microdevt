#include <stdio.h>
#include <net/event.h>
#include <net/swen.h>
#include <net/swen-rc.h>
#include <net/swen-l3.h>
#include <net/swen-cmds.h>
#include <adc.h>
#include <avr/eeprom.h>
#include <interrupts.h>
#include "module.h"
#include "rf-common.h"
#include "module-common.h"
#include "features.h"

/* #define RF_DEBUG */

#define SIREN_ON_DURATION (15 * 60 * 1000000UL)

#if defined (CONFIG_RF_RECEIVER) || defined (CONFIG_RF_SENDER)
static uint8_t rf_addr = RF_MOD0_HW_ADDR;
static iface_t rf_iface;
#ifdef RF_DEBUG
iface_t *rf_debug_iface1 = &rf_iface;
#endif
#endif

#ifdef CONFIG_NETWORKING
extern iface_t eth_iface;
#endif

static tim_t siren_timer;

static sbuf_t s_disarm = SBUF_INITS("disarm");
static sbuf_t s_arm = SBUF_INITS("arm");
static sbuf_t s_get_status = SBUF_INITS("get status");
static sbuf_t s_fan_on = SBUF_INITS("fan on");
static sbuf_t s_fan_off = SBUF_INITS("fan off");
static sbuf_t s_fan_disable = SBUF_INITS("fan disable");
static sbuf_t s_fan_enable = SBUF_INITS("fan enable");
static sbuf_t s_siren_on = SBUF_INITS("siren on");
static sbuf_t s_siren_off = SBUF_INITS("siren off");
static sbuf_t s_humidity_th = SBUF_INITS("humidity th");
static sbuf_t s_report_hum_val = SBUF_INITS("report hum");
static sbuf_t s_disconnect = SBUF_INITS("disconnect");
static sbuf_t s_connect = SBUF_INITS("connect");

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
	{ .s = &s_report_hum_val, .args = { ARG_TYPE_INT16, ARG_TYPE_NONE },
	  .cmd = CMD_GET_REPORT_HUM_VAL },
	{ .s = &s_disconnect, .args = { ARG_TYPE_NONE }, .cmd = CMD_DISCONNECT },
	{ .s = &s_connect, .args = { ARG_TYPE_NONE }, .cmd = CMD_CONNECT },
};

#define NB_MODULES 2
static module_t modules[NB_MODULES];
static module_cfg_t master_cfg;
static module_cfg_t EEMEM persistent_data[NB_MODULES];

static void cfg_load(module_cfg_t *cfg, uint8_t id)
{
	assert(id < NB_MODULES);
	eeprom_read_block(cfg, &persistent_data[id], sizeof(module_cfg_t));
}

static void cfg_update(module_cfg_t *cfg, uint8_t id)
{
	assert(id < NB_MODULES);
	irq_disable();
	eeprom_update_block(cfg, &persistent_data[id], sizeof(module_cfg_t));
	irq_enable();
}

static inline void cfg_load_master(void)
{
	cfg_load(&master_cfg, 0);
	/* master module cannot be disabled */
	if (master_cfg.state == MODULE_STATE_DISABLED) {
		master_cfg.state = MODULE_STATE_DISARMED;
	}
}

static inline void cfg_update_master(void)
{
	cfg_update(&master_cfg, 0);
}

static inline void set_siren_off(void)
{
	PORTB &= ~(1 << PB0);
}

static void siren_tim_cb(void *arg)
{
	set_siren_off();
}

static void set_siren_on(uint8_t force)
{
	if (master_cfg.state == MODULE_STATE_ARMED || force) {
		PORTB |= 1 << PB0;
		timer_del(&siren_timer);
		timer_add(&siren_timer, SIREN_ON_DURATION, siren_tim_cb, NULL);
	}
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

	for (i = 0; i < NB_MODULES; i++)
		if (swen_l3_get_state(&modules[i].assoc) == S_STATE_CONNECTED)
			connected++;
	return connected;
}

static void print_status(uint8_t id, module_status_t *status)
{
	const module_features_t *features = modules[id].features;

	LOG("\nModule %d:\n", id);
	LOG(" State:  %s\n", state_to_string(status->cfg.state));
	if (features->humidity) {
		LOG(" Humidity value:  %u%%\n"
		    " Global humidity value:  %u%%\n"
		    " Humidity tendency:  %s\n"
		    " Humidity threshold:  %u%%\n"
		    " Humidity report interval: %u secs\n",
		    status->humidity_val,
		    status->global_humidity_val,
		    humidity_tendency_to_str(status->humidity_tendency),
		    status->cfg.humidity_threshold,
		    status->cfg.humidity_report_interval);
	}
	if (features->temperature)
		LOG(" Temp:  %u\n", analog_read(3));

	if (features->fan)
		LOG(" Fan:  %s\n Fan enabled:  %s\n",
		    on_off(status->fan_on), yes_no(status->cfg.fan_enabled));
	if (features->siren)
		LOG(" Siren:  %s\n", on_off(status->siren_on));
	if (features->lan)
		LOG(" LAN:  %s\n", on_off(status->lan_up));
	if (features->rf) {
		if (id == 0)
			LOG(" RF connected devices: %u\n",
			    get_connected_rf_devices());
		else
			LOG(" RF:  %s\n", on_off(status->rf_up));
	}
}

static int module_send_cmd(swen_l3_assoc_t *assoc, uint8_t cmd)
{
	buf_t buf = BUF(1);
	sbuf_t sbuf;

	__buf_addc(&buf, cmd);
	sbuf = buf2sbuf(&buf);
	return swen_l3_send(assoc, &sbuf);
}

static void rf_connecting_on_event(event_t *ev, uint8_t events);

static void module_get_master_status(module_status_t *status)
{
	if (master_cfg.state == 0)
		master_cfg.state = MODULE_STATE_DISARMED;
	status->cfg = master_cfg;
	status->temperature = 0;
	status->siren_on = !!(PORTB & (1 << PB0));
	status->lan_up = 0;
	status->rf_up = 1;
}

static void handle_rx_commands(uint8_t id, uint8_t cmd, buf_t *args)
{
	module_cfg_t c;
	module_cfg_t *cfg;
	swen_l3_assoc_t *assoc;

	if ((!modules[id].features->fan
	     && (cmd == CMD_RUN_FAN || cmd == CMD_STOP_FAN ||
		 cmd == CMD_DISABLE_FAN || cmd == CMD_ENABLE_FAN)) ||
	    (!modules[id].features->siren
	     && (cmd == CMD_SIREN_ON || cmd == CMD_SIREN_OFF)) ||
	    (!modules[id].features->humidity && cmd == CMD_SET_HUM_TH)) {
		LOG("unsupported feature\n");
		return;
	}
	if (id == 0)
		cfg = &master_cfg;
	else {
		cfg = &c;
		cfg_load(cfg, id);
	}

	switch (cmd) {
	case CMD_ARM:
		cfg->state = MODULE_STATE_ARMED;
		break;
	case CMD_DISARM:
		cfg->state = MODULE_STATE_DISARMED;
		if (id == 0)
			set_siren_off();
		break;
	case CMD_SET_HUM_TH:
		cfg->humidity_threshold = args->data[0];
		break;
	case CMD_GET_REPORT_HUM_VAL:
		cfg->humidity_report_interval = *(uint16_t *)args->data;
		break;
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

			module_get_master_status(&master_status);
			print_status(0, &master_status);
			return;
		}
		break;
	case CMD_SIREN_ON:
		if (id == 0)
			set_siren_on(1);
		break;
	case CMD_SIREN_OFF:
		if (id == 0)
			set_siren_off();
		break;
	default:
		break;
	}
	cfg_update(cfg, id);
	if (id == 0)
		return;

	if (!modules[id].features->rf) {
		LOG("module does not support RF commands\n");
		return;
	}

	assoc = &modules[id].assoc;

	if (cmd == CMD_CONNECT) {
		if (swen_l3_get_state(assoc) != S_STATE_CLOSED) {
			LOG("disconnect first\n");
			return;
		}
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		swen_l3_associate(&modules[id].assoc);
		return;
	}
	if (cmd == CMD_DISCONNECT) {
		swen_l3_disassociate(assoc);
		return;
	}
	if (swen_l3_get_state(assoc) != S_STATE_CONNECTED) {
		LOG("module not connected\n");
		return;
	}

	__module_add_op(&modules[id].op_queue, cmd);
	swen_l3_event_set_mask(assoc, EV_READ | EV_WRITE);
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
	buf_t tmp = BUF(8);
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

static void module0_parse_commands(uint8_t addr, buf_t *buf)
{
	uint8_t cmd;
	uint8_t id = addr_to_module_id(addr);
	uint8_t val;
	module_status_t *status;

	if (buf == NULL || buf_getc(buf, &cmd) < 0)
		return;

	switch (cmd) {
	case CMD_STATUS:
		if (buf_len(buf) < sizeof(module_status_t))
			goto error;

		if (addr < RF_MOD0_HW_ADDR ||
		    addr - RF_MOD0_HW_ADDR > NB_MODULES) {
			LOG("Address 0x%X not handled\n", addr);
			return;
		}
		status = (module_status_t *)buf_data(buf);
		print_status(id, status);
		return;
	case CMD_NOTIF_ALARM_ON:
		LOG("mod%d: alarm on\n", id);
		return;
	case CMD_REPORT_HUM_VAL:
		if (buf_len(buf) < sizeof(uint8_t))
			goto error;
		val = buf_data(buf)[0];
		LOG("humidity value: %u\n", val);
		return;
	default:
		return;
	}
 error:
	LOG("bad cmd (0x%X) of len %d from 0x%X\n", cmd, buf_len(buf) + 1,
	    addr);
}

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_GENERIC_COMMANDS)
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif

static int handle_tx_commands(module_t *module, uint8_t cmd)
{
	void *data = NULL;
	uint8_t len = 0;
	module_cfg_t cfg;

	if (cmd == CMD_SET_HUM_TH) {
		cfg_load(&cfg, addr_to_module_id(module->assoc.dst));
		data = &cfg.humidity_threshold;
		len = sizeof(uint8_t);
	} else if (cmd == CMD_GET_REPORT_HUM_VAL) {
		cfg_load(&cfg, addr_to_module_id(module->assoc.dst));
		data = &cfg.humidity_report_interval;
		len = sizeof(uint16_t);
	}

	return send_rf_msg(&module->assoc, cmd, data, len);
}

static void rf_event_cb(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
#ifdef DEBUG
	uint8_t id = addr_to_module_id(assoc->dst);
#endif
	module_t *module = container_of(assoc, module_t, assoc);

	if (events & EV_READ) {
		pkt_t *pkt = swen_l3_get_pkt(assoc);

		DEBUG_LOG("got pkt of len:%d from mod%d\n", buf_len(&pkt->buf),
			  id);
		module0_parse_commands(assoc->dst, &pkt->buf);
		pkt_free(pkt);
	}
	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("mod%d disconnected\n", id);
		swen_l3_event_unregister(assoc);
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		if ((events & EV_ERROR) && swen_l3_associate(&module->assoc) < 0)
			__abort();
	}
	if (events & EV_WRITE) {
		uint8_t op;

		if (__module_get_op(&module->op_queue, &op) < 0) {
			swen_l3_event_set_mask(assoc, EV_READ);
			return;
		}

		if (handle_tx_commands(module, op) >= 0) {
			DEBUG_LOG("mod0: sending op:0x%X to mod%d\n", op, id);
			__module_skip_op(&module->op_queue);
		}
	}
}

static void rf_connecting_on_event(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
#ifdef DEBUG
	uint8_t id = addr_to_module_id(assoc->dst);
#endif
	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("failed to connect to mod%d\n", id);
		swen_l3_event_unregister(assoc);
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		return;
	}
	if (events & EV_WRITE) {
		DEBUG_LOG("connected to mod%d\n", id);

		/* get slave statuses */
		if (module_send_cmd(assoc, CMD_GET_STATUS) < 0)
			return;
		swen_l3_event_register(assoc, EV_READ, rf_event_cb);
	}
}

void master_module_init(void)
{
	uint8_t i;

	cfg_load_master();
	module_init_iface(&rf_iface, &rf_addr);
#ifdef RF_DEBUG
	module1_init();
#endif
	for (i = 0; i < NB_MODULES; i++) {
		uint8_t addr;
		module_t *module = &modules[i];
		module_cfg_t cfg;

		module->features = &module_features[i];
		if (i == 0)
			continue;

		cfg_load(&cfg, i);
		ring_init(&module->op_queue, OP_QUEUE_SIZE);
		addr = module_id_to_addr(i);
		swen_l3_assoc_init(&module->assoc, rf_enc_defkey);
		swen_l3_assoc_bind(&module->assoc, addr, &rf_iface);
		swen_l3_event_register(&module->assoc, EV_WRITE,
				       rf_connecting_on_event);
		if (cfg.state == MODULE_STATE_DISABLED)
			continue;
		if (swen_l3_associate(&module->assoc) < 0)
			__abort();
	}
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
}
