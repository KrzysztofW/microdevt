#define NET

#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <enc28j60.h>
#include <rf.h>

#include <timer.h>
#include <net/config.h>
#include <net/eth.h>
#include <net/route.h>
#include <net/udp.h>
#include <net/socket.h>
#include <net_apps/net_apps.h>

#ifdef NET
uint8_t net_wd;

iface_t eth0 = {
	.flags = IF_UP|IF_RUNNING,
	.mac_addr = { 0x62, 0x5F, 0x70, 0x72, 0x61, 0x79 },
	.ip4_addr = { 192, 168, 0, 99 },
	.ip4_mask = { 255, 255, 255, 0 },
};

#ifdef NET_INT
ISR(PCINT0_vect)
{
	enc28j60_get_pkts(&eth0);
}
#endif

void tim_cb_wd(void *arg)
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

int apps_init(void)
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
void apps(void)
{
#if defined(CONFIG_UDP) && !defined(CONFIG_EVENT)
	udp_app();
#endif
#if defined(CONFIG_TCP) && !defined(CONFIG_EVENT)
	tcp_app();
#endif
}
#endif

static void bh(void)
{
#ifdef NET
	pkt_t *pkt;

	if ((pkt = eth0.recv()) == NULL)
		goto send;

	eth_input(pkt, &eth0);
	net_wd = 0;
 send:
	while ((pkt = pkt_get(&eth0.tx)) != NULL) {
		cli();
		eth0.send(&pkt->buf);
		sei();
		pkt_free(pkt);
	}
#endif
}

#ifdef CONFIG_RF_KERUI_CMDS
static void rf_kerui_cb(int nb)
{
	DEBUG_LOG("received kerui cmd %d\n", nb);
}
#endif
#define RF_BUF_SIZE 64
uint8_t rf_buf_data[RF_BUF_SIZE];

static void blink_led(void *arg)
{
	tim_t *tim = arg;

	PORTB ^= (1 << PB7);
	timer_reschedule(tim, 1000000UL);
}

int main(void)
{
	tim_t timer_led;
#ifdef NET
	tim_t timer_wd;
#endif
#ifdef CONFIG_RF_RECEIVER
	buf_t rf_buf;
	uint8_t rf_from;
#endif

	init_adc();
#ifdef DEBUG
	init_stream0(&stdout, &stdin);
	DEBUG_LOG("KW alarm v0.2\n");
#endif
	timer_subsystem_init();
	watchdog_shutdown();
#ifdef CONFIG_TIMER_CHECKS
	timer_checks();
#endif

	DDRB = 0xFF; /* LED PIN */
	timer_init(&timer_led);
	timer_add(&timer_led, 0, blink_led, &timer_led);

#ifdef NET
	timer_init(&timer_wd);
	timer_add(&timer_wd, 500000UL, tim_cb_wd, &timer_wd);

	if (if_init(&eth0, &enc28j60_pkt_send, &enc28j60_pkt_recv) < 0) {
		DEBUG_LOG("can't initialize interface\n");
		return -1;
	}
	if (pkt_mempool_init() < 0) {
		DEBUG_LOG("can't initialize pkt pool\n");
		return -1;
	}

	dft_route.iface = &eth0;
	dft_route.ip = 0x0b00a8c0;
#if defined(CONFIG_UDP) || defined(CONFIG_TCP)
	socket_init(); /* check return val in case of hash-table storage */
#endif
#ifdef NET
	enc28j60_init(eth0.mac_addr);
#endif

#ifdef NET_INT
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT0);
#endif
#endif
	watchdog_enable();

#ifdef CONFIG_RF_RECEIVER
	if (rf_init() < 0) {
		DEBUG_LOG("can't initialize RF\n");
		return -1;
	}
	buf_init(&rf_buf, rf_buf_data, RF_BUF_SIZE);
	rf_buf.len = 0;
#endif
#ifdef CONFIG_RF_KERUI_CMDS
	rf_set_kerui_cb(rf_kerui_cb);
#endif

	delay_ms(3000);
	if (apps_init() < 0)
		return -1;

	/* slow functions */
	while (1) {
		bh(); /* bottom halves */
#ifdef CONFIG_RF_RECEIVER
		/* TODO: this block should be put in a timer cb */
		if (rf_recvfrom(&rf_from, &rf_buf) >= 0 && buf_len(&rf_buf)) {
			DEBUG_LOG("got: %s\n", rf_buf.data);
			buf_reset(&rf_buf);
		}
#endif

#if defined(NET) && !defined(CONFIG_EVENT)
		apps();
#endif

#ifdef NET_INT
		/* strangly this is necessary if using SPI interrupts */
		delay_us(10);
#endif
		watchdog_reset();
	}
#ifdef CONFIG_RF_RECEIVER
	/* rf_shutdown(); */
#endif
	return 0;
}
