#ifndef _UDP_H_
#define _UDP_H_

#include "config.h"

struct udp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
} __attribute__((__packed__));

typedef struct udp_hdr udp_hdr_t;

void udp_input(pkt_t *pkt, const iface_t *iface);
int udp_output(pkt_t *pkt, uint32_t ip_dst, uint16_t sport, uint16_t dport);

#endif
