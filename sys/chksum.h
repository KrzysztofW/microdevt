#ifndef _CHKSUM_H_
#define _CHKSUM_H_
#include <stdint.h>

uint16_t cksum(const void *data, uint16_t len);
uint32_t cksum_partial(const void *data, uint16_t len);
uint16_t cksum_finish(uint32_t csum);

#endif
