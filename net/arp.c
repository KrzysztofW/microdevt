#include "arp.h"
#include "eth.h"
#include "ip.h"
#include <timer.h>

arp_entries_t arp_entries;
#ifdef CONFIG_IPV6
arp6_entries_t arp6_entries;
#endif

#ifdef CONFIG_ARP_EXPIRY
tim_t arp_timer;
#endif

uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define ARP_RETRY_TIMEOUT 3 /* seconds */
#define ARP_RETRIES 2

struct arp_res {
	list_t list;
	list_t pkt_list;
	tim_t tim;
	const iface_t *iface;
	uint8_t retries;
} __attribute__((__packed__));
typedef struct arp_res arp_res_t;

list_t arp_wait_list = LIST_HEAD_INIT(arp_wait_list);

int
arp_find_entry(const uint32_t *ip, const uint8_t **mac, const iface_t **iface)
{
	int i;

	/* linear search ... that's bad but it saves space */
	for (i = 0; i < CONFIG_ARP_TABLE_SIZE; i++) {
		if (arp_entries.entries[i].ip == *ip) {
			*mac = arp_entries.entries[i].mac;
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
			*iface = arp_entries.entries[i].iface;
#else
			(void)iface;
#endif
			return 0;
		}
	}
	return -1;
}

void arp_add_entry(const uint8_t *sha, const uint8_t *spa, const iface_t *iface)
{
	int i;
	arp_entry_t *e = &arp_entries.entries[arp_entries.pos];
	uint8_t *ip = (uint8_t *)&e->ip;

	STATIC_ASSERT(POWEROF2(CONFIG_ARP_TABLE_SIZE));

	for (i = 0; i < IP_ADDR_LEN; i++)
		ip[i] = spa[i];

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		e->mac[i] = sha[i];

#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	e->iface = iface;
#else
	(void)iface;
#endif
	arp_entries.pos = (arp_entries.pos + 1) & (CONFIG_ARP_TABLE_SIZE - 1);
}

#ifdef CONFIG_IPV6
static void
arp6_add_entry(const uint8_t *sha, const uint8_t *spa, iface_t *iface)
{
	int i;
	arp6_entry_t *e = arp6_entries.entries[arp6_entries.pos];

	for (i = 0; i < IP6_ADDR_LEN; i++)
		e->ip[i] = spa[i];

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		e->mac[i] = sha[i];

#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	e->iface = iface;
#endif
	arp6_entries.pos = (arp6_entries.pos + 1) & (CONFIG_ARP_TABLE_SIZE-1);
}
#endif

int
arp_output(const iface_t *iface, int op, const uint8_t *tha, const uint8_t *tpa)
{
	int i;
	pkt_t *out;
	arp_hdr_t *ah;
	uint8_t *data;
	uint8_t arp_hdr_len;

	if ((out = pkt_alloc()) == NULL
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	    && (out = pkt_alloc_emergency()) == NULL
#endif
	    ) {
		/* inc stats */
		return -1;
	}
	arp_hdr_len = ETHER_ADDR_LEN * 2 + IP_ADDR_LEN * 2;

	pkt_adj(out, (int)sizeof(eth_hdr_t));
	ah = btod(out);
	pkt_adj(out, (int)sizeof(arp_hdr_t));
	ah->hrd = ARPHRD_ETHER;
	ah->proto = ETHERTYPE_IP;
	ah->hln = ETHER_ADDR_LEN;
	ah->pln = IP_ADDR_LEN;
	ah->op = op;
	data = ah->data;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		*(data + i) = iface->hw_addr[i];
		if (i < IP_ADDR_LEN) {
			*(data + i + ETHER_ADDR_LEN) = iface->ip4_addr[i];
			*(data + i + ETHER_ADDR_LEN * 2 + IP_ADDR_LEN) = tpa[i];
		}
		if (tha[0] == 0xFF)
			*(data + i + ETHER_ADDR_LEN + IP_ADDR_LEN) = 0;
		else
			*(data + i + ETHER_ADDR_LEN + IP_ADDR_LEN) = tha[i];
	}
	pkt_adj(out, arp_hdr_len);
	pkt_adj(out, -((int)sizeof(arp_hdr_t) + arp_hdr_len));
	return eth_output(out, iface, L3_PROTO_ARP, tha);
}

static uint32_t *arp_res_get_ip(arp_res_t *arp_res)
{
	pkt_t *pkt = list_first_entry(&arp_res->pkt_list, pkt_t, list);
	ip_hdr_t *ip_hdr = btod(pkt);

	return &ip_hdr->dst;
}

static arp_res_t *arp_res_lookup(const uint32_t *ip)
{
	arp_res_t *arp_res;

	list_for_each_entry(arp_res, &arp_wait_list, list) {
		uint32_t *arp_res_ip = arp_res_get_ip(arp_res);

		if (*arp_res_ip == *ip)
			return arp_res;
	}
	return NULL;
}

static void __arp_process_wait_list(arp_res_t *arp_res, uint8_t delete)
{
	pkt_t *pkt, *pkt_tmp;

	list_for_each_entry_safe(pkt, pkt_tmp, &arp_res->pkt_list, list) {
		list_del(&pkt->list);
		if (delete)
			pkt_free(pkt);
		else {
			ip_hdr_t *ip_hdr = btod(pkt);

			eth_output(pkt, arp_res->iface, ETHERTYPE_IP,
				   &ip_hdr->dst);
		}
	}
	timer_del(&arp_res->tim);
	list_del(&arp_res->list);
	free(arp_res);
}

static void arp_process_wait_list(uint32_t *ip, uint8_t delete)
{
	arp_res_t *arp_res;

	if ((arp_res = arp_res_lookup(ip)) == NULL)
		return;
	__arp_process_wait_list(arp_res, delete);
}

void arp_input(pkt_t *pkt, const iface_t *iface)
{
	uint8_t *sha, *spa, *tha, *tpa;
	arp_hdr_t *ah = btod(pkt);
	int i;

	if (ah->hrd != ARPHRD_ETHER) {
		/* unsupported ether hw address */
		goto end;
	}
	if (ah->proto != ETHERTYPE_IP) {
		/* unsupported protocol */
		goto end;
	}
#ifndef CONFIG_IPV6
	if (ah->pln != IP_ADDR_LEN) {
		/* unsupported protocol address length */
		goto end;
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
				if (iface->ip6_addr[i] != tha[i])
					goto end;
			}
			arp6_add_entry(sha, spa, iface);
			arp6_output(out, ARPOP_REPLY, iface, tha, tpa);
			break;
		}
#endif
		/* assuming that MAC address has been checked by the NIC */
		for (i = 0; i < IP_ADDR_LEN; i++) {
			if (iface->ip4_addr[i] != tpa[i])
				goto end;
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
		arp_process_wait_list((uint32_t *)spa, 0);
		break;

	default:
		goto end;
		/* unsupported ARP opcode */
	}

 end:
	pkt_free(pkt);
}

void arp_retry_cb(void *arg)
{
	arp_res_t *arp_res = arg;
	uint32_t *ip;

	arp_res->retries++;
	if (arp_res->retries >= ARP_RETRIES) {
		__arp_process_wait_list(arp_res, 1);
		return;
	}
	timer_reschedule(&arp_res->tim, ARP_RETRY_TIMEOUT * 1000000);
	ip = arp_res_get_ip(arp_res);
	arp_output(arp_res->iface, ARPOP_REQUEST, broadcast_mac, (uint8_t *)ip);
}

void arp_resolve(pkt_t *pkt, const uint32_t *ip_dst, const iface_t *iface)
{
	arp_res_t *arp_res;
#ifdef CONFIG_TCP_RETRANSMIT
	ip_hdr_t *ip_hdr = btod(pkt);
#endif

	arp_output(iface, ARPOP_REQUEST, broadcast_mac, (uint8_t *)ip_dst);

#ifdef CONFIG_TCP_RETRANSMIT
	if (ip_hdr->p == IPPROTO_TCP) {
		pkt_free(pkt);
		return;
	}
#endif
	if ((arp_res = arp_res_lookup(ip_dst))) {
		list_add_tail(&pkt->list, &arp_res->pkt_list);
		return;
	}

	if ((arp_res = malloc(sizeof(arp_res_t))) == NULL) {
		pkt_free(pkt);
		return;
	}
	memset(&arp_res->tim, 0, sizeof(tim_t));
	INIT_LIST_HEAD(&arp_res->list);
	INIT_LIST_HEAD(&arp_res->pkt_list);
	timer_init(&arp_res->tim);
	list_add_tail(&pkt->list, &arp_res->pkt_list);
	arp_res->retries = 0;
	arp_res->iface = iface;
	timer_add(&arp_res->tim, ARP_RETRY_TIMEOUT * 1000000, arp_retry_cb,
		  arp_res);
	/* add a hash table on ip_dst if we have more RAM */
	list_add_tail(&arp_res->list, &arp_wait_list);
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
