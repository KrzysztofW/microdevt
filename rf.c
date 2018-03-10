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

#ifndef RF_HW_ADDR
#error "RF_HW_ADDR not defined"
#endif

#ifdef CONFIG_RF_RECEIVER
#ifndef RF_RCV_PIN_NB
#error "RF_RCV_PIN_NB not defined"
#endif
static tim_t rf_rcv_timer;
static rf_data_t rcv_buf;
static uint8_t my_addr = RF_HW_ADDR;
#endif

#ifdef CONFIG_RF_SENDER
#define FRAME_DELIMITER_LENGTH 30 /* loops */
#define START_FRAME_LENGTH 30	  /* loops */

static tim_t rf_snd_timer;
#if !defined(RF_SND_PIN_NB) || !defined(RF_SND_PORT)
#error "RF_SND_PIN and RF_SND_PORT are not defined"
#endif
rf_data_t snd_buf;
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

static void rf_fill_cmd_ring(uint8_t bit, uint16_t count)
{
	static uint8_t receiving;

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
	static uint16_t cnt;
	static uint8_t prev_val;
	uint8_t v, not_v;

#ifdef RF_ANALOG_SAMPLING
	v = analog_read(RF_RCV_PIN_NB);
	if (v < ANALOG_LOW_VAL)
		v = 0;
	else if (v > ANALOG_HI_VAL)
		v = 1;
	else
		return;
#else
	v = RF_RCV_PIN & (1 << RF_RCV_PIN_NB);
#endif
	not_v = !v;
	if (prev_val == not_v) {
		prev_val = v;
		rf_fill_cmd_ring(not_v, cnt);
		cnt = 0;
	} else
		cnt++;
}
#endif

#ifdef CONFIG_RF_SENDER
/* returns -1 if no data sent */
static int rf_snd(void)
{
	static uint16_t frame_length;
	static uint8_t bit;
	static uint8_t clk;
	static uint8_t frame_delim_started;
	static uint8_t sending;

	if (bit) {
		bit--;
		return 0;
	}

	if (frame_delim_started < START_FRAME_LENGTH) {
		/* send garbage data to calibrate the RF sender */
		RF_SND_PORT ^= 1 << RF_SND_PIN_NB;
		frame_delim_started++;
		return 0;
	}

	if (frame_delim_started == START_FRAME_LENGTH) {
		uint8_t fl;

		frame_length = 0;
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);

		/* get frame length */
		if (ring_getc(snd_buf.ring, (uint8_t *)&frame_length) < 0 ||
		    ring_getc(snd_buf.ring, &fl) < 0)
			goto end;

		frame_length |= fl << 8;

		frame_delim_started++;
		return 0;
	}
	if (frame_delim_started <= FRAME_DELIMITER_LENGTH + START_FRAME_LENGTH) {
		frame_delim_started++;
		return 0;
	}

	if (byte_is_empty(&snd_buf.byte)) {
		uint8_t c;

		if (sending && frame_length == 0 && ring_len(snd_buf.ring)) {
			assert(byte_is_empty(&snd_buf.byte));
			if (RF_SND_PORT & (1 << RF_SND_PIN_NB)) {
				sending = 0;
				frame_delim_started = START_FRAME_LENGTH;
			} else
				RF_SND_PORT |= 1 << RF_SND_PIN_NB;
			return 0;
		}

		if (ring_getc(snd_buf.ring, &c) < 0) {
			if (!sending)
				return -1;
			if (RF_SND_PORT & (1 << RF_SND_PIN_NB))
				goto end;
			RF_SND_PORT |= 1 << RF_SND_PIN_NB;
			return 0;
		}

		frame_length--;
		byte_init(&snd_buf.byte, c);
	}

	/* use 2 loops for '1' value */
	bit = byte_get_bit(&snd_buf.byte) ? 2 : 0;

	if (sending == 0) {
		clk = 1;
		sending = 1;
	} else
		clk ^= 1;

	/* send bit */
	if (clk)
		RF_SND_PORT |= 1 << RF_SND_PIN_NB;
	else
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	return 0;
 end:
	RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	frame_delim_started = 0;
	sending = 0;
	bit = 0;
	return -1;
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

int rf_sendto(uint8_t to, const buf_t *data, int8_t burst)
{
	rf_pkt_t pkt;
	uint32_t csum;
	buf_t pkt_buf;
	uint16_t frame_len;
	ring_t ring;

	STATIC_IF(sizeof(ring.head) > sizeof(uint8_t),
		  if (timer_is_pending(&rf_snd_timer)) return -1,
		  (void)0);

	if (burst == 0)
		burst = 1;
	if (ring_free_entries(snd_buf.ring) < (data->len + sizeof(pkt) + 2) * burst)
		return -1;

	pkt.from = RF_HW_ADDR;
	pkt.to = to;
	pkt.len = htons(data->len);
	pkt.chksum = 0;

	STATIC_ASSERT(!(sizeof(rf_pkt_t) & 0x1));
	csum = cksum_partial(&pkt, sizeof(rf_pkt_t));
	csum += cksum_partial(data->data, data->len);
	pkt.chksum = cksum_finish(csum);
	buf_init(&pkt_buf, (void *)&pkt, sizeof(pkt));

	frame_len = data->len + sizeof(rf_pkt_t);
	/* send the same command 'burst' times as RF is not very reliable */
	do {
		/* add frame length */
		__ring_addc(snd_buf.ring, frame_len & 0xFF);
		__ring_addc(snd_buf.ring, (frame_len >> 8) & 0xFF);
		/* add the buffer */
		__ring_addbuf(snd_buf.ring, &pkt_buf);
		__ring_addbuf(snd_buf.ring, data);
	} while (--burst > 0);

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
		return 0;
	for (i = 0; i < sizeof(rf_ke_cmds) / RF_KE_CMD_SIZE; i++) {
		if (ring_cmp(rcv_buf.ring, rf_ke_cmds[i],
			     RF_KE_CMD_SIZE) == 0) {
			__ring_skip(rcv_buf.ring, RF_KE_CMD_SIZE);
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
	/* skip 1 byte of garbage */
	__ring_skip(rcv_buf.ring, 1);
}
#endif
int rf_recvfrom(uint8_t *from, buf_t *buf)
{
	uint8_t rlen;
	static uint8_t from_addr;
	static int valid_data_left;

	while ((rlen = ring_len(rcv_buf.ring))) {
		if (valid_data_left) {
			int l = __ring_get_dont_skip(rcv_buf.ring, buf,
						     valid_data_left);
			valid_data_left -= l;
			__ring_skip(rcv_buf.ring, l);
			*from = from_addr;
			return 0;
		}

                if (rlen < sizeof(rf_pkt_t))
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

			if (l > rcv_buf.ring->mask) {
				__ring_skip(rcv_buf.ring, 1);
				continue;
			}
			if (rlen < l)
				return -1;
			if (__ring_cksum(rcv_buf.ring, l) != 0) {
				__ring_skip(rcv_buf.ring, 1);
				continue;
			}
			*from = pkt.from;
			__ring_skip(rcv_buf.ring, sizeof(rf_pkt_t));
			l = __ring_get_dont_skip(rcv_buf.ring, buf, pkt_len);
			valid_data_left = pkt_len - l;
			__ring_skip(rcv_buf.ring, l);
			return 0;
		}
#ifdef CONFIG_RF_CMDS
		rf_parse_cmds(rlen);
#else
		/* skip 1 byte of garbage */
		__ring_skip(rcv_buf.ring, 1);
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
#ifndef RF_ANALOG_SAMPLING
	RF_RCV_PORT &= ~(1 << RF_RCV_PIN_NB);
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
