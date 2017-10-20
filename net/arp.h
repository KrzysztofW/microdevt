#ifndef _ARP_H_
#define _ARP_H_

#include "config.h"

struct arp_hdr {
	uint16_t hrd;	/* format of hardware address */
	uint16_t proto;	/* format of protocol address */
	uint8_t  hln;	/* length of hardware address */
	uint8_t  pln;	/* length of protocol address */
	uint16_t op;	/* ARP opcode */
	/* uint8_t  sha[]; /\* sender hardware address *\/ */
	/* uint8_t  spa[]; /\* sender protocol address *\/ */
	/* uint8_t  tha[]; /\* target hardware address *\/ */
	/* uint8_t  tpa[]; /\* target protocol address *\/ */
	uint8_t data[];
} __attribute__((__packed__));

typedef struct arp_hdr arp_hdr_t;

typedef struct arp_entry {
	uint32_t ip;
	uint8_t mac[ETHER_ADDR_LEN];
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	iface_t *iface;
#endif
#ifdef CONFIG_ARP_EXPIRY
	uint8_t updated; /* used by arp_timer */
#endif
} arp_entry_t;

typedef struct arp_entries {
	arp_entry_t entries[CONFIG_ARP_TABLE_SIZE];
	uint8_t pos;
} arp_entries_t;

#ifdef CONFIG_IPV6
typedef struct arp6_entry {
	uint8_t ip[IP6_ADDR_LEN];
	uint8_t mac[ETHER_ADDR_LEN];
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	iface_t *iface;
#endif
} arp_entry_t;

typedef struct arp6_entries {
	arp6_entry_t entries[CONFIG_ARP_TABLE_SIZE];
	uint8_t pos;
} arp6_entries_t;
#endif

extern uint8_t broadcast_mac[];

void arp_input(pkt_t *pkt, iface_t *iface);
int arp_find_entry(uint32_t ip, uint8_t **mac, iface_t **iface);
void arp_output(iface_t *iface, int op, uint8_t *tha, uint8_t *tpa);
void arp_add_entry(uint8_t *sha, uint8_t *spa, iface_t *iface);
#ifdef CONFIG_IPV6
static void arp6_add_entry(uint8_t *sha, uint8_t *spa, iface_t *iface);
#endif

void arp_resolve(pkt_t *pkt, uint32_t ip_dst, iface_t *iface,
		 uint8_t retries);

#ifdef TEST
arp_entries_t *arp_get_entries(void);
#endif

#endif
