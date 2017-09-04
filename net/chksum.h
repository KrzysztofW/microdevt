#ifndef _CHKSUM_H_
#define _CHKSUM_H_

uint16_t cksum(const void *data, uint16_t len);
uint32_t cksum_partial(const void *data, uint16_t len);

#endif
