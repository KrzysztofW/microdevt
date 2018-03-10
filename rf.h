#ifndef _RF_H_
#define _RF_H_

int rf_init(void);
void rf_shutdown(void);
int rf_recvfrom(uint8_t *from, buf_t *buf);
int rf_sendto(uint8_t to, const buf_t *data, int8_t burst);
void rf_start_sending(void);

#ifdef CONFIG_RF_KERUI_CMDS
void rf_set_kerui_cb(void (*cb)(int nb));
#endif

#endif
