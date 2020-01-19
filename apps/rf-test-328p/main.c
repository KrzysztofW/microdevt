/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2020, Krzysztof Witek
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

#include <log.h>
#include <_stdio.h>
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/opts.h>
#include <net/pkt-mempool.h>
#include <interrupts.h>
#include <power-management.h>
#include <net/event.h>
#include <net/swen.h>
#include <net/if.h>
#include <drivers/rf.h>
#include <eeprom.h>
#include "gpio.h"

#define INACTIVITY_TIMEOUT 120

static uint8_t rf_addr = 0xFA;
#ifdef CONFIG_RF_SENDER
static uint8_t echo_server;
#define DEST_ADDR rf_addr
#endif

#ifdef CONFIG_RF_RECEIVER
static sbuf_t s_rec_del = SBUF_INITS("rec del");
static sbuf_t s_rec_list = SBUF_INITS("rec list");
static sbuf_t s_rec = SBUF_INITS("rec");
#endif
#ifdef CONFIG_RF_SENDER
static sbuf_t s_send = SBUF_INITS("send");
static sbuf_t s_burst = SBUF_INITS("burst");
static sbuf_t s_replay = SBUF_INITS("replay");
static sbuf_t s_echo = SBUF_INITS("echo");
#endif
#if defined(CONFIG_IFACE_STATS) && defined(DEBUG)
static sbuf_t s_stats = SBUF_INITS("stats");
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS_CHECKS
static sbuf_t s_dump = SBUF_INITS("dump");
static sbuf_t s_erase = SBUF_INITS("erase");
#endif

typedef enum command {
#ifdef CONFIG_RF_RECEIVER
	CMD_REC_DEL,
	CMD_REC_LIST,
	CMD_REC,
#endif
#ifdef CONFIG_RF_SENDER
	CMD_ECHO,
	CMD_SEND,
	CMD_BURST,
	CMD_REPLAY,
#endif
#if defined(CONFIG_IFACE_STATS) && defined(DEBUG)
	CMD_STATS,
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS_CHECKS
	CMD_DUMP,
	CMD_ERASE,
#endif
} command_t;

static cmd_t cmds[] = {
#ifdef CONFIG_RF_RECEIVER
	{ .s = &s_rec_del, .args = { ARG_TYPE_NONE }, .cmd = CMD_REC_DEL, },
	{ .s = &s_rec_list, .args = { ARG_TYPE_NONE }, .cmd = CMD_REC_LIST, },
	{ .s = &s_rec, .args = { ARG_TYPE_UINT8, ARG_TYPE_NONE },
	  .cmd = CMD_REC, },
#endif
#ifdef CONFIG_RF_SENDER
	{ .s = &s_echo, .args = { ARG_TYPE_UINT8, ARG_TYPE_NONE },
	  .cmd = CMD_ECHO, },
	{ .s = &s_send, .args = { ARG_TYPE_UINT8, ARG_TYPE_STRING,
				  ARG_TYPE_NONE }, .cmd = CMD_SEND, },
	{ .s = &s_burst, .args = { ARG_TYPE_UINT8, ARG_TYPE_NONE },
	  .cmd = CMD_BURST, },
	{ .s = &s_replay, .args = { ARG_TYPE_UINT8, ARG_TYPE_NONE },
	  .cmd = CMD_REPLAY, },
#endif
#if defined(CONFIG_IFACE_STATS) && defined(DEBUG)
	{ .s = &s_stats, .args = { ARG_TYPE_NONE }, .cmd = CMD_STATS, },
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS_CHECKS
	{ .s = &s_dump, .args = { ARG_TYPE_NONE }, .cmd = CMD_DUMP, },
	{ .s = &s_erase, .args = { ARG_TYPE_NONE }, .cmd = CMD_ERASE, },
#endif
};

static iface_t rf_iface;
static rf_ctx_t rf_ctx;
static struct iface_queues {
	RING_DECL_IN_STRUCT(pkt_pool, CONFIG_PKT_DRIVER_NB_MAX);
#ifdef CONFIG_RF_RECEIVER
	RING_DECL_IN_STRUCT(rx, CONFIG_PKT_NB_MAX);
#endif
#ifdef CONFIG_RF_SENDER
	RING_DECL_IN_STRUCT(tx, CONFIG_PKT_NB_MAX);
#endif
} iface_queues = {
	.pkt_pool = RING_INIT(iface_queues.pkt_pool),
#ifdef CONFIG_RF_RECEIVER
	.rx = RING_INIT(iface_queues.rx),
#endif
#ifdef CONFIG_RF_SENDER
	.tx = RING_INIT(iface_queues.tx),
#endif
};

#define ONE_SECOND 1000000
static tim_t one_sec_timer = TIMER_INIT(one_sec_timer);

#define UART_RING_SIZE 64
STATIC_RING_DECL(uart_ring, UART_RING_SIZE);

#ifdef CONFIG_RF_SENDER
static void send(buf_t *buf, uint8_t multi)
{
	uint8_t i;
	sbuf_t s = buf2sbuf(buf);

	LOG("sending: %s (x%u time(s))\n", buf->data, multi);
	for (i = 0; i < multi; i++) {
		/* replace the last byte '\0' with 'i' to detect lost pkts */
		buf->data[buf->len - 1] = i;

		if (swen_sendto(&rf_iface, DEST_ADDR, &s) < 0)
			goto error;
	}
	return;
 error:
	LOG("failed sending\n");
}
#endif

#if defined(CONFIG_IFACE_STATS) && defined(DEBUG)
static void print_stats(void)
{
	if_dump_stats(&rf_iface);
	LOG("\nifce pool: %d rx: %d tx:%d\npkt pool: %d\n",
	    ring_len(rf_iface.pkt_pool),
#ifdef CONFIG_RF_RECEIVER
	    ring_len(rf_iface.rx),
#else
	    0,
#endif
#ifdef CONFIG_RF_SENDER
	    ring_len(rf_iface.tx),
#else
	    0,
#endif
	    pkt_pool_get_nb_free());
#ifdef PKT_TRACE
	pkt_get_traced_pkts();
#endif
}
#endif

static void handle_commands(uint8_t cmd, buf_t *args)
{
	uint8_t v;
	const char *s;

	switch (cmd) {
#ifdef CONFIG_RF_RECEIVER
	case CMD_REC_DEL:
		LOG("deleting cmds\n");
		swen_generic_cmds_delete_all_recorded_cmd();
		return;
	case CMD_REC_LIST:
		LOG("recorded cmds\n");
		buf_reset(args);
		swen_generic_cmds_get_list(args);
		buf_print_hex(args);
		while (args->len > 1) {
			uint16_t n;

			__buf_get_u16(args, &n);
			LOG("id: %u cmd: %u\n", n & 0xFF, n >> 8);
		}
		return;
	case CMD_REC:
		LOG("recording cmd %u\n", args->data[0]);
		swen_generic_cmds_start_recording(args->data[0]);
		return;
#endif
#ifdef CONFIG_RF_SENDER
	case CMD_ECHO:
		echo_server = args->data[0];
		if (args->data[0])
			s = "on";
		else
			s = "off";
		LOG("%s %s\n", s_echo.data, s);
		return;
	case CMD_SEND:
		__buf_getc(args, &v);
		send(args, v);
		return;
	case CMD_BURST:
		rf_set_burst(&rf_iface, args->data[0]);
		return;
	case CMD_REPLAY:
		LOG("replaying cmd %u\n", args->data[0]);
		swen_generic_cmd_replay(&rf_iface, args->data[0]);
		return;
#endif
#if defined(CONFIG_IFACE_STATS) && defined(DEBUG)
	case CMD_STATS:
		print_stats();
		return;
#endif
#ifdef CONFIG_RF_GENERIC_COMMANDS_CHECKS
	case CMD_DUMP:
		swen_generic_cmds_dump_storage(0);
		return;
	case CMD_ERASE:
		swen_generic_cmds_erase_storage();
		return;
#endif
	default:
		opts_print_usage(cmds, countof(cmds));
	}
}

static void uart_task(void *arg)
{
	buf_t buf = BUF(UART_RING_SIZE);
	buf_t args = BUF(UART_RING_SIZE);
	uint8_t c;

	while (ring_getc(uart_ring, &c) >= 0) {
		__buf_addc(&buf, c);
		if  (c == '\0')
			break;
	}
	if (buf.len > 1 &&
	    opts_parse_buf(cmds, countof(cmds), &buf, &args,
			   handle_commands) < 0)
		opts_print_usage(cmds, countof(cmds));
}

#ifdef ATMEGA2560
#define USART USART0_RX_vect
#else
#define USART USART_RX_vect
#endif
ISR(USART)
{
	uint8_t c = UDR0;

	if (c == '\r')
		return;
	if (c == '\n') {
		c = '\0';
		schedule_task(uart_task, NULL);
	}
	if (ring_addc(uart_ring, c) < 0)
		ring_reset(uart_ring);
}

#ifdef CONFIG_RF_RECEIVER
static void generic_cmds_cb(uint16_t cmd, uint8_t status)
{
	if (status == GENERIC_CMD_STATUS_RCV) {
		DEBUG_LOG("received generic cmd %u\n", cmd);
		return;
	}
	LOG("Generic cmd status: %s\n", swen_generic_cmd_status2str(status));
}
#endif
static void one_sec_cron(void *arg)
{
	gpio_led_toggle();
	timer_reschedule(&one_sec_timer, ONE_SECOND);
}

#ifdef CONFIG_RF_RECEIVER
static void rf_recv_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	LOG("got: ");
	buf_print(buf);
	LOG("\n");
#ifdef CONFIG_RF_SENDER
	if (echo_server) {
		sbuf_t sbuf = buf2sbuf(buf);

		swen_sendto(&rf_iface, from, &sbuf);
	}
#endif
}
#endif

int main(void)
{
	init_stream0(&stdout, &stdin, 1);
	DEBUG_LOG("KW RF-TEST v0.1 (%s)\n", VERSION);
	timer_subsystem_init();
	irq_enable();

	gpio_init();
	pkt_mempool_init();

#ifndef CONFIG_AVR_SIMU
	watchdog_enable(WATCHDOG_TIMEOUT_8S);
#endif
	rf_iface.hw_addr = &rf_addr;
	rf_iface.recv = rf_input;
#ifdef CONFIG_RF_SENDER
	rf_iface.send = rf_output;
#endif
	rf_iface.flags = IF_UP|IF_RUNNING|IF_NOARP;

	if_init(&rf_iface, IF_TYPE_RF, &iface_queues.pkt_pool,
#ifdef CONFIG_RF_RECEIVER
		&iface_queues.rx,
#else
		NULL,
#endif
#ifdef CONFIG_RF_SENDER
		&iface_queues.tx,
#else
		NULL,
#endif
		1);

	rf_init(&rf_iface, &rf_ctx);
#ifdef CONFIG_RF_RECEIVER
	swen_generic_cmds_init(generic_cmds_cb);
	swen_ev_set(rf_recv_cb);
#endif
	timer_add(&one_sec_timer, ONE_SECOND, one_sec_cron, NULL);

	/* interruptible functions */
	while (1) {
		scheduler_run_task();
#ifndef CONFIG_AVR_SIMU
		watchdog_reset();
#endif
	}
	return 0;
}
