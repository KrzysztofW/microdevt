#include <avr/io.h>
#include <stdio.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>

#define DEBUG
#ifdef DEBUG
//#define NET
#include "usart0.h"
#define SERIAL_SPEED 57600
#define SYSTEM_CLOCK F_CPU
#endif
#include "avr_utils.h"
#include "ring.h"
#include "timer.h"
#include "rf.h"
#include "adc.h"
#include "config.h"
#include "network.h"
#include "enc28j60.h"

#ifdef NET
int net_wd;
#define NET_TX_SIZE  512
#define NET_RX_SIZE  256
#define NET_MAX_NB_PKTS 10

struct net_buf {
	ring_t *tx;
	ring_t *txaddr;
	ring_t *rx;
	ring_t *rxaddr;
} net_buf;
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
#endif

void init_streams()
{
	/* initialize the standard streams to the user defined one */
	stdout = &my_stream;
	stdin = &my_stream;
	usart0_init(BAUD_RATE(SYSTEM_CLOCK, SERIAL_SPEED));
}

#ifdef NET
static uint8_t mymac[6] = {0x62,0x5F,0x70,0x72,0x61,0x79};
static uint8_t myip[4] = {192,168,0,99};
static uint16_t mywwwport = 80;

#define BUFFER_SIZE 900UL
uint8_t buf[BUFFER_SIZE + 1],browser;
uint16_t plen;

void testpage(void) {
	plen=make_tcp_data_pos(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: "
					  "text/html\r\n\r\n"));
	plen=make_tcp_data_pos(buf,plen,PSTR("<html><body><h1>It Works!"
					     "</h1></body></html>"));
}

void sendpage(void) {
	tcp_ack(buf);
	tcp_ack_with_data(buf,plen);
}

static void net_reset(void)
{
	CLKPR = (1<<CLKPCE);
	CLKPR = 0;
	_delay_loop_1(50);
	ENC28J60_Init(mymac);
	ENC28J60_ClkOut(2);
	_delay_loop_1(50);
	ENC28J60_PhyWrite(PHLCON,0x0476);
	_delay_loop_1(50);
}

ISR(PCINT0_vect)
{
	uint8_t eint = ENC28J60_Read(EIR);
	uint16_t dat_p;
	uint16_t freespace, erxwrpt, erxrdpt, erxnd, erxst;
	net_wd = 0;

	if (eint == 0)
		return;

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
	if (eint & TXERIF) {
		ENC28J60_WriteOp(BFC, EIE, TXERIF);
	}

	if (eint & RXERIF) {
		ENC28J60_WriteOp(BFC, EIE, RXERIF);
	}

	if (!(eint & PKTIF)) {
		return;
	}

	plen = ENC28J60_PacketReceive(BUFFER_SIZE,buf);
	if (plen == 0)
		return;

	if(eth_is_arp(buf,plen)) {
		arp_reply(buf);
		return;
	}

	if(eth_is_ip(buf,plen)==0) return;
	if(buf[IP_PROTO]==IP_ICMP && buf[ICMP_TYPE]==ICMP_REQUEST) {
		icmp_reply(buf,plen);
		return;
	}
	if (buf[IP_PROTO]==IP_TCP && buf[TCP_DST_PORT]==0
	   && buf[TCP_DST_PORT+1]==mywwwport) {
		if(buf[TCP_FLAGS] & TCP_SYN) {
			tcp_synack(buf);
			net_reset();
			return;
		}
		if(buf[TCP_FLAGS] & TCP_ACK) {
			init_len_info(buf);
			dat_p = get_tcp_data_ptr();
			if(dat_p==0) {
				if(buf[TCP_FLAGS] & TCP_FIN) tcp_ack(buf);
				net_reset();
				return;
			}

			if(strstr((char*)&(buf[dat_p]),"User Agent")) browser=0;
			else if(strstr((char*)&(buf[dat_p]),"MSIE")) browser=1;
			else browser=2;

			if(strncmp("/ ",(char*)&(buf[dat_p+4]),2)==0){
				testpage();
				sendpage();
				net_reset();
				return;
			}
		}
	}
}

void tim_cb_wd(void *arg)
{
	tim_t *timer = arg;

	puts("bip");
	if (net_wd > 0) {
		puts("resetting net device");
		ENC28J60_reset();
		net_reset();
		init_network(mymac,myip,mywwwport);

	}
	net_wd++;
	timer_reschedule(timer, 10000000UL);
}

static int init_trx_buffers(void)
{
	if ((net_buf.tx = ring_create(NET_TX_SIZE)) == NULL) {
		printf_P(PSTR("can't create TX ring\n"));
		return -1;
	}
	if ((net_buf.txaddr = ring_create(NET_MAX_NB_PKTS)) == NULL) {
		printf_P(PSTR("can't create TX addr ring\n"));
		return -1;
	}
	if ((net_buf.rx = ring_create(NET_RX_SIZE)) == NULL) {
		printf_P(PSTR("can't create RX ring\n"));
		return -1;
	}
	if ((net_buf.rxaddr = ring_create(NET_MAX_NB_PKTS)) == NULL) {
		printf_P(PSTR("can't create RX addr ring\n"));
		return -1;
	}
	return 0;
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
	if (rf_init() < 0) {
		printf_P(PSTR("can't initialize RF\n"));
		return -1;
	}

#ifdef NET
	memset(&timer_wd, 0, sizeof(tim_t));
	timer_add(&timer_wd, 1000000UL, tim_cb_wd, &timer_wd);

	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT0);

	sei();
	net_reset();
	init_network(mymac,myip,mywwwport);

	if (init_trx_buffers() < 0) {
		printf_P(PSTR("can't initialize network buffers\n"));
		return -1;
	}
#endif
	while (1) {
		/* slow functions */
		decode_rf_cmds();

		/* XXX if omitted, the while() content does not get executed */
		delay_ms(100);
	}
	rf_shutdown();

	return 0;
}
