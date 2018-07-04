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
#include <net-apps/net-apps.h>
#include <net/swen.h>
#include <net/swen-cmds.h>
#include <crypto/xtea.h>
#include <drivers/rf.h>
#include <drivers/gsm-at.h>
#include <scheduler.h>
#include "rf-common.h"

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

#ifdef CONFIG_GSM_SIM900
static FILE *gsm_in, *gsm_out;
#endif

#define ENC28J60_INT
#ifdef CONFIG_NETWORKING
//static uint8_t net_wd;

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

#if 0
static void tim_cb_wd(void *arg)
{
	tim_t *timer = arg;

	DEBUG_LOG("bip\n");

	if (net_wd > 1) {
		DEBUG_LOG("resetting net device\n");
#ifndef CONFIG_AVR_SIMU
		enc28j60_init(eth0.mac_addr);
#endif
	}
	net_wd++;
	timer_reschedule(timer, 10000000UL);
}
#else
extern ring_t *pkt_pool;

static void tim_cb_wd(void *arg)
{
	tim_t *timer = arg;

	timer_reschedule(timer, 10000000UL);
	printf("eth1: pool:%d tx:%d rx:%d if_pool:%d\n",
	       ring_len(pkt_pool),
	       ring_len(eth1.tx), ring_len(eth1.rx), ring_len(eth1.pkt_pool));
	printf("temp:%u\n", analog_read(3));
}
#endif
#endif

#ifdef CONFIG_GSM_SIM900
ISR(USART1_RX_vect)
{
	gsm_handle_interrupt(UDR1);
}

static void gsm_cb(uint8_t status, const sbuf_t *from, const sbuf_t *msg)
{
	DEBUG_LOG("gsm callback (status:%d)\n", status);
	if (status == GSM_STATUS_RECV) {
		DEBUG_LOG("from:");
		sbuf_print(from);
		DEBUG_LOG(" msg:<");
		sbuf_print(msg);
		DEBUG_LOG(">\n");
	}
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
#ifdef CONFIG_NETWORKING
#if defined(CONFIG_UDP) && !defined(CONFIG_EVENT)
	udp_app();
#endif
#if defined(CONFIG_TCP) && !defined(CONFIG_EVENT)
	tcp_app();
#endif
#endif
}
#endif

#ifdef CONFIG_NETWORKING
#if 0
static void net_task_cb(void *arg)
{
	pkt_t *pkt;

	eth0.recv(&eth0);
	eth0.if_input(&eth0);
	while ((pkt = pkt_get(eth0.tx)) != NULL) {
		net_wd = 0;
		eth0.send(&eth0, pkt);
	}
	schedule_task(net_task_cb, NULL);
}
#endif
#ifdef ENC28J60_INT
ISR(PCINT0_vect)
{
	/* if (ring_is_empty(eth0.tx)) */
	/* 	schedule_task(net_task_cb, NULL); */
	//eth0.recv(&eth0);
	enc28j60_handle_interrupts(&eth0);
}
#endif
#endif

#ifdef CONFIG_RF_SENDER
static void tim_rf_cb(void *arg)
{
	tim_t *timer = arg;
	buf_t buf = BUF(32);
	sbuf_t sbuf;
	const char *s = "I am your master!";

	__buf_adds(&buf, s, strlen(s));

	if (xtea_encode(&buf, rf_enc_defkey) < 0)
		DEBUG_LOG("cannot encode buf\n");
	sbuf = buf2sbuf(&buf);
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
INIT_ADC_DECL(f, DDRF, PORTF)

static void blink_led(void *arg)
{
	tim_t *tim = arg;
#ifdef CONFIG_GSM_SIM900
	static uint8_t gsm;
#endif
	PORTB ^= (1 << PB7);
	timer_reschedule(tim, 1000000UL);
#ifdef CONFIG_GSM_SIM900
	if ((gsm % 60) == 0)
		gsm_send_sms("+33687236420", "SMS from KW alarm");
	gsm++;
#endif
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
#ifdef CONFIG_NETWORKING
	tim_t timer_wd;
#endif
#ifdef CONFIG_RF_SENDER
	tim_t timer_rf;
#endif
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 0);
	DEBUG_LOG("KW alarm v0.2\n");
#endif
#ifdef CONFIG_GSM_SIM900
	init_stream1(&gsm_in, &gsm_out, 1);
#endif
	init_adc_f();
	timer_subsystem_init();
	watchdog_shutdown();
#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif
	scheduler_init();
	DDRB = 0xFF; /* LED PIN */
	timer_init(&timer_led);
	timer_add(&timer_led, 0, blink_led, &timer_led);

#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER	\
	|| defined CONFIG_NETWORKING
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
#endif
#ifdef CONFIG_NETWORKING
	timer_init(&timer_wd);
	timer_add(&timer_wd, 500000UL, tim_cb_wd, &timer_wd);

	if_init(&eth0, IF_TYPE_ETHERNET, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 0);

	dft_route.iface = &eth0;
	dft_route.ip = 0x0b00a8c0;
#if defined(CONFIG_UDP) || defined(CONFIG_TCP)
	socket_init();
#endif

#ifdef ENC28J60_INT
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT5);
#endif
	enc28j60_init(eth0.hw_addr);
#endif
#if defined CONFIG_RF_RECEIVER || defined CONFIG_RF_SENDER
	if_init(&eth1, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(&eth1, 1);
#ifdef CONFIG_RF_RECEIVER
	swen_ev_set(rf_event_cb);
#ifdef CONFIG_RF_GENERIC_COMMANDS
	swen_generic_cmds_init(rf_kerui_cb, rf_ke_cmds);
#endif
#endif

#ifdef CONFIG_RF_SENDER
	timer_init(&timer_rf);
	timer_add(&timer_rf, 3000000, tim_rf_cb, &timer_rf);
	/* port F used by the RF sender */
	DDRF = (1 << PF1);
#endif
#endif
	watchdog_enable();

	if (apps_init() < 0)
		__abort();
#ifdef CONFIG_GSM_SIM900
	gsm_init(gsm_in, gsm_out, gsm_cb);
#endif

	/* slow functions */
	while (1) {
		scheduler_run_tasks();
#ifdef CONFIG_NETWORKING
		eth0.recv(&eth0);
#ifdef ENC28J60_INT
		/* We must sleep if EIE_PKTIE is set and pkts are received here
		 * (not in the interrupt handler). */
		/* delay_us(10); */
#endif
#endif
#ifndef CONFIG_EVENT
		apps();
#endif
		watchdog_reset();
	}
	return 0;
}
