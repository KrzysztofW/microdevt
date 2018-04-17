#include <stdint.h>
#include <_stdio.h>
#include <sys/buf.h>
#include <timer.h>
#include <usart.h>
#include "gsm.h"

typedef enum state {
	GSM_STATE_NONE,
	GSM_STATE_CMGF,
	GSM_STATE_CMGS,
	GSM_STATE_SMS,
	GSM_STATE_END,
} state_t;

static uint8_t state;
static tim_t timer;
static FILE *gsm_in, *gsm_out;

#define GSM_TIMEOUT 5000000 /* 5s */
#define GSM_ANS_LEN 24
static char gsm_resp_data[GSM_ANS_LEN];
static buf_t gsm_resp = BUF_INIT(gsm_resp_data, GSM_ANS_LEN);

#define GSM_SMS_MAX_LEN 32
static char gsm_sms_data[GSM_SMS_MAX_LEN];
static buf_t gsm_sms = BUF_INIT(gsm_sms_data, GSM_SMS_MAX_LEN);

#define GSM_SMS_DEST_NUMBER_LEN 12
static char gsm_dst_number_data[GSM_SMS_DEST_NUMBER_LEN];
static buf_t gsm_dst_number = BUF_INIT(gsm_dst_number_data,
				       GSM_SMS_DEST_NUMBER_LEN);

static sbuf_t ok = SBUF_INITS("OK");
static sbuf_t error = SBUF_INITS("ERROR");
static sbuf_t wait_for_more = SBUF_INITS(">");

void (*gsm_cb)(uint8_t status);

static void gsm_end(uint8_t status)
{
	state = GSM_STATE_NONE;
	if (timer_is_pending(&timer))
		timer_del(&timer);
	gsm_cb(status);
}

static void gsm_cmd_continue(void)
{
	switch (state) {
	case GSM_STATE_NONE:
		fprintf(gsm_out, "AT+CMGF=1\r");
		state = GSM_STATE_CMGF;
		break;
	case GSM_STATE_CMGF:
		fprintf(gsm_out, "AT+CMGS=\"%.*s\"\r", gsm_dst_number.len,
			gsm_dst_number.data);
		state = GSM_STATE_CMGS;
		break;
	case GSM_STATE_CMGS:
		fprintf(gsm_out, "%.*s\r", gsm_sms.len, gsm_sms.data);
		state = GSM_STATE_SMS;
		break;
	case GSM_STATE_SMS:
		usart1_put(0x1A);
		state = GSM_STATE_END;
		break;
	case GSM_STATE_END:
		gsm_end(GSM_STATUS_OK);
		break;
	}
}

static void gsm_handle_resp(void)
{
	sbuf_t buf = buf2sbuf(&gsm_resp);

	if (sbuf_cmp(&buf, &ok) == 0) {
		gsm_cmd_continue();
		return;
	}
	if (sbuf_cmp(&buf, &error) == 0) {
		DEBUG_LOG("GSM: failed (state: %d) ans: `%.*s'\n", state,
			  buf.len, buf.data);
		goto end;
	}
	if (sbuf_cmp(&buf, &wait_for_more) == 0 &&
	    state == GSM_STATE_CMGS) {
		gsm_cmd_continue();
		return;
	}
	DEBUG_LOG("GSM: unhandled answer: `%.*s'\n", buf.len, buf.data);
 end:
	gsm_end(GSM_STATUS_ERROR);
}

ISR(USART1_RX_vect)
{
	uint8_t c = UDR1;

	DEBUG_LOG("%c", c);
	if (c == '\r')
		gsm_handle_resp();
	else
		buf_addc(&gsm_resp, c);
}

static void timeout(void *arg)
{
	DEBUG_LOG("GSM: timeout\n");
	state = GSM_STATE_NONE;
	gsm_cb(GSM_STATUS_TIMEOUT);
}


int gsm_send_sms(const char *number, const char *sms)
{
	if (state != GSM_STATE_NONE) {
		DEBUG_LOG("GSM: busy\n");
		return -1;
	}

	buf_reset(&gsm_resp);
	buf_reset(&gsm_sms);
	if (buf_add(&gsm_sms, sms, strlen(sms)) < 0) {
		DEBUG_LOG("GSM: msg too long\n");
		return -1;
	}
	if (buf_add(&gsm_dst_number, number, strlen(number)) < 0) {
		DEBUG_LOG("GSM: dest number too long\n");
		return -1;
	}
	timer_add(&timer, GSM_TIMEOUT, timeout, NULL);
	gsm_cmd_continue();
	return 0;
}

void gsm_init(void (*cb)(uint8_t status))
{
	init_stream1(&gsm_in, &gsm_out, 1);
	timer_init(&timer);
	gsm_cb = cb;
}
