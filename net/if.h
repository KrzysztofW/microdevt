#ifndef _IF_H_
#define _IF_H_
#include <sys/buf.h>
#include <sys/list.h>

#include "config.h"

#define IF_UP      (1 << 0)
#define IF_RUNNING (1 << 1)
#define IF_PROMISC (1 << 2)
#define IF_NOARP   (1 << 3)
/*      IF_LAST    (1 << 7) */

typedef enum if_type {
	IF_TYPE_ETHERNET,
	IF_TYPE_RF,
} if_type_t;

struct iface {
	uint8_t flags;
	uint8_t type;
	uint8_t *hw_addr;

	/* only one ip address allowed (network endianess) */
#ifdef CONFIG_IP
	uint8_t *ip4_addr;
	uint8_t *ip4_mask;
#endif
#ifdef CONFIG_IPV6
	uint8_t *ip6_addr;
#endif
	/* low level driver functions used in interrupt handlers */
	/* asynchronious sending if possible */
	int (*send)(const struct iface *iface, pkt_t *pkt);
	void (*recv)(const struct iface *iface);

	/* private interface handle */
	void *priv;

	/* level 2 functions used in bottom halves */
	/* points to eth_output() or swen_output() */
	int (*if_output)(pkt_t *out, const struct iface *iface, uint8_t type,
			 const void *dst);
	/* points to eth_input() or swen_input() */
	void (*if_input)(const struct iface *iface);

#ifdef CONFIG_STATS
	uint16_t rx_packets;
	uint16_t rx_errors;
	uint16_t rx_dropped;
	uint16_t tx_packets;
	uint16_t tx_errors;
	uint16_t tx_dropped;
#endif
	/* interrupt handler's pkt rings */
	ring_t *pkt_pool;
	ring_t *rx;
	ring_t *tx;
} __attribute__((__packed__));
typedef struct iface iface_t;

void if_init(iface_t *ifce, uint8_t type,
	     unsigned nb_pkt_rx, unsigned nb_pkt_tx, unsigned nb_if_pkt_pool);
void if_shutdown(iface_t *ifce);

/* functions supposed to be called from an interrupt handler */
void if_schedule_receive(const iface_t *iface, pkt_t *pkt);
void if_schedule_tx_pkt_free(pkt_t *pkt);
#endif
