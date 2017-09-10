#ifndef _UDP_H_
#define _UDP_H_

#include "config.h"

#define UDP_MAX_BINDS 8

typedef struct udp_port_bind {
	uint16_t port;
	uint16_t socket;
} udp_port_bind_t;

int udp_init(void);
void udp_shutdown(void);
void udp_input(pkt_t *pkt, iface_t *iface);
void udp_output(pkt_t *out, iface_t *iface, const buf_t *buf);
int udp_bind(int fd, uint16_t port);

struct udp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
}  __attribute__((__packed__));

typedef struct udp_hdr udp_hdr_t;

#endif
