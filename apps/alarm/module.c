#include <stdio.h>
#include <net/event.h>
#include <net/swen.h>
#include <net/swen-rc.h>
#include <net/swen-l3.h>
#include <net/swen-cmds.h>
#include <adc.h>
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
static sbuf_t s_humidity_th = SBUF_INITS("humidity th");
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

static const char *state_to_string(uint8_t state)
{
	switch (state) {
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

static int module_send_cmd(swen_l3_assoc_t *assoc, uint8_t cmd)
{
	buf_t buf = BUF(1);
	sbuf_t sbuf;

	__buf_addc(&buf, cmd);
	sbuf = buf2sbuf(&buf);
	return swen_l3_send(assoc, &sbuf);
}

static int send_rf_msg(uint8_t module, uint8_t cmd, const buf_t *args)
{
	buf_t buf = BUF(sizeof(uint8_t) + args->len);
	sbuf_t sbuf;
	swen_l3_assoc_t *assoc = &modules[module].assoc;

	__buf_addc(&buf, cmd);
	buf_addbuf(&buf, args);

	sbuf = buf2sbuf(&buf);
	if (swen_l3_send(assoc, &sbuf) < 0) {
		LOG("sending to module %d failed\n", module);
		modules[module].status.rf_up = 0;
		return -1;
	}
	return 0;
}

static void rf_connecting_on_event(event_t *ev, uint8_t events);

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
		swen_l3_disassociate(&modules[module].assoc);
		modules[module].status.rf_up = 0;
		return;
	}
	if (cmd == CMD_CONNECT) {
		swen_l3_assoc_t *assoc = &modules[module].assoc;

		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		swen_l3_assoc_bind(assoc, assoc->dst, assoc->iface);
		swen_l3_associate(&modules[module].assoc);
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
		    ring_len(ifce->tx), pkt_pool_get_nb_free());
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
		//buf_reset(&args); ?
		return;
	}
 usage:
	LOG("usage: [help] | mod[0-%d] command\n", NB_MODULES - 1);
}

static void module_handle_commands(uint8_t addr, buf_t *buf)
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

/* static void module_set_cfg(module_t *module) */
/* { */
/* 	if (module->cfg.state != module->status.state) { */
/* 	} */
/* } */

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_GENERIC_COMMANDS)
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif

static void rf_event_cb(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
#ifdef DEBUG
	uint8_t m_nb = addr_to_module(assoc->dst);
#endif
	module_t *module = container_of(assoc, module_t, assoc);

	DEBUG_LOG("from:%X ev:0x%X\n", assoc->dst, events);
	if (events & EV_READ) {
		pkt_t *pkt = swen_l3_get_pkt(assoc);

		DEBUG_LOG("got pkt:%p from mod%d\n", pkt, m_nb);
		module_handle_commands(assoc->dst, &pkt->buf);
		pkt_free(pkt);
	}
	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("mod%d disconnected\n", m_nb);
		module->status.rf_up = 0;
		swen_l3_event_unregister(assoc);
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
	}
	if (events & EV_WRITE) {
		DEBUG_LOG("mod%d ready to write\n");
		//module_set_cfg(module);
	}
}

static void rf_connecting_on_event(event_t *ev, uint8_t events)
{
	swen_l3_assoc_t *assoc = swen_l3_event_get_assoc(ev);
#ifdef DEBUG
	uint8_t m_nb = addr_to_module(assoc->dst);
#endif
	module_t *module = container_of(assoc, module_t, assoc);

	if (events & (EV_ERROR | EV_HUNGUP)) {
		DEBUG_LOG("failed to connect to mod%d\n", m_nb);
		module->status.rf_up = 0;
		swen_l3_event_unregister(assoc);
		swen_l3_event_register(assoc, EV_WRITE, rf_connecting_on_event);
		return;
	}
	if (events & EV_WRITE) {
		DEBUG_LOG("connected to mod%d\n", m_nb);
		module->status.rf_up = 1;

		/* get slave statuses */
		if (module_send_cmd(assoc, CMD_GET_STATUS) < 0)
			return;
		swen_l3_event_register(assoc, EV_READ, rf_event_cb);
	}
}

void master_module_init(void)
{
	uint8_t i;

	module_init_iface(&rf_iface, &rf_addr);
#ifdef RF_DEBUG
	module1_init();
#endif
	for (i = 0; i < NB_MODULES; i++) {
		uint8_t addr;
		module_t *module = &modules[i];

		sprintf(module->name, "mod%u", i);
		module->features = &module_features[i];

		if (i == 0)
			continue;

		addr = module_to_addr(i);

		swen_l3_assoc_init(&module->assoc, rf_enc_defkey);
		swen_l3_assoc_bind(&module->assoc, addr, &rf_iface);
		swen_l3_event_register(&module->assoc, EV_WRITE,
				       rf_connecting_on_event);
		if (swen_l3_associate(&module->assoc) < 0)
			__abort();
#ifdef CONFIG_RF_GENERIC_COMMANDS
		swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
	}
}
