#ifndef _ROUTE_H_
#define _ROUTE_H_

#include "proto-defs.h"
#include "config.h"

struct route {
	uint32_t ip;
	iface_t *iface;
} __attribute__((__packed__));
typedef struct route route_t;

extern route_t dft_route;

#ifdef CONFIG_IPV6
struct route6 {
	uint8_t ip[IP6_ADDR_LEN];
	/* iface_t *iface; */
} __attribute__((__packed__));
typedef struct route6 route6_t;

extern route6_t dft_route6;
#endif

#endif
