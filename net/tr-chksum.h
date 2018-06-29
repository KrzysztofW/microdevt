#ifndef _TR_CHKSUM_H_
#define _TR_CHKSUM_H_
#include <sys/chksum.h>
#include "ip.h"
#include "udp.h"
#include "tcp.h"

void set_transport_cksum(const void *iph, void *trans_hdr, int len);
uint16_t
transport_cksum(const ip_hdr_t *ip, const void *hdr, uint16_t len);

#endif
