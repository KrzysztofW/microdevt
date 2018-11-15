#include <stdint.h>
#include <_stdio.h>
#include <sys/buf.h>
#include <sys/ring.h>
#include <timer.h>
#include <scheduler.h>
#include "gsm-at.h"

typedef enum state {
	GSM_STATE_NONE,
	GSM_STATE_READY,
	GSM_STATE_INIT,
	GSM_STATE_SENDING,
	GSM_STATE_RECEIVING,
} state_t;

static uint8_t state;
static tim_t timer;
static FILE *gsm_in, *gsm_out;

#define GSM_CMD_DELAY 1000000 /* 200ms */
#define GSM_SENDING_DELAY 5000000 /* 5s */

#define RING_SIZE 128
STATIC_RING_DECL(ring, RING_SIZE);
#ifdef DEBUG_GSM
STATIC_RING_DECL(ring_dbg, RING_SIZE);
#endif

static sbuf_t ok = SBUF_INITS("OK\n");
static sbuf_t error = SBUF_INITS("ERROR\n");
static sbuf_t wait = SBUF_INITS("> ");
static sbuf_t recv_start_pattern = SBUF_INITS("+CMT: \"");
static sbuf_t sbuf_null;

static const char *init_seq[] = {
	"", /* dummy entry */
	"AT\r",
	"ATE0\r",
	"AT+CMGF=1\r",
};
static uint8_t seq_pos;

static void (*gsm_cb)(uint8_t status, const sbuf_t *from, const sbuf_t *msg);

static void gsm_init_terminal(void *arg)
{
	state = GSM_STATE_INIT;

	if (seq_pos == 3) {
		ring_reset(ring);
		goto end;
	}

	/* send commands */
	if (seq_pos != sizeof(init_seq) / sizeof(char *))
		goto end;

	seq_pos = 0;
	while (ring_len(ring)) {
		if (ring_sbuf_cmp(ring, &ok) == 0) {
			ring_reset(ring);
			state = GSM_STATE_READY;
#ifdef DEBUG_GSM
			DEBUG_LOG("gsm terminal ready\n");
#endif
			gsm_cb(GSM_STATUS_READY, NULL, NULL);
			return;
		}
		/* recover from error */
		if (ring_sbuf_cmp(ring, &wait) == 0) {
#ifndef TEST
			fprintf(gsm_out, "%c", 0x1A);
#endif
			ring_reset(ring);
			goto end;
		}
		__ring_skip(ring, 1);
	}

 end:
#ifndef TEST
	fprintf(gsm_out, "%s", init_seq[seq_pos]);
#endif
	seq_pos++;
	timer_add(&timer, GSM_CMD_DELAY, gsm_init_terminal, NULL);
}

/* The received SMS is of the form: */
/* +CMT: "phone number","","18/04/08,17:05:43+08" */
/* <msg body> */
static void gsm_receive_msg(void *arg)
{
	buf_t buf;
	int rlen = ring_len(ring);
	sbuf_t tmp;

	buf = BUF(rlen);
	__ring_get(ring, &buf, rlen);
	state = GSM_STATE_READY;

	while (buf_get_sbuf_upto_sbuf_and_skip(&buf, &tmp,
					       &recv_start_pattern) >= 0) {
		sbuf_t from, msg;

		if (buf_get_sbuf_upto_and_skip(&buf, &from, "\"") < 0
		    || from.len == 0
		    || buf_get_sbuf_upto_and_skip(&buf, &tmp, "\n") < 0)
			continue;

		msg = buf2sbuf(&buf);

		/* check for more messages */
		buf_get_sbuf_upto_sbuf(&buf, &msg, &recv_start_pattern);
		if (msg.data[msg.len - 1] == '\n')
			msg.len--;

		gsm_cb(GSM_STATUS_RECV, &from, &msg);
	}
}

#ifdef DEBUG_GSM
static void gsm_dbg_cb(void *arg)
{
	uint8_t c;

	while (ring_getc(ring_dbg, &c) >= 0)
		DEBUG_LOG("%c", c);
}
#endif

static void gsm_schedule_recv_task(void *arg)
{
	schedule_task(gsm_receive_msg, NULL);
}

void gsm_handle_interrupt(uint8_t c)
{
	uint8_t b;
	if (state == GSM_STATE_NONE)
		return;
#ifdef DEBUG_GSM
	ring_addc(ring_dbg, c);
	schedule_task(gsm_dbg_cb, NULL);
#endif
	if (c == '\n') {
		if (ring_get_last_byte(ring, &b) < 0 || (b == '\n'))
			return;
	} else if (c == '\r') {
		if (state == GSM_STATE_READY) {
			state = GSM_STATE_RECEIVING;
			timer_add(&timer, GSM_CMD_DELAY, gsm_schedule_recv_task, NULL);
		}
		return;
	}
	ring_addc(ring, c);
}

static void gsm_send_sms_cb_ack(void *arg)
{
	while (ring_len(ring)) {
		if (ring_sbuf_cmp(ring, &error) == 0)
			goto error;

		if (ring_sbuf_cmp(ring, &ok) == 0) {
			__ring_skip(ring, ok.len);
			state = GSM_STATE_READY;
			gsm_cb(GSM_STATUS_OK, &sbuf_null, &sbuf_null);
			/* XXX check for received data if any */
			return;
		}
		if (ring_skip_upto(ring, '\n') < 0)
			goto error;
	}
 error:
	ring_reset(ring);
	gsm_init_terminal(NULL);
	gsm_cb(GSM_STATUS_ERROR, &sbuf_null, &sbuf_null);
}

static void gsm_send_sms_cb(void *arg)
{
	const char *sms = arg;

	if (ring_sbuf_cmp(ring, &wait) != 0) {
		gsm_init_terminal(NULL);
		gsm_cb(GSM_STATUS_ERROR, &sbuf_null, &sbuf_null);
		return;
	}

	timer_add(&timer, GSM_SENDING_DELAY, gsm_send_sms_cb_ack, (void *)sms);
	ring_reset(ring);
#ifndef TEST
	fprintf(gsm_out, "%s\r%c", sms, 0x1A);
#endif
}

int gsm_send_sms(const char *number, const char *sms)
{
	if (state != GSM_STATE_READY) {
		DEBUG_LOG("GSM: busy (state:%d)\n", state);
		return -1;
	}

	state = GSM_STATE_SENDING;
	timer_add(&timer, GSM_CMD_DELAY, gsm_send_sms_cb, (void *)sms);
	ring_reset(ring);
#ifndef TEST
	fprintf(gsm_out, "AT+CMGS=\"%s\"\r", number);
#endif
	return 0;
}

void
gsm_init(FILE *in, FILE *out, void (*cb)(uint8_t status, const sbuf_t *from,
					 const sbuf_t *msg))
{
	timer_init(&timer);
	gsm_in = in;
	gsm_out = out;
	gsm_cb = cb;
	gsm_init_terminal(NULL);
}

#ifdef TEST
static int test_data_pos;
static sbuf_t receive_sms[] = {
	SBUF_INITS(
		   "+CMT: \"+33612345678\",\"\",\"18/04/08,17:05:43+08\"\r"
		   "\n\r"
		   "message 1\r"),

	SBUF_INITS("+CMT: \"+33612345671\",\"\",\"18/04/08,17:05:43+08\"\n\r"
		   "message 2\n\r"
		   "fd\n\r"
		   "\n\r"
		   "+CMT: \"+33612345671\",\"\",\"18/04/08,17:05:43+08\"\n\r"
		   "message 2\n\r"
		   "fd\n\r"
		   "\r"),
	SBUF_INITS("+CMT: \"+33612345672\",\"\",\"18/04/08,17:05:43+08\"\n\r"
		   "message 3\n\r"
		   "fd\n\r"
		   "\n\r"
		   "+CMT: \"\",\"\",\"18/04/08,17:05:43+08\"\n\r"
		   "message 3 (ko)\n\r"
		   "fd2\n\r"
		   "\r"),
	SBUF_INITS("+CMT: \"\",\"\",\"18/04/08,17:05:43+08\"\n\r"
		   "message 4 (ko)\n\r"
		   "fd\n\r"
		   "\n\r"
		   "+CMT: \"+33612345672\",\"\",\"18/04/08,17:05:43+08\"\n\r"
		   "message 4\n\r"
		   "fd2\n"),
};
static sbuf_t expected_nbs[] = {
	SBUF_INITS("+33612345678"),
	SBUF_INITS("+33612345671"),
	SBUF_INITS("+33612345672"),
	SBUF_INITS("+33612345672"),
};
static sbuf_t expected_msgs[] = {
	SBUF_INITS("message 1"),
	SBUF_INITS("message 2\nfd"),
	SBUF_INITS("message 3\nfd"),
	SBUF_INITS("message 4\nfd2"),
};

static gsm_status_t test_cb_status = GSM_STATUS_ERROR;
static void test_cb(uint8_t status, const sbuf_t *from, const sbuf_t *msg)
{
	if (status == GSM_STATUS_RECV) {
		if (sbuf_cmp(&expected_nbs[test_data_pos], from) != 0) {
			fprintf(stderr, "%s: got unexpected number: `%.*s' "
				"(expected: `%.*s')\n",
				__func__, from->len, from->data,
				expected_nbs[test_data_pos].len,
				expected_nbs[test_data_pos].data);
			exit(-1);
		}
		if (sbuf_cmp(&expected_msgs[test_data_pos], msg) != 0) {
			fprintf(stderr, "%s: got unexpected msg: `%.*s' "
				"(expected: `%.*s')\n",
				__func__, msg->len, msg->data,
				expected_msgs[test_data_pos].len,
				expected_msgs[test_data_pos].data);
			exit(-1);
		}

	}
	test_cb_status = status;
}

static void gsm_tests_send_str(const sbuf_t *sbuf)
{
	int i;

	for (i = 0; i < sbuf->len; i++)
		gsm_handle_interrupt(sbuf->data[i]);
}

static int gsm_tests_send_sms(void)
{
	const char *sms = "SMS from KW alarm";

	if (state != GSM_STATE_READY) {
		fprintf(stderr, "%s:%d not ready (state:%d)\n", __func__,
			__LINE__, state);
		return -1;
	}
	if (gsm_send_sms("+33612345678", sms) < 0) {
		fprintf(stderr, "%s:%d sending sms failed\n", __func__,
			__LINE__);
		return -1;
	}
	timer_del(&timer);
	if (state != GSM_STATE_SENDING) {
		fprintf(stderr, "%s:%d not sending (state:%d)\n", __func__,
			__LINE__, state);
		return -1;
	}
	gsm_tests_send_str(&wait);
	gsm_send_sms_cb((char *)sms);
	timer_del(&timer);

	gsm_tests_send_str(&ok);
	gsm_send_sms_cb_ack(NULL);

	if (test_cb_status != GSM_STATUS_OK) {
		fprintf(stderr, "%s:%d returned with status :%d\n",
			__func__, __LINE__, test_cb_status);
		return -1;
	}
	return 0;
}

static int gsm_tests_receive_sms(void)
{
	for (test_data_pos = 0;
	     test_data_pos < sizeof(receive_sms) / sizeof(sbuf_t);
	     test_data_pos++) {
		gsm_tests_send_str(&receive_sms[test_data_pos]);
		if (state != GSM_STATE_RECEIVING) {
			fprintf(stderr, "%s:%d not receiving (state:%d)\n",
				__func__, __LINE__, state);
			return -1;
		}
		timer_del(&timer);
		gsm_receive_msg(NULL);
		if (state != GSM_STATE_READY) {
			fprintf(stderr, "%s:%d not ready (state:%d)\n",
				__func__, __LINE__, state);
			return -1;
		}
		if (test_cb_status != GSM_STATUS_RECV) {
			fprintf(stderr, "%s:%d returned with status :%d\n",
				__func__, __LINE__, test_cb_status);
			return -1;
		}
	}
	return 0;
}

static int gsm_tests_check_init(void)
{
	int nb = sizeof(init_seq) / sizeof(char *);

	while (nb) {
		gsm_tests_send_str(&ok);
		gsm_init_terminal(NULL);
		timer_del(&timer);
		nb--;
	}
	if (state != GSM_STATE_READY)
		return -1;
	return 0;
}

int gsm_tests(void)
{
	int i, nb = 4;

	gsm_init(stdin, stdout, test_cb);
	timer_del(&timer);

	if (gsm_tests_check_init() < 0)
		return -1;

	for (i = 0; i < nb; i++) {
		if (gsm_tests_send_sms() < 0)
			return -1;
	}

	for (i = 0; i < nb; i++) {
		if (gsm_tests_receive_sms() < 0)
			return -1;
	}

	for (i = 0; i < nb; i++) {
		if (gsm_tests_send_sms() < 0)
			return -1;
		if (gsm_tests_receive_sms() < 0)
			return -1;
	}

	return 0;
}

#endif
