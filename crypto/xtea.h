#ifndef _XTEA_H_
#define _XTEA_H_

#include <sys/buf.h>

int xtea_encode(buf_t *buf, uint32_t const key[4]);
int xtea_decode(buf_t *buf, uint32_t const key[4]);

#endif
