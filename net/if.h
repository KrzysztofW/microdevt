#ifndef _IF_H_
#define _IF_H_
#include "../sys/buf.h"
#include "../sys/list.h"

#include "config.h"

#define IF_UP      (1 << 0)
#define IF_RUNNING (1 << 1)
#define IF_PROMISC (1 << 2)
#define IF_NOARP   (1 << 3)
/*      IF_LAST    (1 << 7) */

struct iface {
	uint8_t flags;
	uint8_t mac_addr[ETHER_ADDR_LEN];

	/* only one ip address allowed (network endianess) */

	uint8_t ip4_addr[IP_ADDR_LEN];
	uint8_t ip4_mask[IP_ADDR_LEN];
#ifdef CONFIG_IPV6
	uint8_t ip6_addr[IP6_ADDR_LEN];
#endif
	uint16_t (*send)(const buf_t *out);
	uint16_t (*recv)(buf_t *in);
#ifdef CONFIG_STATS
	uint16_t rx_packets;
	uint16_t rx_errors;
	uint16_t rx_dropped;
	uint16_t rx_packets;
	uint16_t rx_errors;
	uint16_t rx_dropped;
#endif
	list_t rx;
	list_t tx;
} __attribute__((__packed__));
typedef struct iface iface_t;

int if_init(iface_t *ifce, uint16_t (*send)(const buf_t *out),
	    uint16_t (*recv)(buf_t *in));
void if_shutdown(iface_t *ifce);

#endif
