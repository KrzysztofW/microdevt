#include <string.h>
#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"

void eth_input(buf_t buf, iface_t *iface)
{
	eth_hdr_t *eh;

	if ((iface->flags & IFF_UP) == 0) {
		return;
	}
	eh = btod(&buf, eth_hdr_t *);
#ifdef CONFIG_PROMISC
	if (iface->flags & IFF_PROMISC) {
		/* check mac address */
	}
#endif
	buf_adj(&buf, sizeof(eth_hdr_t));

	switch (eh->type) {
	case ETHERTYPE_ARP:
		arp_input(buf, iface);
		break;

	case ETHERTYPE_IP:
		ip_input(buf, iface);
		break;

	case ETHERTYPE_IPV6:
		//ip6_input(buf, iface);
		break;

	default:
		break;
		/* unsupported ethertype */
	}
}

void eth_output(buf_t *out, iface_t *iface, const uint8_t *mac_dst,
		uint16_t type)
{
	eth_hdr_t *eh;
	int i;

	if ((iface->flags & IFF_UP) == 0) {
		return;
	}

	buf_adj(out, -(int)sizeof(eth_hdr_t));
	eh = btod(out, eth_hdr_t *);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		eh->dst[i] = mac_dst[i];
		eh->src[i] = iface->mac_addr[i];
	}
	eh->type = type;
	iface->send(out);
	/* TODO update iface stats */
}
