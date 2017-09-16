#include <stdint.h>
#include "ip.h"
#include "udp.h"
#include "tcp.h"
#include "chksum.h"

static uint32_t cksum_partial(const void *data, uint16_t len)
{
	const uint16_t *w = data;
	uint32_t sum = 0;

	while (len > 1)  {
		sum += *w++;
		len -= 2;
	}

	if (len == 1)
		sum += *(uint8_t *)w;

	return sum;
}

uint16_t cksum(const void *data, uint16_t len)
{
	uint32_t sum = cksum_partial(data, len);

	sum = (sum >> 16) + (sum & 0xffff);
	sum += sum >> 16;

	return ~sum;
}

/* len is in network byte order */
uint16_t
transport_cksum(const ip_hdr_t *ip, const void *hdr, uint16_t len)
{
	uint32_t csum;

	csum  = cksum_partial(&ip->src, sizeof(ip->src));
	csum += cksum_partial(&ip->dst, sizeof(ip->dst));

	/* even though ip->p is already in network byte order,
	 * htons() will convert it to a uint16_t properly
	 */
	csum += htons(ip->p);

	csum += cksum_partial(&len, sizeof(len));
	csum += cksum_partial(hdr, ntohs(len));
	csum = (csum & 0xFFFF) + (csum >> 16);
	csum += csum >> 16;

	csum = ~csum;
	return csum;
}

void set_transport_cksum(const void *iph, void *trans_hdr, int len)
{
	const ip_hdr_t *ip_hdr = iph;
	uint16_t *checksum;

	if (ip_hdr->p == IPPROTO_UDP) {
		udp_hdr_t *udp_hdr = trans_hdr;
		checksum = &udp_hdr->checksum;
	} else if (ip_hdr->p == IPPROTO_TCP) {
		tcp_hdr_t *tcp_hdr = trans_hdr;
		checksum = &tcp_hdr->checksum;
	} else {
		assert(0);
		return;
	}
	*checksum = 0;
	*checksum = transport_cksum(ip_hdr, trans_hdr, len);
	if (*checksum == 0)
		*checksum = 0xFFFF;
}
