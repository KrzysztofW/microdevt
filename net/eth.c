#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"

void eth_input(pkt_t *pkt, iface_t *iface)
{
	eth_hdr_t *eh;

	if ((iface->flags & IF_UP) == 0)
		goto unsupported;

	eh = btod(pkt, eth_hdr_t *);
#ifdef CONFIG_PROMISC
	if (iface->flags & IFF_PROMISC) {
		/* check mac address */
	}
#endif
	pkt_adj(pkt, sizeof(eth_hdr_t));

	switch (eh->type) {
	case ETHERTYPE_ARP:
		arp_input(pkt, iface);
		return;

	case ETHERTYPE_IP:
		ip_input(pkt, iface);
		return;

	case ETHERTYPE_IPV6:
		//ip6_input(pkt, iface);
		break;

	default:
		/* unsupported ethertype */
		break;
	}
 unsupported:
	pkt_free(pkt);
}

void eth_output(pkt_t *out, iface_t *iface, const uint8_t *mac_dst,
		uint16_t type)
{
	eth_hdr_t *eh;
	int i;

	if ((iface->flags & IF_UP) == 0) {
		pkt_free(out);
		return;
	}

	pkt_adj(out, -(int)sizeof(eth_hdr_t));
	eh = btod(out, eth_hdr_t *);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		eh->dst[i] = mac_dst[i];
		eh->src[i] = iface->mac_addr[i];
	}
	eh->type = type;
	if (pkt_put(&iface->tx, out) < 0) {
		pkt_free(out);
	}
#ifdef X86
	/* send the packet immediately */
	while ((out = pkt_get(&iface->tx)) != NULL) {
		iface->send(&out->buf);
		pkt_free(out);
	}
#endif
	/* TODO update iface stats */
}
