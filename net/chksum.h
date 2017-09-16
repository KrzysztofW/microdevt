#ifndef _CHKSUM_H_
#define _CHKSUM_H_

uint16_t cksum(const void *data, uint16_t len);
void set_transport_cksum(const void *iph, void *trans_hdr, int len);
uint16_t
transport_cksum(const ip_hdr_t *ip, const void *hdr, uint16_t len);

#endif
