#ifndef _ROUTE_H_
#define _ROUTE_H_

#include "proto_defs.h"
#include "config.h"

typedef struct route {
	uint32_t ip;
	iface_t *iface;
} route_t;

extern route_t dft_route;

#ifdef CONFIG_IPV6
typedef struct route6 {
	uint8_t ip[IP6_ADDR_LEN];
	/* iface_t *iface; */
} route6_t;

extern route6_t dft_route6;
#endif

#endif
