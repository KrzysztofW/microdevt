#include "arp.h"
#include "../timer.h"

arp_entries_t arp_entries;
#ifdef CONFIG_IPV6
arp6_entries_t arp6_entries;
#endif

#ifdef CONFIG_ARP_EXPIRY
tim_t arp_timer;
#endif

int arp_find_entry(uint32_t ip, uint8_t **mac, iface_t **iface)
{
	int i;

	for (i = 0; i < CONFIG_ARP_TABLE_SIZE; i++) {
		if (arp_entries.entries[i].ip == ip) {
			*mac = arp_entries.entries[i].mac;
			*iface = arp_entries.entries[i].iface;
			return 0;
		}
	}
	return -1;
}

static void arp_add_entry(uint8_t *sha, uint8_t *spa, iface_t *iface)
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

static void arp_serialize_data(buf_t *out, uint32_t data, int len)
{
	const uint8_t *d = (uint8_t *)&data;
	buf_add(out, d, len);
}

void arp_reply(iface_t *iface, uint8_t *tha, uint8_t *tpa)
{
	int i;
	buf_t *out = &iface->tx_buf;

	buf_adj(out, (int)sizeof(eth_hdr_t));
	arp_serialize_data(out, ARPHRD_ETHER, 2);
	arp_serialize_data(out, ETHERTYPE_IP, 2);
	buf_addc(out, ETHER_ADDR_LEN);
	buf_addc(out, IP_ADDR_LEN);

	arp_serialize_data(out, ARPOP_REPLY, 2);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		buf_addc(out, iface->mac_addr[i]);
	}
	for (i = 0; i < IP_ADDR_LEN; i++) {
		buf_addc(out, iface->ip4_addr[i]);
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		buf_addc(out, tha[i]);
	}
	for (i = 0; i < IP_ADDR_LEN; i++) {
		buf_addc(out, tpa[i]);
	}
	eth_output(out, iface, tha);
}

void arp_input(buf_t buf, iface_t *iface)
{
	uint8_t *sha, *spa, *tha, *tpa;
	arp_hdr_t *ah = btod(&buf, arp_hdr_t *);
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
			arp6_reply(out, iface, tha, tpa);
			return;
		}
#endif
		/* assuming that MAC address has been checked by the NIC */
		for (i = 0; i < IP_ADDR_LEN; i++) {
			if (iface->ip4_addr[i] != tpa[i]) {
				goto error;
			}
		}
		arp_add_entry(sha, spa, iface);
		arp_reply(iface, sha, spa);
		return;

	case ARPOP_REPLY:
#ifdef CONFIG_IPV6
		if (ah->pln == IP6_ADDR_LEN) {
			arp6_add_entry(sha, spa, iface);
			return;
		}
#endif
		arp_add_entry(sha, spa, iface);
		return;

	default:
		break;
		/* unsupported ARP opcode */
	}
 error:
	/* inc stats */
	return;
}

void arp_output()
{

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

