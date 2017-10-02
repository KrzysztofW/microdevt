#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "rf.h"
#include "sys/ring.h"
#include "timer.h"
#include "rf_commands.h"
#include "adc.h"

static tim_t timer_rf;

ring_t *rf_ring;
#define RF_RING_SIZE 32
#define LED PD4

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

static void tim_cb_led(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= (1 << LED);
	timer_reschedule(timer, TIMER_RESOLUTION_US);
}
#endif

#define ANALOG_LOW_VAL 170
#define ANALOG_HI_VAL  690

void rf_decode_cmds(void)
{
	if (ring_len(rf_ring) < CMD_SIZE)
		return;

#ifdef DEBUG
	ring_print_limit(rf_ring, CMD_SIZE);
#endif
	if (ring_cmp(rf_ring, remote1_btn1,
		     sizeof(remote1_btn1)) == 0) {
		PORTD |= (1 << LED);
	} else if (ring_cmp(rf_ring, remote1_btn2,
			    sizeof(remote1_btn2)) == 0) {
		PORTD &= ~(1 << LED);
	}
	ring_skip(rf_ring, CMD_SIZE);
}

static void fill_cmd_ring(int bit, int count)
{
	static int started;

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

		if (started && (ring_len(rf_ring) % CMD_SIZE) != 0)
			goto error;
		started = 1;
		return;
	}

 error:
	ring_reset(rf_ring);
	ring_reset_byte(rf_ring);
	started = 0;
}

static inline void rf_sample(void)
{
	int v = analog_read(0);
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

static void tim_cb_rf(void *arg)
{
	tim_t *timer = arg;

	rf_sample();
	timer_reschedule(timer, TIMER_RESOLUTION_US);
}

int rf_init(void)
{
	DDRD = (0x01 << LED); /* Configure the PORTD4 as output */
	if ((rf_ring = ring_create(RF_RING_SIZE)) == NULL) {
#ifdef DEBUG
		printf_P(PSTR("can't create RF ring\n"));
#endif
		return -1;
	}

	memset(&timer_rf, 0, sizeof(tim_t));
	timer_add(&timer_rf, TIMER_RESOLUTION_US, tim_cb_rf, &timer_rf);
	return 0;
}

void rf_shutdown(void)
{
	free(rf_ring);
}
