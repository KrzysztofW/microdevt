#include <stdio.h>
#include <net/event.h>
#include <net/swen.h>
#include <net/swen-rc.h>
#include <net/swen-l3.h>
#include <adc.h>
#include "module.h"
#include "rf-common.h"
#include "features.h"

#define SIREN_ON_DURATION (15 * 60 * 1000000UL)

extern iface_t rf_iface;
#ifdef CONFIG_NETWORKING
extern iface_t eth_iface;
#endif
extern uint32_t rf_enc_defkey[4];

#ifdef CONFIG_SWEN_ROLLING_CODES
static uint32_t local_counter;
static uint32_t remote_counter;
#endif

#define NB_MODULES 2
static module_t modules[NB_MODULES];

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
static sbuf_t s_humidity_th  = SBUF_INITS("humidity th");
static sbuf_t s_disconnect = SBUF_INITS("disconnect");
static sbuf_t s_connect = SBUF_INITS("connect");

#define MAX_ARGS 8
typedef enum arg_type {
	ARG_TYPE_NONE,
	ARG_TYPE_INT8,
	ARG_TYPE_INT16,
	ARG_TYPE_INT32,
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
	{ .s = &s_humidity_th, .args = { ARG_TYPE_INT16, ARG_TYPE_NONE },
	  .cmd = CMD_SET_HUM_TH },
	{ .s = &s_disconnect, .args = { ARG_TYPE_NONE }, .cmd = CMD_DISCONNECT },
	{ .s = &s_connect, .args = { ARG_TYPE_NONE }, .cmd = CMD_CONNECT },
};

static uint8_t module_to_addr(uint8_t module)
{
	return RF_MOD0_HW_ADDR + module;
}

static uint8_t addr_to_module(uint8_t addr)
{
	return addr - RF_MOD0_HW_ADDR;
}

static void set_siren_off(void)
{
	DDRB &= ~(1 << PB0);
	modules[0].status.siren_on = 0;
}

static void siren_tim_cb(void *arg)
{
	set_siren_off();
}

static void set_siren_on(uint8_t force)
{
	if (modules[0].status.state == MODULE_STATE_ARMED || force) {
		DDRB |= 1 << PB0;
		modules[0].status.siren_on = 1;
		timer_del(&siren_timer);
		timer_add(&siren_timer, SIREN_ON_DURATION, siren_tim_cb, NULL);
	}
}

void pir_on_action(void *arg)
{
	set_siren_on(0);
}

static const char *state_to_string(uint8_t state)
{
	switch (state) {
	case MODULE_STATE_INIT:
		return "INIT";
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

static void module_set_status(uint8_t nb, const module_status_t *status)
{
	uint8_t rf_up = modules[nb].status.rf_up;

	modules[nb].status = *status;
	modules[nb].status.rf_up = rf_up;
}

static void print_status(uint8_t nb)
{
	const module_status_t *status = &modules[nb].status;
	const module_features_t *features = modules[nb].features;

	LOG("\nModule %d:\n", nb);
	LOG(" State:  %s\n", state_to_string(status->state));
	if (features->humidity) {
		LOG(" Humidity value:  %u\n"
		    " Global humidity value:  %u\n"
		    " Humidity tendency:  %u\n"
		    " Humidity threshold:  %u\n",
		    status->humidity_val,
		    status->global_humidity_val,
		    status->humidity_tendency,
		    status->humidity_threshold);
	}
	if (features->temperature)
		LOG(" Temp:  %u\n", analog_read(3));

	if (features->fan)
		LOG(" Fan: %s\n Fan enabled:  %s\n",
		    on_off(status->fan_on), yes_no(status->fan_enabled));

	if (features->siren)
		LOG(" Siren:  %s\n", on_off(status->siren_on));
	if (features->lan)
		LOG(" LAN:  %s\n", on_off(status->lan_up));
	if (features->rf)
		LOG(" RF:  %s\n", on_off(status->rf_up));
}

static void send_rf_msg(uint8_t module, uint8_t cmd, const buf_t *args)
{
	buf_t buf = BUF(sizeof(uint8_t) + args->len);
	sbuf_t sbuf;
#ifdef CONFIG_SWEN_ROLLING_CODES
	swen_rc_ctx_t *rc_ctx = &modules[module].rc_ctx;
#else
	swen_l3_assoc_t *assoc = &modules[module].assoc;
#endif

	__buf_addc(&buf, cmd);
	buf_addbuf(&buf, args);

	sbuf = buf2sbuf(&buf);
#ifdef CONFIG_SWEN_ROLLING_CODES
	if (swen_rc_sendto(rc_ctx, &sbuf) >= 0)
		remote_counter++;
#else
	if (swen_l3_send(assoc, &sbuf) < 0) {
		LOG("sending to module %d failed\n", module);
		modules[module].status.rf_up = 0;
	}
#endif
}

static void handle_commands(uint8_t module, uint8_t cmd, const buf_t *args)
{
	if (module == 0) {
		switch (cmd) {
		case CMD_ARM:
			modules[0].status.state = MODULE_STATE_ARMED;
			return;
		case CMD_DISARM:
			modules[0].status.state = MODULE_STATE_DISARMED;
			set_siren_off();
			return;
		case CMD_GET_STATUS:
			print_status(module);
			return;
		case CMD_SIREN_ON:
			set_siren_on(1);
			return;
		case CMD_SIREN_OFF:
			set_siren_off();
			return;
		default:
			/* unsupported */
			return;
		}
	} else if (module > NB_MODULES) {
		LOG("unhandled module %d\n", module);
		return;
	}
	if (!modules[module].features->rf) {
		LOG("module does not support RF commands\n");
		return;
	}
	if (cmd == CMD_DISCONNECT) {
#ifndef CONFIG_SWEN_ROLLING_CODES
		swen_l3_disassociate(&modules[module].assoc);
		modules[module].status.rf_up = 0;
#endif
		return;
	}
	if (cmd == CMD_CONNECT) {
#ifndef CONFIG_SWEN_ROLLING_CODES
		swen_l3_associate(&modules[module].assoc);
#endif
		return;
	}
	if ((!modules[module].features->fan
	     && (cmd == CMD_RUN_FAN || cmd == CMD_STOP_FAN ||
		 cmd == CMD_DISABLE_FAN || cmd == CMD_ENABLE_FAN)) ||
	    (!modules[module].features->siren
	     && (cmd == CMD_SIREN_ON || cmd == CMD_SIREN_OFF)) ||
	    (!modules[module].features->humidity && cmd == CMD_SET_HUM_TH)) {
		LOG("unsupported feature\n");
		return;
	}
	send_rf_msg(module, cmd, args);
}

static int fill_args(uint8_t arg, buf_t *args, buf_t *buf)
{
	int skip = 0;
	uint16_t v16;
	uint32_t v32;

	switch (arg) {
	case ARG_TYPE_INT8:
		if (buf_addc(args, atoi((char *)buf->data)) < 0)
			return -1;
		skip = sizeof(uint8_t);
		break;
	case ARG_TYPE_INT16:
		v16 = atoi((char *)buf->data);
		if (buf_add(args, &v16, sizeof(uint16_t)) < 0)
			return -1;
		skip = sizeof(uint16_t);
		break;
	case ARG_TYPE_INT32:
		v32 = atoi((char *)buf->data);
		if (buf_add(args, &v32, sizeof(uint32_t)) < 0)
			return -1;
		skip = sizeof(uint32_t);
		break;
	case ARG_TYPE_STRING:
		skip = args->len;
		if (buf_adds(args, (char *)buf->data) < 0)
			return -1;
		skip = args->len - skip;
		break;
	case ARG_TYPE_NONE:
		return -1;
	}
	__buf_skip(buf, skip);
	return 0;
}

static void print_usage(void)
{
	uint8_t i;

	LOG("");
	for (i = 0; i < sizeof(cmds) / sizeof(cmd_t); i++) {
		cmd_t *cmd = &cmds[i];
		uint8_t *args = cmd->args;

		LOG("mod[nb] %s", cmd->s->data);
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
	buf_t args = BUF(8);
	uint8_t module;
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
		    ring_len(ifce->tx), ring_len(pkt_pool));
		return;
	}

	for (i = 0; i < NB_MODULES; i++) {
		module_t *m = &modules[i];

		if (buf_get_sbuf_upto_and_skip(buf, &s, m->name) >= 0) {
			module = i;
			break;
		}
	}

	if ((i == NB_MODULES) || buf_get_sbuf_upto_and_skip(buf, &s, " ") < 0)
		goto usage;

	for (i = 0; i < sizeof(cmds) / sizeof(cmd_t); i++) {
		const cmd_t *cmd = &cmds[i];
		const uint8_t *a;
		uint8_t c;
		sbuf_t sbuf;

		if (buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, cmd->s) < 0)
			continue;
		if (buf->len > args.size)
			goto usage;

		a = cmd->args;
		c = cmd->cmd;
		while (a[0] != ARG_TYPE_NONE) {
			if (fill_args(a[0], &args, buf) < 0)
				goto usage;
			a++;
		}
		handle_commands(module, c, &args);
		return;
	}
 usage:
	LOG("usage: [help] | mod[0-%d] command\n", NB_MODULES - 1);
}
#ifdef RF_DEBUG
static void send_status(void)
{
	buf_t buf = BUF(sizeof(uint8_t) + sizeof(module_status_t));
	sbuf_t sbuf;
	module_status_t status;

	memset(&status, 0, sizeof(module_status_t));
	__buf_addc(&buf, CMD_STATUS);
	__buf_add(&buf, &status, sizeof(module_status_t));
	sbuf = buf2sbuf(&buf);
#ifdef CONFIG_SWEN_ROLLING_CODES
	if (swen_rc_sendto(&debug_rc_ctx, &sbuf) >= 0)
		debug_remote_counter++;
#else
	if (swen_l3_send(&debug_assoc, &sbuf) < 0)
		printf("failed sending status\n");
#endif
}
#endif
static void handle_rf_replies(uint8_t addr, buf_t *buf)
{
	uint8_t cmd;
	uint8_t module;

	if (buf == NULL)
		return;
	cmd = buf_data(buf)[0];
	switch (cmd) {
	case CMD_STATUS:
		if (buf_len(buf) - 1 < sizeof(module_status_t)) {
			LOG("corrupted message of len %d from 0x%X\n",
			    buf_len(buf), addr);
			return;
		}
		if (addr < RF_MOD0_HW_ADDR ||
		    addr - RF_MOD0_HW_ADDR > NB_MODULES) {
			LOG("Address 0x%X not handled\n", addr);
			return;
		}
		module = addr_to_module(addr);
		module_set_status(module, (module_status_t *)(buf_data(buf) + 1));
		print_status(module);
		return;
	case CMD_NOTIF_ALARM_ON:
		/* unsupported yet */
		return;
	default:
		return;
	}
}

#ifdef CONFIG_SWEN_ROLLING_CODES
static void set_rc_cnt(uint32_t *counter, uint8_t value)
{
	*counter += value;
	// eeprom ...
}
#endif

static void rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
#ifdef CONFIG_SWEN_ROLLING_CODES
	if (events & EV_READ) {
		if (from == RF_MOD1_HW_ADDR)
			modules[1].local_counter++;
		handle_rf_replies(from, buf);
	}
#else
	if (events & EV_READ) {
		if (from == RF_MOD1_HW_ADDR) {
			LOG("mod1 connected\n");
			modules[1].status.rf_up = 1;
		}
		handle_rf_replies(from, buf);
	} else if (events & EV_ERROR) {
		LOG("failed sending to 0x%02X\n", from);
		if (from == RF_MOD1_HW_ADDR)
			modules[1].status.rf_up = 0;
	}
#endif
}

void module_init(void)
{
	uint8_t i;

	swen_ev_set(rf_event_cb);

	for (i = 0; i < NB_MODULES; i++) {
		module_t *m = &modules[i];
#ifndef CONFIG_SWEN_ROLLING_CODES
		swen_l3_assoc_t *assoc;
#endif
		uint8_t addr;

		sprintf(m->name, "mod%u", i);
		m->features = &module_features[i];

		if (i == 0)
			continue;
		addr = module_to_addr(i);
#ifndef CONFIG_SWEN_ROLLING_CODES
		assoc = &m->assoc;
		swen_l3_assoc_init(assoc, rf_enc_defkey);
		swen_l3_assoc_bind(assoc, addr, &rf_iface);
		swen_l3_set_events(assoc, EV_READ | EV_WRITE);
		if (swen_l3_associate(assoc) < 0)
			__abort();
#else
		swen_rc_init(&m->rc_ctx, &rf_iface, addr,
			     &local_counter, &remote_counter, set_rc_cnt,
			     rf_enc_defkey);
#endif

	}
}
