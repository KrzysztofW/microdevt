#ifndef _PROTO_DEFS_H_
#define _PROTO_DEFS_H_

/* Supported l3 protocols - internal values */
#define L3_PROTO_IP       0x0
#define L3_PROTO_ARP      0x2
#define L3_PROTO_SWEN     0x3
#define L3_PROTO_NONE     0x4

/* Ethernet */
#define ETHER_ADDR_LEN 6
#define ETHER_TYPE_LEN 2
#define ETHER_CRC_LEN  4
/* XXX to be used by the driver */
#define ETHER_MIN_LEN  64
#define ETHER_MAX_LEN  1518

/* supported ethertypes */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define	ETHERTYPE_ARP  0x0608  /* Address resolution protocol */
#define	ETHERTYPE_IP   0x0008  /* IP protocol */
#define	ETHERTYPE_IPV6 0xDD86  /* IP protocol version 6 */
#else
#define	ETHERTYPE_ARP  0x0806  /* Address resolution protocol */
#define	ETHERTYPE_IP   0x0800  /* IP protocol */
#define	ETHERTYPE_IPV6 0x86DD  /* IP protocol version 6 */
#endif

/* ARP */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
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
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
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

/* IP */
#define IP_ADDR_LEN 4
#ifdef IPV6
#define IP6_ADDR_LEN 16
#endif

#define	IPPROTO_ICMP	1
#define	IPPROTO_TCP	6
#define	IPPROTO_UDP	17
#define	IPPROTO_ICMPV6	58

/* ICMP */
#define ICMP_ECHOREPLY 0
#define	ICMP_UNREACHABLE		3	  /* dest unreachable, codes: */
#define		ICMP_UNREACH_NET	0	  /* bad net */
#define		ICMP_UNREACH_HOST	1	  /* bad host */
#define		ICMP_UNREACH_PROTOCOL	2	  /* bad protocol */
#define		ICMP_UNREACH_PORT	3	  /* bad port */
#define		ICMP_UNREACH_NEEDFRAG	4	  /* IP_DF caused drop */
#define		ICMP_UNREACH_SRCFAIL	5	  /* src route failed */
#define		ICMP_UNREACH_NET_UNKNOWN 6	  /* unknown net */
#define		ICMP_UNREACH_HOST_UNKNOWN 7	  /* unknown host */
#define		ICMP_UNREACH_ISOLATED	8	  /* src host isolated */
#define		ICMP_UNREACH_NET_PROHIB	9	  /* prohibited access */
#define		ICMP_UNREACH_HOST_PROHIB 10	  /* ditto */
#define		ICMP_UNREACH_TOSNET	11	  /* bad tos for net */
#define		ICMP_UNREACH_TOSHOST	12	  /* bad tos for host */
#define		ICMP_UNREACH_FILTER_PROHIB 13	  /* admin prohib */
#define		ICMP_UNREACH_HOST_PRECEDENCE 14	  /* host prec vio. */
#define		ICMP_UNREACH_PRECEDENCE_CUTOFF 15 /* prec cutoff */
#define ICMP_ECHO 8

#endif
