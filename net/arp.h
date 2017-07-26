#ifndef _ARP_H_
#define _ARP_H_

#include "config.h"

#if BYTE_ORDER == BIG_ENDIAN
#define ARPHRD_ETHER	 1	/* ethernet hardware format */
#else
#define ARPHRD_ETHER	 0x100	/* ethernet hardware format */
#endif

#define ARPHRD_IEEE802	 6	/* IEEE 802 hardware format */
#define ARPHRD_ARCNET	 7	/* ethernet hardware format */
#define ARPHRD_FRELAY	15	/* frame relay hardware format */
#define ARPHRD_STRIP	23	/* Ricochet Starmode Radio hardware format */
#define	ARPHRD_IEEE1394	24	/* IEEE 1394 (FireWire) hardware format */

/* ARP opcodes */
#if BYTE_ORDER == BIG_ENDIAN
#define	ARPOP_REQUEST	 1	/* request to resolve address */
#define	ARPOP_REPLY	 2	/* response to previous request */
#define	ARPOP_REVREQUEST 3	/* request protocol address given hardware */
#define	ARPOP_REVREPLY	 4	/* response giving protocol address */
#define	ARPOP_INVREQUEST 8	/* request to identify peer */
#define	ARPOP_INVREPLY	 9	/* response identifying peer */
#else
#define	ARPOP_REQUEST	 0x100	/* request to resolve address */
#define	ARPOP_REPLY	 0x200	/* response to previous request */
#define	ARPOP_REVREQUEST 0x300	/* request protocol address given hardware */
#define	ARPOP_REVREPLY	 0x400	/* response giving protocol address */
#define	ARPOP_INVREQUEST 0x800	/* request to identify peer */
#define	ARPOP_INVREPLY	 0x900	/* response identifying peer */
#endif

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
	//	uint8_t ip[IP_ADDR_LEN];
	uint32_t ip;
	uint8_t mac[ETHER_ADDR_LEN];
	iface_t *iface;
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
	iface_t *iface;
} arp_entry_t;

typedef struct arp6_entries {
	arp6_entry_t entries[CONFIG_ARP_TABLE_SIZE];
	uint8_t pos;
} arp6_entries_t;
#endif

void arp_input(buf_t buf, iface_t *iface);
int arp_find_entry(uint32_t ip, uint8_t **mac, iface_t **iface);
void arp_output(iface_t *iface, int op, uint8_t *tha, uint8_t *tpa);
#ifdef TEST
arp_entries_t *arp_get_entries(void);
#endif

#endif
