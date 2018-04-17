#ifndef _GSM_H_
#define _GSM_H_

typedef enum gsm_status {
	GSM_STATUS_OK,
	GSM_STATUS_TIMEOUT,
	GSM_STATUS_ERROR,
} gsm_status_t;

void gsm_init(void (*cb)(uint8_t status));
int gsm_send_sms(const char *number, const char *sms);

#endif
