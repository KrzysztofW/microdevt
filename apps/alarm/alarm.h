#ifndef _ALARM_H_
#define _ALARM_H_

void alarm_gsm_init(void);
void alarm_network_init(void);
void alarm_network_loop(void);
void alarm_rf_init(void);
void alarm_parse_uart_commands(buf_t *buf);

#endif
