#ifndef _GSM_AT_H_
#define _GSM_AT_H_

typedef enum gsm_status {
	GSM_STATUS_OK,
	GSM_STATUS_TIMEOUT,
	GSM_STATUS_ERROR,
	GSM_STATUS_RECV,
} gsm_status_t;

void
gsm_init(FILE *in, FILE *out, void (*cb)(uint8_t status, const sbuf_t *from,
					 const sbuf_t *msg));
int gsm_send_sms(const char *number, const char *sms);
void gsm_handle_interrupt(uint8_t c);
int gsm_tests(void);

#endif
