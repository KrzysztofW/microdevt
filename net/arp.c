#include "arp.h"
#include "eth.h"
#include "../timer.h"

arp_entries_t arp_entries;
#ifdef CONFIG_IPV6
arp6_entries_t arp6_entries;
#endif

#ifdef CONFIG_ARP_EXPIRY
tim_t arp_timer;
#endif

uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int arp_find_entry(uint32_t ip, uint8_t **mac, iface_t **iface)
{
	int i;

	/* linear search ... that's bad */
	for (i = 0; i < CONFIG_ARP_TABLE_SIZE; i++) {
		if (arp_entries.entries[i].ip == ip) {
			*mac = arp_entries.entries[i].mac;
			*iface = arp_entries.entries[i].iface;
			return 0;
		}
	}
	return -1;
}

void arp_add_entry(uint8_t *sha, uint8_t *spa, iface_t *iface)
{
	int i;
	arp_entry_t *e = &arp_entries.entries[arp_entries.pos];
	uint8_t *ip = (uint8_t *)&e->ip;

	STATIC_ASSERT(POWEROF2(CONFIG_ARP_TABLE_SIZE));

	for (i = 0; i < IP_ADDR_LEN; i++) {
		ip[i] = spa[i];
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		e->mac[i] = sha[i];
	}
	e->iface = iface;
	arp_entries.pos = (arp_entries.pos + 1) & (CONFIG_ARP_TABLE_SIZE-1);
}

#ifdef CONFIG_IPV6
static void arp6_add_entry(uint8_t *sha, uint8_t *spa, iface_t *iface)
{
	int i;
	arp6_entry_t *e = arp6_entries.entries[arp6_entries.pos];

	for (i = 0; i < IP6_ADDR_LEN; i++) {
		e->ip[i] = spa[i];
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		e->mac[i] = sha[i];
	}
	e->iface = iface;
	arp6_entries.pos = (arp6_entries.pos + 1) & (CONFIG_ARP_TABLE_SIZE-1);
}
#endif

void arp_output(iface_t *iface, int op, uint8_t *tha, uint8_t *tpa)
{
	int i;
	pkt_t *out = pkt_alloc();
	arp_hdr_t *ah;
	uint8_t *data;

	if (out == NULL) {
		/* inc stats */
		return;
	}
	pkt_adj(out, (int)sizeof(eth_hdr_t));
	ah = btod(out, arp_hdr_t *);
	pkt_adj(out, (int)sizeof(arp_hdr_t));
	ah->hrd = ARPHRD_ETHER;
	ah->proto = ETHERTYPE_IP;
	ah->hln = ETHER_ADDR_LEN;
	ah->pln = IP_ADDR_LEN;
	ah->op = op;
	data = ah->data;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		*data = iface->mac_addr[i];
		data++;
	}
	for (i = 0; i < IP_ADDR_LEN; i++) {
		*data = iface->ip4_addr[i];
		data++;
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (tha[0] == 0xFF)
			*data = 0;
		else
			*data = tha[i];
		data++;
	}
	for (i = 0; i < IP_ADDR_LEN; i++) {
		*data = tpa[i];
		data++;
	}
	pkt_adj(out, data - ah->data);
	pkt_adj(out, -((int)sizeof(arp_hdr_t) + (data - ah->data)));
	eth_output(out, iface, tha, ETHERTYPE_ARP);
}

void arp_input(pkt_t *pkt, iface_t *iface)
{
	uint8_t *sha, *spa, *tha, *tpa;
	arp_hdr_t *ah = btod(pkt, arp_hdr_t *);
	int i;

	if (ah->hrd != ARPHRD_ETHER) {
		/* unsupported ether hw address */
		goto error;
	}
	if (ah->proto != ETHERTYPE_IP) {
		/* unsupported protocol */
		goto error;
	}
#ifndef CONFIG_IPV6
	if (ah->pln != IP_ADDR_LEN) {
		/* unsupported protocol address length */
		goto error;
	}
#endif
	sha = ah->data;
	spa = sha + ah->hln;
	tha = spa + ah->pln;
	tpa = tha + ah->hln;

	switch (ah->op) {
	case ARPOP_REQUEST:
#ifdef CONFIG_IPV6
		if (ah->pln == IP6_ADDR_LEN) {
			for (i = 0; i < IP6_ADDR_LEN; i++) {
				if (iface->ip6_addr[i] != tha[i]) {
					goto error;
				}
			}
			arp6_add_entry(sha, spa, iface);
			arp6_output(out, ARPOP_REPLY, iface, tha, tpa);
			break;
		}
#endif
		/* assuming that MAC address has been checked by the NIC */
		for (i = 0; i < IP_ADDR_LEN; i++) {
			if (iface->ip4_addr[i] != tpa[i]) {
				goto error;
			}
		}
		arp_add_entry(sha, spa, iface);
		arp_output(iface, ARPOP_REPLY, sha, spa);
		break;

	case ARPOP_REPLY:
#ifdef CONFIG_IPV6
		if (ah->pln == IP6_ADDR_LEN) {
			arp6_add_entry(sha, spa, iface);
			break;
		}
#endif
		arp_add_entry(sha, spa, iface);
		break;

	default:
		goto error;
		/* unsupported ARP opcode */
	}

	pkt_free(pkt);
	return;
 error:
	pkt_free(pkt);
	/* inc stats */
	return;
}

#ifdef CONFIG_ARP_EXPIRY
static void arp_flush(void)
{
	/* TODO */
}
#endif

#ifdef TEST
arp_entries_t *arp_get_entries(void)
{
	return &arp_entries;
}
#endif
