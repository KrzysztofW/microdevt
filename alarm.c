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
#define TIMER_RESOLUTION_US 150UL
#endif
#include "avr_utils.h"
#include "ring.h"
#include "timer.h"
#include "rf_commands.h"
#include "config.h"
#include "network.h"
#include "enc28j60.h"

ring_t *rf_ring;
#define RF_RING_SIZE 32

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

void initADC()
{
	/* Select Vref=AVcc */
	ADMUX |= (1<<REFS0);

	/* set prescaller to 128 and enable ADC */
	/* ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN); */

	/* set prescaller to 64 and enable ADC */
	ADCSRA |= (1<<ADPS1)|(1<<ADPS0);
}

uint16_t analogRead(uint8_t ADCchannel)
{
	/* enable ADC */
	ADCSRA |= (1<<ADEN);

	/* select ADC channel with safety mask */
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);

	/* single conversion mode */
	ADCSRA |= (1<<ADSC);

	// wait until ADC conversion is complete
	while (ADCSRA & (1<<ADSC));

	/* shutdown the ADC */
	ADCSRA &= ~(1<<ADEN);
	return ADC;
}

#if 0
#define START_FRAME = 0xAA

typedef struct pkt_header {
	unsigned char  start_frame; /* 0 */
	unsigned short from;	    /* 1 */
	unsigned char  len;	    /* 3 */
	unsigned short chksum;	    /* 4 */
	unsigned char  data[];      /* 6 */
} pkt_header_t;

static void pkt_parse(ring_t *ring)
{
	unsigned short addr;
	int len, chksum, chksum2 = 0, i;

	if (ring_is_empty(ring))
		return;

	if (ring_len(ring) < sizeof(pkt_head)) {
		return;
	}

	if (ring->data[ring->tail] != START_FRAME) {
		ring_reset(ring); // maybe skip 1?
		return;
	}

	/* check if we have enough data */
	len = ring->data[ring->tail + 3];
	if (len + sizeof(pkt_header_t) > MAX_SIZE) {
		ring_reset(ring);
		return;
	}
	if (ring_len(ring) < len + sizeof(pkt_head_t)) {
		/* not enough data */
		return;
	}

	addr = (unsigned short)ring->data[ring->tail + 1];
	chksum = (unsigned short)ring->data[ring->tail + 4];
	for (i = 0; i < len; i++) {
		chksum2 += ring->data[ring->tail + 6 + i];
	}
	if (checksum != chksum) {
		ring_skip(ring, len);
	}
}
#endif

#define LED PD4
void tim_cb_led(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= (1 << LED);
	timer_reschedule(timer, TIMER_RESOLUTION_US);
}

#define ANALOG_LOW_VAL 170
#define ANALOG_HI_VAL  690
static int started;

static void decode_rf_cmds(void)
{
	if (ring_len(rf_ring) < CMD_SIZE)
		return;

	ring_print(rf_ring);

	if (ring_cmp(rf_ring, remote1_btn1,
		     sizeof(remote1_btn1)) == 0) {
		PORTD |= (1 << LED);
	} else if (ring_cmp(rf_ring, remote1_btn2,
			    sizeof(remote1_btn2)) == 0) {
		PORTD &= ~(1 << LED);
	}
	ring_skip(rf_ring, CMD_SIZE);
	ring_cons_finish(rf_ring);
}

static void fill_cmd_ring(int bit, int count)
{
	if (started && count >= 1 && count <= 5) {
		if (started == 1 && bit == 0)
			goto error;

		if (ring_add_bit(rf_ring, bit) < 0)
			goto error;

		if (count >= 3 &&
		    ring_add_bit(rf_ring, bit) < 0)
			goto error;
		started = 2;
		return;
	}

	if (count >= 36 && count <= 40) {
		/* frame delimiter is only made of low values */
		if (bit)
			goto error;

		if (started) {
			if (ring_prod_len(rf_ring) == CMD_SIZE) {
				ring_prod_finish(rf_ring);
			} else
				goto error;
		}
		started = 1;
		return;
	}

 error:
	ring_prod_reset(rf_ring);
	ring_reset_byte(rf_ring);
	started = 0;
}

static inline void rf_sample(void)
{
	int v = analogRead(0);
	static int cnt;
	static int prev_val;

	if (v < ANALOG_LOW_VAL) {
		if (prev_val == 1) {
			prev_val = 0;
			fill_cmd_ring(1, cnt);
			cnt = 0;
		}
		cnt++;
	} else if (v > ANALOG_HI_VAL) {
		if (prev_val == 0) {
			prev_val = 1;
			fill_cmd_ring(0, cnt);
			cnt = 0;
		}
		cnt++;
	}
}

void tim_cb_rf(void *arg)
{
	tim_t *timer = arg;

	rf_sample();
	timer_reschedule(timer, TIMER_RESOLUTION_US);
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
	tim_t timer_rf;
#ifdef NET
	tim_t timer_wd;
#endif

	initADC();
	/* set PORTC (analog) for input */
	DDRC &= 0xFB; // pin 2 b1111 1011
	DDRC &= 0xFD; // pin 1 b1111 1101
	DDRC &= 0xFE; // pin 0 b1111 1110
	PORTC = 0xFF; // pullup resistors

#ifdef DEBUG
	init_streams();
	printf_P(PSTR("KW alarm v0.2\n"));
#endif
	timer_subsystem_init(TIMER_RESOLUTION_US);
	DDRD = (0x01 << LED); /* Configure the PORTD4 as output */

	if ((rf_ring = ring_create(RF_RING_SIZE)) == NULL) {
		printf_P(PSTR("can't create RF ring\n"));
		return -1;
	}

	memset(&timer_rf, 0, sizeof(tim_t));
	timer_add(&timer_rf, TIMER_RESOLUTION_US, tim_cb_rf, &timer_rf);
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
	free(rf_ring);

	return 0;
}
