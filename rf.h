#ifndef _RF_H_
#define _RF_H_

void *rf_init(void);
void rf_shutdown(void *handle);
int rf_send(void *handle, uint16_t len, int8_t burst, ...);
ring_t *rf_get_ring(void *handle);

#endif
