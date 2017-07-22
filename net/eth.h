#ifndef _ETH_H_
#define _ETH_H_

#define ETHER_ADDR_LEN 6
#define ETHER_TYPE_LEN 2
#define ETHER_CRC_LEN  4
/* XXX to be used by the driver */
#define ETHER_MIN_LEN  64
#define ETHER_MAX_LEN  1518

/* supported ethertypes */
#if BYTE_ORDER == LITTLE_ENDIAN
#define	ETHERTYPE_ARP  0x0608  /* Address resolution protocol */
#define	ETHERTYPE_IP   0x0080  /* IP protocol */
#define	ETHERTYPE_IPV6 0xDD86  /* IP protocol version 6 */
#else
#define	ETHERTYPE_ARP  0x0806  /* Address resolution protocol */
#define	ETHERTYPE_IP   0x0800  /* IP protocol */
#define	ETHERTYPE_IPV6 0x86DD  /* IP protocol version 6 */
#endif

struct eth_hdr {
	uint8_t dst[ETHER_ADDR_LEN];
	uint8_t src[ETHER_ADDR_LEN];
	uint16_t type;
} __attribute__((__packed__));

typedef struct eth_hdr eth_hdr_t;

struct iface;
void eth_input(buf_t buf, const struct iface *iface);

#endif
