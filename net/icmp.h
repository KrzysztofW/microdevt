#ifndef _ICMP_H_
#define _ICMP_H_

#include "config.h"

struct icmp {
	uint8_t   type;  /* type of message, see below */
	uint8_t   code;  /* type sub code */
	uint16_t  cksum; /* ones complement cksum of struct */
	uint16_t  id;
	uint16_t  seq;
	int8_t	  id_data[];
} __attribute__((__packed__));

typedef struct icmp icmp_hdr_t;

void icmp_input(buf_t buf, const iface_t *iface);

#endif
