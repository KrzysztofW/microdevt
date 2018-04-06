#ifndef _ICMP_H_
#define _ICMP_H_

#include "config.h"

struct icmp {
	uint8_t   type;  /* type of message */
	uint8_t   code;  /* type sub code */
	uint16_t  cksum; /* ones complement cksum of struct */
	uint16_t  id;
	uint16_t  seq;
	uint8_t	  id_data[];
} __attribute__((__packed__));

typedef struct icmp icmp_hdr_t;

#define ALLOWED_ICMP_MAX_DATA_SIZE 586
/* MAX_ICMP_DATA_SIZE should not exceed ALLOWED_ICMP_MAX_DATA_SIZE bytes */
#define MAX_ICMP_DATA_SIZE (int)(CONFIG_PKT_SIZE - sizeof(eth_hdr_t) \
				 - sizeof(ip_hdr_t) - sizeof(icmp_hdr_t))

void icmp_input(pkt_t *pkt, const iface_t *iface);
int
icmp_output(pkt_t *out, const iface_t *iface, int type, int code,
	    uint16_t id, uint16_t seq, const buf_t *id_data, uint16_t ip_flags);

#endif
