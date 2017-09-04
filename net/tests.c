#include "config.h"
#include "tests.h"
#include "arp.h"
#include "eth.h"
#include "pkt-mempool.h"

uint16_t send(const buf_t *out)
{
	(void)out;
	return 0;
}

uint16_t recv(buf_t *in)
{
	(void)in;
	return 0;
}

iface_t iface = {
	.flags = IFF_UP|IFF_RUNNING,
	.mac_addr = { 0x54, 0x52, 0x00, 0x02, 0x00, 0x40 },
	.ip4_addr = { 192, 168, 2, 32 },
	.send = &send,
};

unsigned char arp_request_pkt[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0x4D, 0x7E,
	0xE4, 0xDA, 0x65, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00,
	0x06, 0x04, 0x00, 0x01, 0x48, 0x4D, 0x7E, 0xE4, 0xDA,
	0x65, 0xC0, 0xA8, 0x02, 0xA3, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC0, 0xA8, 0x02, 0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
unsigned char arp_reply_pkt[] = {
	0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65, 0x54, 0x52, 0x00,
	0x02, 0x00, 0x40, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00,
	0x06, 0x04, 0x00, 0x02, 0x54, 0x52, 0x00, 0x02, 0x00,
	0x40, 0xc0, 0xa8, 0x02, 0x20, 0x48, 0x4d, 0x7e, 0xe4,
	0xda, 0x65, 0xc0, 0xa8, 0x02, 0xa3
};

/* mac_src: 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65,
 * mac_dst: 0xe8, 0x39, 0x35, 0x10, 0xfc, 0xed
 * ip_src:  192.168.2.163
 * ip_dst:  172.217.22.131
 */
unsigned char icmp_echo_pkt[] = {
	0xe8, 0x39, 0x35, 0x10, 0xfc, 0xed, 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65,
	0x08, 0x00, 0x45, 0x00, 0x00, 0x54, 0xc6, 0x5c, 0x40, 0x00, 0x40, 0x01,
	0xed, 0xa4, 0xc0, 0xa8, 0x02, 0xa3, 0xac, 0xd9, 0x16, 0x83, 0x08, 0x00,
	0xfd, 0x94, 0x03, 0xdd, 0x00, 0x01, 0xa6, 0xb7, 0x78, 0x59, 0x00, 0x00,
	0x00, 0x00, 0x14, 0xa9, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x36, 0x37
};

unsigned char icmp_reply_pkt[] = {
	0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65, 0xe8, 0x39, 0x35, 0x10, 0xfc, 0xed,
	0x08, 0x00, 0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x38, 0x01,
	0xfc, 0x01, 0xac, 0xd9, 0x16, 0x83, 0xc0, 0xa8, 0x02, 0xa3, 0x00, 0x00,
	0x05, 0x95, 0x03, 0xdd, 0x00, 0x01, 0xa6, 0xb7, 0x78, 0x59, 0x00, 0x00,
	0x00, 0x00, 0x14, 0xa9, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x36, 0x37
};

static void net_arp_print_entries(void)
{
	arp_entries_t *arp_entries = arp_get_entries();
	int i, j;

	for (i = 0; i < CONFIG_ARP_TABLE_SIZE; i++) {
		printf("ip:\n 0x%X", arp_entries->entries[i].ip);
		puts("\nmac:");
		for (j = 0; j < ETHER_ADDR_LEN; j++) {
			printf(" 0x%X", arp_entries->entries[i].mac[j]);
		}
		puts("");
	}
}

static int net_arp_request_test(pkt_t *pkt, iface_t ifa)
{
	uint8_t dst_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t dst_ip = 0xC0A80220;
#else
	uint32_t dst_ip = 0x2002A8C0;
#endif
	uint8_t src_mac[] = { 0x48, 0x4D, 0x7E, 0xE4, 0xDA, 0x65 };
	uint8_t src_ip[] = { 192, 168, 2, 163 };
	int i;
	pkt_t *out;

	memcpy(ifa.mac_addr, src_mac, 6);
	memcpy(ifa.ip4_addr, src_ip, 4);

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "can't alloc a packet\n");
		return -1;
	}
	memset(pkt->buf.data, 0, pkt->buf.size);

	arp_output(&ifa, ARPOP_REQUEST, dst_mac, (uint8_t *)&dst_ip);
	if ((out = pkt_get(&ifa.tx)) == NULL) {
		fprintf(stderr, "can't get tx packet\n");
		return -1;
	}
	for (i = 0; i < pkt->buf.len; i++) {
		if (pkt->buf.data[i] != out->buf.data[i]) {
			fprintf(stderr, "failed serializing arp request\n");
			printf("got:\n");
			buf_print(out->buf);
			printf("expected:\n");
			buf_print(pkt->buf);
			puts("");
			return -1;
		}
	}
	pkt_free(out);
	return 0;
}

int net_arp_tests(void)
{
	int ret = 0, i;
	pkt_t *pkt;
	buf_t out;
	/* ip addr 192.168.2.163 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip = 0xC0A802A3;
#else
	uint32_t ip = 0xA302A8C0;
#endif
	uint8_t mac_dst[] = { 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65 };
	uint8_t *mac = NULL;
	iface_t *interface = NULL;

	if (if_init(&iface, &send, &recv) < 0) {
		fprintf(stderr, "can't init interface\n");
		return -1;
	}

	if (pkt_mempool_init() < 0) {
		fprintf(stderr, "can't initialize pkt pool\n");
		return -1;
	}
	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "can't alloc a packet\n");
		return -1;
	}
	arp_init();
	memset(pkt->buf.data, 0, pkt->buf.size);
	buf_init(&pkt->buf, arp_request_pkt, sizeof(arp_request_pkt));

	/* printf("in pkt:\n"); */
	/* buf_print(pkt->buf); */
	if (pkt_put(&iface.rx, pkt) < 0) {
		fprintf(stderr , "can't put rx packet\n");
		return -1;
	}
	if ((pkt = pkt_get(&iface.rx)) == NULL) {
		fprintf(stderr , "can't get rx packet\n");
		return -1;
	}

	eth_input(pkt, &iface);
	(void)net_arp_print_entries;

	buf_init(&out, arp_reply_pkt, sizeof(arp_reply_pkt));
	if ((pkt = pkt_get(&iface.tx)) == NULL) {
		fprintf(stderr, "can't get tx packet\n");
		return -1;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("out pkt:\n");
		buf_print(pkt->buf);
		printf("expected:\n");
		buf_print(out);
		ret = -1;
		goto end;
	}
	if (arp_find_entry(ip, &mac, &interface) < 0) {
		fprintf(stderr, "failed find arp entry\n");
		ret = -1;
		goto end;
	}
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	if (interface != &iface) {
		fprintf(stderr, "bad interface\n");
		ret = -1;
		goto end;
	}
#endif
	if (mac == NULL) {
		fprintf(stderr, "mac address is null\n");
		ret = -1;
		goto end;
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (mac[i] != mac_dst[i]) {
			fprintf(stderr, "bad mac address\n");
			ret = -1;
			goto end;
		}
	}
	if (net_arp_request_test(pkt, iface) < 0)
		ret = -1;
 end:
	pkt_free(pkt);
	return ret;
}

/* mac_src: 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65,
 * mac_dst: 0xe8, 0x39, 0x35, 0x10, 0xfc, 0xed
 * ip_src:  192.168.2.163
 * ip_dst:  172.217.22.131
 */

int net_icmp_tests(void)
{
	pkt_t *pkt;
	buf_t out;
	/* ip_src:  172.217.22.131 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip_src = 0xACD91683;
#else
	uint32_t ip_src = 0x8316D9AC;
#endif
	/* ip_dst:  192.168.2.163 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip_dst = 0xC0A802A3;
#else
	uint32_t ip_dst = 0xA302A8C0;
#endif
	uint8_t mac_src[] = { 0xe8, 0x39, 0x35, 0x10, 0xfc, 0xed };
	uint8_t mac_dst[] = { 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65 };

	memcpy(iface.ip4_addr, &ip_src, sizeof(uint32_t));
	memcpy(iface.mac_addr, &mac_src, ETHER_ADDR_LEN);
	if (if_init(&iface, &send, &recv) < 0) {
		fprintf(stderr, "can't init interface\n");
		return -1;
	}
	pkt_mempool_shutdown();
	if (pkt_mempool_init() < 0) {
		fprintf(stderr, "can't initialize pkt pool\n");
		return -1;
	}

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "can't alloc a packet\n");
		return -1;
	}
	memset(pkt->buf.data, 0, pkt->buf.size);
	buf_init(&pkt->buf, icmp_echo_pkt, sizeof(icmp_echo_pkt));

	arp_add_entry(mac_dst, (uint8_t *)&ip_dst, &iface);
	if (pkt_put(&iface.rx, pkt) < 0) {
		fprintf(stderr , "can't put rx packet\n");
		return -1;
	}
	if ((pkt = pkt_get(&iface.rx)) == NULL) {
		fprintf(stderr , "can't get rx packet\n");
		return -1;
	}
	eth_input(pkt, &iface);

	buf_init(&out, icmp_reply_pkt, sizeof(icmp_reply_pkt));
	if ((pkt = pkt_get(&iface.tx)) == NULL) {
		fprintf(stderr, "can't get tx packet\n");
		return -1;
	}
	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("out pkt:\n");
		buf_print(pkt->buf);
		printf("expected:\n");
		buf_print(out);
		pkt_free(pkt);
		return -1;
	}
	pkt_mempool_shutdown();
	return 0;
}
