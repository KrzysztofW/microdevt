#include <log.h>
#include <_stdio.h>
#include <usart.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <enc28j60.h>
#include <timer.h>
#include <net/config.h>
#include <net/eth.h>
#include <net/route.h>
#include <net/udp.h>
#include <net/socket.h>
#include <net_apps/net_apps.h>
#include <net/swen.h>
#include <net/swen_cmds.h>
#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <scheduler.h>
#include "rf_common.h"
#include "gsm.h"

#define NET
#define GSM

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
static uint8_t rf_addr = RF_MOD0_HW_ADDR;

static iface_t eth1 = {
	.flags = IF_UP|IF_RUNNING|IF_NOARP,
	.hw_addr = &rf_addr,
#ifdef CONFIG_RF_RECEIVER
	.recv = &rf_input,
#endif
#ifdef CONFIG_RF_SENDER
	.send = &rf_output,
#endif
};

static uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};
#endif

#ifdef NET
static uint8_t net_wd;

static uint8_t ip[] = { 192, 168, 0, 99 };
static uint8_t ip_mask[] = { 255, 255, 255, 0 };
static uint8_t mac[] = { 0x62, 0x5F, 0x70, 0x72, 0x61, 0x79 };

static iface_t eth0 = {
	.flags = IF_UP|IF_RUNNING,
	.hw_addr = mac,
	.ip4_addr = ip,
	.ip4_mask = ip_mask,
	.send = &enc28j60_pkt_send,
	.recv = &enc28j60_pkt_recv,
};

#ifdef ENC28J60_INT
ISR(PCINT0_vect)
{
	enc28j60_get_pkts(&eth0);
}
#endif

static void tim_cb_wd(void *arg)
{
	tim_t *timer = arg;

	DEBUG_LOG("bip\n");

	if (net_wd > 0) {
		DEBUG_LOG("resetting net device\n");
#ifndef CONFIG_AVR_SIMU
		enc28j60_init(eth0.mac_addr);
#endif
	}
	net_wd++;
	timer_reschedule(timer, 10000000UL);
}
#endif

#ifdef GSM
static void gsm_cb(uint8_t status)
{
	DEBUG_LOG("sending sms finished with status: %s\n",
		  status ? "ERROR" : "OK");
}
#endif

static int apps_init(void)
{
#ifdef CONFIG_UDP
	if (udp_init() < 0)
		return -1;
#endif
#ifdef CONFIG_TCP
	if (tcp_init() < 0)
		return -1;
#endif
#ifdef CONFIG_DNS
	if (dns_resolver_init() < 0)
		return -1;
#endif
	return 0;
}

#ifndef CONFIG_EVENT
static void apps(void)
{
#ifdef NET
#if defined(CONFIG_UDP) && !defined(CONFIG_EVENT)
	udp_app();
#endif
#if defined(CONFIG_TCP) && !defined(CONFIG_EVENT)
	tcp_app();
#endif
#endif
}
#endif

#ifdef NET
static void net_task_cb(void *arg)
{
	pkt_t *pkt;

	/* this should go the the interrupt handler that should schedule
	 * net_task_cb() */
	eth0.recv(&eth0);

	/* this should be executed in the net_task_cb() */
	eth0.if_input(&eth0);

	net_wd = 0;

	while ((pkt = pkt_get(eth0.tx)) != NULL) {
		eth0.send(&eth0, pkt);
		pkt_free(pkt);
	}

	/* this should be moved to the interrupt handler */
	schedule_task(net_task_cb, NULL);
}
#endif

#ifdef CONFIG_RF_SENDER
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(32);
	sbuf_t sbuf = buf2sbuf(&buf);
	const char *s = "I am your master!";

	__buf_adds(&buf, s, strlen(s));

	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("cannot encode buf\n");
	if (swen_sendto(&eth1, RF_MOD1_HW_ADDR, &sbuf) < 0)
		DEBUG_LOG("failed sending RF msg\n");
	timer_reschedule(timer, 5000000UL);
}
#endif

#ifdef CONFIG_RF_RECEIVER
#ifdef CONFIG_RF_GENERIC_COMMANDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif
#endif

static void blink_led(void *arg)
{
	tim_t *tim = arg;

	PORTB ^= (1 << PB7);
	timer_reschedule(tim, 1000000UL);
}

#ifdef CONFIG_RF_RECEIVER
static void
rf_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ) {
		if (xtea_decode(buf, rf_enc_defkey) < 0) {
			DEBUG_LOG("cannot decode buf\n");
			return;
		}
		DEBUG_LOG("got from 0x%X: %s\n", from, buf_data(buf));
	}
}
#endif

int main(void)
{
	tim_t timer_led;
#ifdef NET
	tim_t timer_wd;
#endif
#ifdef CONFIG_RF_SENDER
	tim_t timer_rf;
#endif
	init_adc();
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 0);
	DEBUG_LOG("KW alarm v0.2\n");
#endif
	timer_subsystem_init();
	watchdog_shutdown();
#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	if (scheduler_init() < 0)
		return -1;

	DDRB = 0xFF; /* LED PIN */
	timer_init(&timer_led);
	timer_add(&timer_led, 5000000, blink_led, &timer_led);

#ifdef NET
	timer_init(&timer_wd);
	timer_add(&timer_wd, 500000UL, tim_cb_wd, &timer_wd);

	if (if_init(&eth0, IF_TYPE_ETHERNET) < 0) {
		DEBUG_LOG("cannot init interface\n");
		return -1;
	}
	if (pkt_mempool_init() < 0) {
		DEBUG_LOG("cannot init pkt pool\n");
		return -1;
	}

	dft_route.iface = &eth0;
	dft_route.ip = 0x0b00a8c0;
#if defined(CONFIG_UDP) || defined(CONFIG_TCP)
	socket_init(); /* check return val in case of hash-table storage */
#endif
	enc28j60Init(eth0.hw_addr);

#ifdef ENC28J60_INT
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT0);
#endif
	schedule_task(net_task_cb, NULL);
#endif
	watchdog_enable();

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
	if (rf_init(&eth1, RF_BURST_NUMBER) < 0) {
		DEBUG_LOG("cannot init RF\n");
		return -1;
	}
#endif
#ifdef CONFIG_RF_RECEIVER
	swen_ev_set(rf_event_cb);
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif

#endif
#ifdef CONFIG_RF_SENDER
	timer_init(&timer_rf);
	timer_add(&timer_rf, 0, tim_rf_cb, &timer_rf);

	/* port F used by the RF sender */
	DDRF = (1 << PF1);
#endif

	if (apps_init() < 0)
		return -1;

#ifdef GSM
	gsm_init(gsm_cb);
#endif

	/* slow functions */
	while (1) {
		bh(); /* bottom halves */

#ifndef CONFIG_EVENT
		apps();
#endif
#ifdef ENC28J60_INT
		delay_us(10);
#endif
		watchdog_reset();
	}
	return 0;
}
