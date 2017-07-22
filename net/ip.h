#ifndef _IP_H_
#define _IP_H_

#define IP_ADDR_LEN 4
#ifdef IPV6
#define IP6_ADDR_LEN 16
#endif

struct ip_hdr {
#if BYTE_ORDER == LITTLE_ENDIAN
	unsigned int ip_hl:4,		/* header length */
		     ip_v:4;		/* version */
#endif
/* #if BYTE_ORDER == BIG_ENDIAN */
/* 	unsigned int ip_v:4,		/\* version *\/ */
/* 		     ip_hl:4;		/\* header length *\/ */
/* #endif */
	uint8_t  ip_tos;		/* type of service */
	uint16_t ip_len;		/* total length */
	uint16_t ip_id;			/* identification */
	uint16_t ip_off;		/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_EF 0x8000			/* evil flag, per RFC 3514 */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	uint8_t  ip_ttl;		/* time to live */
	uint8_t  ip_p;			/* protocol */
	uint16_t ip_sum;		/* checksum */
	uint32_t ip_src, ip_dst;	/* source and dest address */
} __attribute__((__packed__));

typedef struct ip_hdr ip_hdr_t;

#define	IPPROTO_ICMP	1
#define	IPPROTO_TCP	6
#define	IPPROTO_UDP	17
#define	IPPROTO_ICMPV6	58

#endif
