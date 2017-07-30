#ifndef _ETH_H_
#define _ETH_H_

#include "config.h"

struct eth_hdr {
	uint8_t dst[ETHER_ADDR_LEN];
	uint8_t src[ETHER_ADDR_LEN];
	uint16_t type;
} __attribute__((__packed__));

typedef struct eth_hdr eth_hdr_t;

struct iface;
void eth_input(buf_t buf, struct iface *iface);
void eth_output(buf_t *buf, struct iface *iface, const uint8_t *mac_dst);

#endif
