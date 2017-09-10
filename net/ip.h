#ifndef _IP_H_
#define _IP_H_

#include "config.h"

struct ip_hdr {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t hl:4;		/* header length */
	uint8_t  v:4;		/* version */
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_T   v:4;		/* version */
	uint8_t  hl:4;		/* header length */
#endif
	uint8_t  tos;		/* type of service */
	uint16_t len;		/* total length */
	uint16_t id;		/* identification */
	uint16_t off;		/* fragment offset field */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define	IP_RF 0x8000		/* reserved fragment flag */
#define	IP_EF 0x8000		/* evil flag, per RFC 3514 */
#define	IP_DF 0x4000		/* dont fragment flag */
#define	IP_MF 0x2000		/* more fragments flag */
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define	IP_RF 0x80		/* reserved fragment flag */
#define	IP_EF 0x80		/* evil flag, per RFC 3514 */
#define	IP_DF 0x40		/* dont fragment flag */
#define	IP_MF 0x20		/* more fragments flag */
#endif
#define	IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	uint8_t  ttl;		/* time to live */
	uint8_t  p;		/* protocol */
	uint16_t chksum;	/* checksum */
	uint32_t src, dst;	/* source and dest address */
} __attribute__((__packed__));

typedef struct ip_hdr ip_hdr_t;

#define IP_MIN_HDR_LEN 5
#define IP_MAX_HDR_LEN 15

void ip_input(pkt_t *pkt, iface_t *iface);
void ip_output(pkt_t *out, iface_t *iface, uint8_t retries, uint16_t flags);

#endif
