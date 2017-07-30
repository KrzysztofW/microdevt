#ifndef _IF_H_
#define _IF_H_
#include "../sys/buf.h"

#include "config.h"

#define IFF_UP      (1 << 0)
#define IFF_RUNNING (1 << 1)
#define IFF_PROMISC (1 << 2)
#define IFF_NOARP   (1 << 3)
/*      IFF_LAST    (1 << 7) */

typedef struct iface {
	uint8_t flags;
	uint8_t mac_addr[ETHER_ADDR_LEN];

	/* only one ip address allowed (network endianess) */
	uint8_t ip4_addr[IP_ADDR_LEN];
#ifdef IPV6
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
	buf_t rx_buf;
	buf_t tx_buf;
} iface_t;

int if_init(iface_t *ifce, int rx_size, int tx_size,
	    uint16_t (*send)(const buf_t *out),
	    uint16_t (*recv)(buf_t *in));
void if_shutdown(iface_t *ifce);

#endif

