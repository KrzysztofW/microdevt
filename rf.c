#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <sys/ring.h>
#include <timer.h>
#include <sys/byte.h>
#include <sys/chksum.h>
#include "rf.h"
#include "rf_cfg.h"
#include "rf_commands.h"
#include "adc.h"

#define RF_SAMPLING_US 150
#define RF_RING_SIZE    64

#define ANALOG_LOW_VAL 170
#define ANALOG_HI_VAL  650

#if RF_SAMPLING_US < CONFIG_TIMER_RESOLUTION_US
#error "RF sampling will not work with the current timer resolution"
#endif

struct rf_data {
	ring_t *ring;
	byte_t byte;
} __attribute__((__packed__));
typedef struct rf_data rf_data_t;

#ifdef CONFIG_RF_RECEIVER
#ifndef RF_ANALOG_PIN
#error "RF_ANALOG_PIN not defined"
#endif
static tim_t rf_rcv_timer;
static rf_data_t rcv_buf;
static volatile uint8_t receiving;
#endif

#ifndef RF_HW_ADDR
#error "RF_HW_ADDR not defined"
#endif

#ifdef CONFIG_RF_SENDER
#define FRAME_DELIMITER_LENGTH 30 /* loops */
#define START_FRAME_LENGTH 30	  /* loops */

static tim_t rf_snd_timer;
#if !defined(RF_SND_PIN) || !defined(RF_SND_PORT)
#error "RF_SND_PIN and RF_SND_PORT are not defined"
#endif
rf_data_t snd_buf;
static volatile uint8_t sending;
#endif

/* XXX the size of this structure has to be even (in order to use
 * cksum_partial() on it). */
struct rf_pkt {
	uint8_t to;
	uint8_t from;
	uint16_t len;
	uint16_t chksum;
} __attribute__((__packed__));
typedef struct rf_pkt rf_pkt_t;

#ifdef CONFIG_RF_RECEIVER
static int rf_ring_add_bit(rf_data_t *buf, uint8_t bit)
{
	int byte = byte_add_bit(&buf->byte, bit);
	int ret;

	if (byte < 0)
		return 0;

	ret = ring_addc(buf->ring, byte);
	return ret;
}

static int rf_get_rcv_len(void)
{
	int len;

	//disable_timer_int();
	len = ring_len(rcv_buf.ring);
	//enable_timer_int();
	return len;
}

static void rf_skip_rcv_size(uint8_t size)
{
	disable_timer_int();
	__ring_skip(rcv_buf.ring, size);
	enable_timer_int();
}

static void rf_fill_cmd_ring(uint8_t bit, uint16_t count)
{
	if (receiving && count < 11) {
		if (receiving == 1) {
			if (bit == 0)
				goto end;
			receiving = 2;
		}

		if (count > 3) {
			if (rf_ring_add_bit(&rcv_buf, 1) < 0)
				goto end;
		} else {
			if (rf_ring_add_bit(&rcv_buf, 0) < 0) {
				goto end;
			}
		}
		return;
	}

	if (count >= 55 && count <= 62) {
		/* frame delimiter is only made of low values */
		if (bit)
			goto end;

		byte_reset(&rcv_buf.byte);
		receiving = 1;
		return;
	}
 end:
	if (receiving)
		receiving = 0;
	byte_reset(&rcv_buf.byte);
}

static inline void rf_sample(void)
{
	unsigned v = analog_read(RF_ANALOG_PIN);
	static uint16_t cnt;
	static uint8_t prev_val;

	PORTD ^= (1 << PD2);
	if (v < ANALOG_LOW_VAL) {
		if (prev_val == 1) {
			prev_val = 0;
			rf_fill_cmd_ring(1, cnt);
			cnt = 0;
		}
		cnt++;
	} else if (v > ANALOG_HI_VAL) {
		if (prev_val == 0) {
			prev_val = 1;
			rf_fill_cmd_ring(0, cnt);
			cnt = 0;
		}
		cnt++;
	}
}
#endif

#ifdef CONFIG_RF_SENDER
/* returns -1 if no data sent */
static int rf_snd(void)
{
	static uint8_t bit;
	static uint8_t frame_delim_started;
	static uint8_t clk;

	if (sending > 1) {
		sending--;
		return 0;
	}

	if (frame_delim_started < START_FRAME_LENGTH) {
		RF_SND_PORT ^= 1 << RF_SND_PIN;
		frame_delim_started++;
		return 0;
	}

	if (frame_delim_started == START_FRAME_LENGTH) {
		RF_SND_PORT &= ~(1 << RF_SND_PIN);
		frame_delim_started++;
		return 0;
	}
	if (frame_delim_started < FRAME_DELIMITER_LENGTH + START_FRAME_LENGTH) {
		frame_delim_started++;
		return 0;
	}
	if (frame_delim_started == FRAME_DELIMITER_LENGTH + START_FRAME_LENGTH) {
		frame_delim_started++;
		return 0;
	}

	if (bit) {
		bit--;
		return 0;
	}

	if (byte_is_empty(&snd_buf.byte)) {
		uint8_t c;

		if (ring_getc(snd_buf.ring, &c) < 0) {
			if (sending) {
				if (RF_SND_PORT & (1 << RF_SND_PIN)) {
					RF_SND_PORT &= ~(1 << RF_SND_PIN);
					frame_delim_started = 0;
					sending = 0;
					bit = 0;
					return -1;
				} else {
					RF_SND_PORT |= 1 << RF_SND_PIN;
					return 0;
				}
			}
			return -1;
		}
		byte_init(&snd_buf.byte, c);
	}

	if (sending == 0) {
		clk = 1;
		sending = 1;
	} else
		clk ^= 1;

	bit = byte_get_bit(&snd_buf.byte);
	if (bit)
		bit = 2;

	/* send bit */
	if (clk)
		RF_SND_PORT |= 1 << RF_SND_PIN;
	else
		RF_SND_PORT &= ~(1 << RF_SND_PIN);

	return 0;
}

static void rf_snd_tim_cb(void *arg);

void rf_start_sending(void)
{
#ifdef CONFIG_RF_RECEIVER
	timer_del(&rf_rcv_timer);
#endif
	if (!timer_is_pending(&rf_snd_timer))
		timer_add(&rf_snd_timer, RF_SAMPLING_US * 2, rf_snd_tim_cb,
			  &rf_snd_timer);
}

#if 0 /* for debug purposes */
int rf_send_raw_data(const buf_t *data)
{
	if (ring_addbuf(snd_buf.ring, data) < 0)
		return -1;
	return 0;
}

int rf_send_raw_byte(uint8_t byte)
{
	if (ring_addc(snd_buf.ring, byte) < 0)
		return -1;
	return 0;
}
#endif

int rf_sendto(uint8_t to, const sbuf_t *data)
{
	rf_pkt_t pkt;
	uint32_t csum;
	buf_t buf;

	if (sending
	    || ring_free_entries(snd_buf.ring) < data->len + sizeof(pkt))
		return -1;

	pkt.from = RF_HW_ADDR;
	pkt.to = to;
	pkt.len = htons(data->len);
	pkt.chksum = 0;

	STATIC_ASSERT(!(sizeof(rf_pkt_t) & 0x1));
	csum = cksum_partial(&pkt, sizeof(rf_pkt_t));
	csum += cksum_partial(data->data, data->len);
	pkt.chksum = cksum_finish(csum);

	buf_init(&buf, (void *)data->data, data->len);
	__ring_addbuf(snd_buf.ring, &buf);

	buf = sbuf2buf(data);
	__ring_addbuf(snd_buf.ring, &buf);

	rf_start_sending();
	return 0;
}

#endif

#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg)
{
	timer_reschedule(&rf_snd_timer, RF_SAMPLING_US * 2);

	if (rf_snd() < 0) {
#ifdef CONFIG_RF_RECEIVER
		/* enable receiving */
		timer_reschedule(&rf_rcv_timer, RF_SAMPLING_US);
#endif
		timer_del(&rf_snd_timer);
		return;
	}
}
#endif

#ifdef CONFIG_RF_RECEIVER
static void rf_rcv_tim_cb(void *arg)
{
	timer_reschedule(&rf_rcv_timer, RF_SAMPLING_US);
	rf_sample();
}
#endif

#ifdef CONFIG_RF_RECEIVER

#ifdef CONFIG_RF_CMDS
#ifdef CONFIG_RF_KERUI_CMDS

void (*rf_kerui_cb)(int nb);

void rf_set_kerui_cb(void (*cb)(int nb))
{
	rf_kerui_cb = cb;
}

static int rf_parse_kerui_cmds(int rlen)
{
	int i;

	if (rlen < RF_KE_CMD_SIZE)
		return -1;
	for (i = 0; i < sizeof(rf_ke_cmds) / RF_KE_CMD_SIZE; i++) {
		if (ring_cmp(rcv_buf.ring, rf_ke_cmds[i],
			     RF_KE_CMD_SIZE) == 0) {
			rf_skip_rcv_size(RF_KE_CMD_SIZE);
			rf_kerui_cb(i);
			return 0;
		}
	}
	return -1;
}
#endif
static void rf_parse_cmds(int rlen)
{
	/* vendor specific parsers go here */
#ifdef CONFIG_RF_KERUI_CMDS
	if (rf_parse_kerui_cmds(rlen) >= 0)
		return;
#endif
	rf_skip_rcv_size(1); /* skip 1 byte of garbage */
}
#endif
int rf_recvfrom(uint8_t *from, buf_t *buf)
{
	uint8_t my_addr = RF_HW_ADDR;
	uint8_t rlen;
	static uint8_t from_addr;
	static int valid_data_left;

	while (receiving != 1 && (rlen = rf_get_rcv_len())) {
		if (valid_data_left) {
			int l = __ring_get_dont_skip(rcv_buf.ring, buf,
						     valid_data_left);
			valid_data_left -= l;
			rf_skip_rcv_size(l);
			*from = from_addr;
			return 0;
		}

		/* we got the header but wait if we are still receiving */
		if (rlen < sizeof(rf_pkt_t) || receiving)
			return -1;

		/* check if data is for us */
		if (ring_cmp(rcv_buf.ring, &my_addr, 1) == 0) {
			rf_pkt_t pkt;
			uint16_t pkt_len;
			buf_t b;
			unsigned l;

			buf_init(&b, &pkt, sizeof(rf_pkt_t));
			b.len = 0;
			__ring_get_dont_skip(rcv_buf.ring, &b, sizeof(rf_pkt_t));
			pkt_len = ntohs(pkt.len);
			l = pkt_len + sizeof(rf_pkt_t);

			if (rlen < l || __ring_cksum(rcv_buf.ring, l) != 0) {
				rf_skip_rcv_size(1);
				continue;
			}
			*from = pkt.from;
			rf_skip_rcv_size(sizeof(rf_pkt_t));
			l = __ring_get_dont_skip(rcv_buf.ring, buf, pkt_len);
			valid_data_left = pkt_len - l;
			rf_skip_rcv_size(l);
			return 0;
		}
#ifdef CONFIG_RF_CMDS
		rf_parse_cmds(rlen);
#else
		/* skip 1 byte of garbage */
		rf_skip_rcv_size(1);
#endif
	}
	return -1;
}
#endif

int rf_init(void)
{
#ifdef CONFIG_RF_RECEIVER
	timer_add(&rf_rcv_timer, RF_SAMPLING_US, rf_rcv_tim_cb, &rf_rcv_timer);
	if ((rcv_buf.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("can't create receive RF ring\n");
		return -1;
	}
#endif
#ifdef CONFIG_RF_SENDER
	if ((snd_buf.ring = ring_create(RF_RING_SIZE)) == NULL) {
		DEBUG_LOG("can't create send RF ring\n");
		return -1;
	}
#endif
	return 0;
}

#if 0 /* Will never be used in an infinite loop. Save space, don't compile it */
void rf_shutdown(void)
{
#ifdef CONFIG_RF_RECEIVER
	timer_del(&rf_rcv_timer);
	free(rcv_buf.ring);
#endif
#ifdef CONFIG_RF_SENDER
	timer_del(&rf_snd_timer);
	free(snd_buf.ring);
#endif
}
#endif
