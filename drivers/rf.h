#ifndef _RF_H_
#define _RF_H_
#include <sys/ring.h>

void *rf_init(uint8_t burst);
void rf_shutdown(void *handle);
int rf_output(void *handle, uint16_t bufs_len, uint8_t nb_bufs,
	      const buf_t **bufs);
int rf_send(void *handle, uint16_t len, int8_t burst, ...);
ring_t *rf_get_ring(void *handle);
int rf_checks(void *handle);
#endif
