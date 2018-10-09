#include <avr/interrupt.h>
#include <_stdio.h>
#include <sys/buf.h>
#include <timer.h>
#include <usart.h>
#include <drivers/gsm-at.h>

static FILE *gsm_in, *gsm_out;
static tim_t timer;

ISR(USART1_RX_vect)
{
	gsm_handle_interrupt(UDR1);
}

static void gsm_cb(uint8_t status, const sbuf_t *from, const sbuf_t *msg)
{
	DEBUG_LOG("gsm callback (status:%d)\n", status);

	switch (status) {
	case GSM_STATUS_RECV:
		DEBUG_LOG("from:");
		sbuf_print(from);
		DEBUG_LOG(" msg:<");
		sbuf_print(msg);
		DEBUG_LOG(">\n");
		break;
	case GSM_STATUS_READY:
	case GSM_STATUS_TIMEOUT:
	case GSM_STATUS_ERROR:
		break;
	}
}

static void gsm_tim_cb(void *arg)
{
	gsm_send_sms("+33687236420", "SMS from KW alarm");
}

void alarm_gsm_init(void)
{
	init_stream1(&gsm_in, &gsm_out, 1);
	gsm_init(gsm_in, gsm_out, gsm_cb);
	timer_init(&timer);
	timer_add(&timer, 30000000, gsm_tim_cb, NULL);
}
