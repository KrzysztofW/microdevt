#ifndef _SWEN_H_
#define _SWEN_H_

int swen_recvfrom(void *handle, uint8_t *from, buf_t *buf);
int swen_sendto(void *handle, uint8_t to, const buf_t *data);
void *
swen_init(void *rf_handle, uint8_t addr, void (*generic_cmds_cb)(int nb),
	  uint8_t *cmds);
void swen_shutdown(void *handle);

#endif
