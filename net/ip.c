#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "eth.h"
#include "chksum.h"
#include <assert.h>

void ip_output(buf_t *out, iface_t *iface)
{
	ip_hdr_t *ip = btod(out, ip_hdr_t *);
	uint8_t *mac_addr;

	/* XXX check for buf_adj coherency with other layers */
	if (ip->dst == 0) {
		/* no dest ip address set. Drop the packet */
		return;
	}
	if (ip->src == 0) {
		uint32_t *ip_addr = (uint32_t *)iface->ip4_addr;
		ip->src = *ip_addr;
	}

	ip->v = 4;
	ip->hl = sizeof(ip_hdr_t) / 4;
	ip->tos = 0;
	ip->len = htons(out->len);
	ip->id = 0;
	ip->off = 0;
	ip->ttl = 0x38;
	assert(ip->p); /* must be set by upper layer */
	ip->chksum = 0;
	ip->chksum = cksum(ip, sizeof(ip_hdr_t));

	if (arp_find_entry(ip->dst, &mac_addr, &iface) < 0) {
		/* XXX TODO arp resolution */
		return;
	}

	eth_output(out, iface, mac_addr, ETHERTYPE_IP);
}

void ip_input(buf_t buf, iface_t *iface)
{
	ip_hdr_t *ip;
	uint32_t *ip_addr = (uint32_t *)iface->ip4_addr;
	int ip_len;

	ip = btod(&buf, ip_hdr_t *);

	if (ip->v != 4 || ip->dst != *ip_addr || ip->ttl == 0)
		goto error;

	if (ip->off & IP_MF) {
		/* ip fragmentation is unsupported */
		goto error;
	}
	if (ip->hl > 15 || ip->hl < 5) {
		goto error;
	}

	ip_len = ip->hl * 4;
	if (cksum(ip, ip_len) != 0) {
		goto error;
	}

#ifdef CONFIG_IP_CHKSUM
#endif

	switch (ip->p) {
	case IPPROTO_ICMP:
		icmp_input(buf, iface);
		return;

#ifdef CONFIG_IPV6
	case IPPROTO_ICMPV6:
		icmp6_input(buf, iface);
		return;
#endif
	case IPPROTO_TCP:
		//tcp_input(buf, iface);
		return;

	case IPPROTO_UDP:
		//udp_input(buf, iface);
		return;
	default:
		/* unsupported protocols */
		break;
	}

 error:
	/* inc stats */
	return;
}
