#include <stdio.h>
#include <net/event.h>
#include <net/swen-l3.h>
#include <adc.h>
#include "module.h"
#include "rf-common.h"
#include "features.h"

#define SIREN_ON_DURATION (15 * 60 * 1000000UL)

extern iface_t rf_iface;
extern uint32_t rf_enc_defkey[4];

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
	modules[nb].status = *status;
}

static void print_status(uint8_t nb)
{
	const module_status_t *status = &modules[nb].status;
	const module_features_t *features = modules[nb].features;

	printf_P(PSTR("\nModule %d:\n"), nb);
	printf_P(PSTR(" State:\t%s\n"), state_to_string(status->state));
	if (features->humidity) {
		printf_P(PSTR(" Humidity value:\t%u\n"
			      " Global humidity value:\t%u\n"
			      " Humidity tendency:\t%u\n"
			      " Humidity threshold:\t%u\n"),
			 status->humidity_val,
			 status->global_humidity_val,
			 status->humidity_tendency,
			 status->humidity_threshold);
	}
	if (features->temperature)
		printf_P(PSTR(" Temp:\t%u\n"), status->temperature);

	if (features->fan)
		printf_P(PSTR(" Fan: %s\n Fan enabled:\t%s\n"),
			 on_off(status->fan_on), yes_no(status->fan_enabled));

	if (features->siren)
		printf_P(PSTR(" Siren:\t%s\n"), on_off(status->siren_on));
	if (features->lan)
		printf_P(PSTR(" LAN:\t%s\n"), on_off(status->lan_up));
	if (features->rf)
		printf_P(PSTR(" RF:\t%s\n"), on_off(status->rf_up));
}

static void send_rf_msg(uint8_t module, uint8_t cmd, const buf_t *args)
{
	buf_t buf = BUF(sizeof(uint8_t) + args->len);
	sbuf_t sbuf;
	swen_l3_assoc_t *assoc = &modules[module].assoc;

	if (!modules[module].status.rf_up) {
		printf_P(PSTR("module %d not connected\n"), module);
		return;
	}
	__buf_addc(&buf, cmd);
	buf_addbuf(&buf, args);

	sbuf = buf2sbuf(&buf);
	if (swen_l3_send(assoc, &sbuf) < 0)
		modules[module].status.rf_up = 0;
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
		printf_P(PSTR("unhandled module %d\n"), module);
		return;
	}
	if ((!modules[module].features->fan
	     && (cmd == CMD_RUN_FAN || cmd == CMD_STOP_FAN ||
		 cmd == CMD_DISABLE_FAN || cmd == CMD_ENABLE_FAN)) ||
	    (!modules[module].features->siren
	     && (cmd == CMD_SIREN_ON || cmd == CMD_SIREN_OFF)) ||
	    (!modules[module].features->humidity && cmd == CMD_SET_HUM_TH)) {
		printf_P(PSTR("unsupported feature\n"));
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

	puts("");
	for (i = 0; i < sizeof(cmds) / sizeof(cmd_t); i++) {
		cmd_t *cmd = &cmds[i];
		uint8_t *args = cmd->args;

		printf_P(PSTR("mod[nb] %s"), cmd->s->data);
		while (args[0] != ARG_TYPE_NONE) {
			if (args[0] == ARG_TYPE_STRING)
				printf_P(PSTR(" <string>"));
			else
				printf_P(PSTR(" <number>"));
			args++;
		}
		puts("");
	}
	puts("");
}

void alarm_parse_uart_commands(buf_t *buf)
{
	buf_t args = BUF(8);
	uint8_t module;
	uint8_t i;
	sbuf_t s;

	if (buf_get_sbuf_upto_and_skip(buf, &s, "help") >= 0) {
		print_usage();
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
	printf_P(PSTR("usage: [help] | mod[0-%d] command\n"), NB_MODULES - 1);
}

static void handle_rf_replies(uint8_t addr, buf_t *buf)
{
	uint8_t cmd = buf_data(buf)[0];
	uint8_t module;

	switch (cmd) {
	case CMD_STATUS:
		if (buf_len(buf) - 1 < sizeof(module_status_t)) {
			printf_P(PSTR("corrupted message of len %d from 0x%X\n"),
				 buf_len(buf), addr);
			return;
		}
		if (addr < RF_MOD0_HW_ADDR ||
		    addr - RF_MOD0_HW_ADDR > NB_MODULES) {
			printf_P(PSTR("Address 0x%X not handled\n"), addr);
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

static void rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ) {
		if (buf == NULL) {
			if (from == RF_MOD1_HW_ADDR)
				modules[1].status.rf_up = 1;
			return;
		}
		handle_rf_replies(from, buf);
	} else if (events & EV_ERROR) {
		if (from == RF_MOD1_HW_ADDR)
			modules[1].status.rf_up = 0;
	}
}

void module_init(void)
{
	uint8_t i;

	swen_ev_set(rf_event_cb);

	for (i = 0; i < NB_MODULES; i++) {
		module_t *m = &modules[i];
		swen_l3_assoc_t *assoc;
		uint8_t addr;

		sprintf(m->name, "mod%u", i);
		m->features = &module_features[i];

		if (i == 0)
			continue;
		assoc = &m->assoc;
		addr = module_to_addr(i);

		swen_l3_assoc_init(assoc);
		swen_l3_assoc_bind(assoc, addr, &rf_iface,
				   rf_enc_defkey);
		if (swen_l3_associate(assoc) < 0)
			__abort();
	}
}
