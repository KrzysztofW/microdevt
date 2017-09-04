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

void icmp_input(pkt_t *pkt, iface_t *iface);
void icmp_output(pkt_t *out, iface_t *iface, int type,
		 uint16_t id, uint16_t seq, const buf_t *id_data);

#endif
