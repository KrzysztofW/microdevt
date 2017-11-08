#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "eth.h"
#include "chksum.h"
#include "route.h"
#include "udp.h"
#include "tcp.h"
#include <assert.h>

#ifndef CONFIG_IP_TTL
#error "mandatory CONFIG_IP_TTL option not set"
#endif

void ip_output(pkt_t *out, iface_t *iface, uint16_t flags)
{
	ip_hdr_t *ip = btod(out, ip_hdr_t *);
	uint8_t *mac_addr;
	uint32_t ip_dst;
	uint32_t *mask;
	uint32_t *ip_addr;
	uint16_t payload_len = pkt_len(out);

	if (iface == NULL)
		iface = dft_route.iface;

	if (iface == NULL) {
		/* no interface to send the pkt to */
		pkt_free(out);
		return;
	}
	mask = (uint32_t *)iface->ip4_mask;
	ip_addr = (uint32_t *)iface->ip4_addr;

	/* XXX check for buf_adj coherency with other layers */
	if (ip->dst == 0) {
		/* no dest ip address set. Drop the packet */
		pkt_free(out);
		return;
	}

	ip->src = *ip_addr;
	ip->v = 4;
	ip->hl = sizeof(ip_hdr_t) / 4;
	ip->tos = 0;
	ip->len = htons(payload_len);
	ip->id = 0;
	ip->off = flags;
	ip->ttl = CONFIG_IP_TTL;
	assert(ip->p); /* must be set by upper layer */
	ip->chksum = 0;
	ip->chksum = cksum(ip, sizeof(ip_hdr_t));

	if ((ip->dst & *mask) != (*ip_addr & *mask))
		ip_dst = dft_route.ip;
	else
		ip_dst = ip->dst;

	if (arp_find_entry(ip_dst, &mac_addr, &iface) < 0) {
		arp_resolve(out, ip_dst, iface);
		return;
	}

	pkt_adj(out, (int)sizeof(ip_hdr_t));
	if (ip->p == IPPROTO_UDP) {
		udp_hdr_t *udp_hdr = btod(out, udp_hdr_t *);
		set_transport_cksum(ip, udp_hdr, udp_hdr->length);
	} else if (ip->p == IPPROTO_TCP) {
		tcp_hdr_t *tcp_hdr = btod(out, tcp_hdr_t *);
		set_transport_cksum(ip, tcp_hdr, htons(payload_len - sizeof(ip_hdr_t)));
	}
	pkt_adj(out, -(int)sizeof(ip_hdr_t));

	eth_output(out, iface, mac_addr, ETHERTYPE_IP);
}

void ip_input(pkt_t *pkt, iface_t *iface)
{
	ip_hdr_t *ip;
	uint32_t *ip_addr = (uint32_t *)iface->ip4_addr;
	int ip_len;

	ip = btod(pkt, ip_hdr_t *);

	if (ip->v != 4 || ip->dst != *ip_addr || ip->ttl == 0)
		goto error;

	if (ip->off & IP_MF) {
		/* ip fragmentation is unsupported */
		goto error;
	}
	if (ip->hl > IP_MAX_HDR_LEN || ip->hl < IP_MIN_HDR_LEN)
		goto error;

	ip_len = ip->hl * 4;
	if (cksum(ip, ip_len) != 0)
		goto error;

#ifdef CONFIG_IP_CHKSUM
#endif

	switch (ip->p) {
#ifdef CONFIG_ICMP
	case IPPROTO_ICMP:
		icmp_input(pkt, iface);
		return;

#ifdef CONFIG_IPV6
	case IPPROTO_ICMPV6:
		icmp6_input(pkt, iface);
		return;
#endif
#endif
#ifdef CONFIG_TCP
	case IPPROTO_TCP:
		tcp_input(pkt);
		return;
#endif
#ifdef CONFIG_UDP
	case IPPROTO_UDP:
		udp_input(pkt, iface);
		return;
#endif
	default:
		/* unsupported protocols */
		break;
	}

 error:
	pkt_free(pkt);
	/* inc stats */
}
