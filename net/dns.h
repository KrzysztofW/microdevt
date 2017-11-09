#ifndef _DNS_H_
#define _DNS_H_
#include <stdint.h>
#include "../sys/buf.h"

void dns_init(uint32_t ip);
int dns_resolve(const sbuf_t *name, void (*cb)(uint32_t ip));

#endif
