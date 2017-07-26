#include <avr/io.h>
#include <stdio.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>

#define NET

#ifdef DEBUG
#include "usart0.h"
#define SERIAL_SPEED 57600
#define SYSTEM_CLOCK F_CPU
#endif
#include "avr_utils.h"
#include "ring.h"
#include "timer.h"
#include "rf.h"
#include "adc.h"
#include "net/config.h"
#include "enc28j60.h"

#ifdef NET
int net_wd;
#define NET_TX_SIZE  256
#define NET_RX_SIZE  128

iface_t eth0 = {
	.flags = IFF_UP|IFF_RUNNING,
	.mac_addr = { 0x62, 0x5F, 0x70, 0x72, 0x61, 0x79 },
	.ip4_addr = { 192, 168, 0, 99 },
};
#endif

#ifdef DEBUG
static int my_putchar(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r') {
		usart0_put('\r');
		usart0_put('\n');
	}
	usart0_put(c);
	return 0;
}

static int my_getchar(FILE * stream)
{
	(void)stream;
	return usart0_get();
}

/*
 * Define the input and output streams.
 * The stream implemenation uses pointers to functions.
 */
static FILE my_stream =
	FDEV_SETUP_STREAM (my_putchar, my_getchar, _FDEV_SETUP_RW);

static void init_streams(void)
{
	/* initialize the standard streams to the user defined one */
	stdout = &my_stream;
	stdin = &my_stream;
	usart0_init(BAUD_RATE(SYSTEM_CLOCK, SERIAL_SPEED));
}
#endif

#ifdef NET
static void net_reset(void)
{
	CLKPR = (1<<CLKPCE);
	CLKPR = 0;
	_delay_loop_1(50);
	ENC28J60_Init(eth0.mac_addr);
	ENC28J60_ClkOut(2);
	_delay_loop_1(50);
	ENC28J60_PhyWrite(PHLCON,0x0476);
	_delay_loop_1(50);
}

ISR(PCINT0_vect)
{
	uint8_t eint = ENC28J60_Read(EIR);
	uint16_t plen;
//	uint16_t freespace, erxwrpt, erxrdpt, erxnd, erxst;
	net_wd = 0;

	if (eint == 0)
		return;
#if 0
	erxwrpt = ENC28J60_Read(ERXWRPTL);
	erxwrpt |= ENC28J60_Read(ERXWRPTH) << 8;

	erxrdpt = ENC28J60_Read(ERXRDPTL);
	erxrdpt |= ENC28J60_Read(ERXRDPTH) << 8;

	erxnd = ENC28J60_Read(ERXNDL);
	erxnd |= ENC28J60_Read(ERXNDH) << 8;

	erxst = ENC28J60_Read(ERXSTL);
	erxst |= ENC28J60_Read(ERXSTH) << 8;

	if (erxwrpt > erxrdpt) {
		freespace = (erxnd - erxst) - (erxwrpt - erxrdpt);
	} else if (erxwrpt == erxrdpt) {
		freespace = erxnd - erxst;
	} else {
		freespace = erxrdpt - erxwrpt - 1;
	}
	printf("int:0x%X freespace:%u\n", eint, freespace);
#endif
	if (eint & TXERIF) {
		ENC28J60_WriteOp(BFC, EIE, TXERIF);
	}

	if (eint & RXERIF) {
		ENC28J60_WriteOp(BFC, EIE, RXERIF);
	}

	if (!(eint & PKTIF)) {
		return;
	}

	plen = eth0.recv(&eth0.rx_buf);
	printf("len:%u\n", plen);
	if (plen == 0)
		return;
	eth_input(eth0.rx_buf, &eth0);
	buf_reset(&eth0.rx_buf);
	buf_reset(&eth0.tx_buf);
}

void tim_cb_wd(void *arg)
{
	tim_t *timer = arg;

#ifdef DEBUG
	puts("bip");
#endif
	if (net_wd > 0) {
#ifdef DEBUG
		puts("resetting net device");
#endif
		ENC28J60_reset();
		net_reset();
	}
	net_wd++;
	timer_reschedule(timer, 10000000UL);
}
#endif

int main(void)
{
#ifdef NET
	tim_t timer_wd;
#endif
	init_adc();

#ifdef DEBUG
	init_streams();
	printf_P(PSTR("KW alarm v0.2\n"));
#endif
	timer_subsystem_init(TIMER_RESOLUTION_US);
#ifdef RF
	if (rf_init() < 0) {
#ifdef DEBUG
		printf_P(PSTR("can't initialize RF\n"));
#endif
		return -1;
	}
#endif
#ifdef NET
	memset(&timer_wd, 0, sizeof(tim_t));
	timer_add(&timer_wd, 1000000UL, tim_cb_wd, &timer_wd);
	if_init(&eth0, NET_RX_SIZE, NET_TX_SIZE, &ENC28J60_PacketSend,
		&ENC28J60_PacketReceive);
	net_reset();
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT0);
	sei();
	while (1) {}
#endif
#ifdef RF
	while (1) {
		/* slow functions */
		decode_rf_cmds();

		/* XXX if omitted, the while() content does not get executed */
		delay_ms(100);
	}
	rf_shutdown();
#endif
	return 0;
}
