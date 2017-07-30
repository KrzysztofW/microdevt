#include "config.h"
#include "tests.h"

uint16_t send(const buf_t *out)
{
	(void)out;
	return 0;
}

iface_t iface = {
	.flags = IFF_UP|IFF_RUNNING,
	.mac_addr = { 0x54, 0x52, 0x00, 0x02, 0x00, 0x40 },
	.ip4_addr = { 192, 168, 2, 32 },
	.send = &send,
};

unsigned char arp_request_data[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0x4D, 0x7E,
	0xE4, 0xDA, 0x65, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00,
	0x06, 0x04, 0x00, 0x01, 0x48, 0x4D, 0x7E, 0xE4, 0xDA,
	0x65, 0xC0, 0xA8, 0x02, 0xA3, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC0, 0xA8, 0x02, 0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
unsigned char arp_reply_data[] = {
	0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65, 0x54, 0x52, 0x00,
	0x02, 0x00, 0x40, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00,
	0x06, 0x04, 0x00, 0x02, 0x54, 0x52, 0x00, 0x02, 0x00,
	0x40, 0xc0, 0xa8, 0x02, 0x20, 0x48, 0x4d, 0x7e, 0xe4,
	0xda, 0x65, 0xc0, 0xa8, 0x02, 0xa3
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

int net_arp_request_test(iface_t ifa)
{
	uint8_t dst_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t dst_ip = 0xC0A80220;
#else
	uint32_t dst_ip = 0x2002A8C0;
#endif
	uint8_t src_mac[] = { 0x48, 0x4D, 0x7E, 0xE4, 0xDA, 0x65 };
	uint8_t src_ip[] = { 192, 168, 2, 163 };
	unsigned int i, pkt_len = ifa.tx_buf.len;

	memcpy(ifa.mac_addr, src_mac, 6);
	memcpy(ifa.ip4_addr, src_ip, 4);

	buf_reset(&ifa.tx_buf);
	arp_output(&ifa, ARPOP_REQUEST, dst_mac, (uint8_t *)&dst_ip);
	for (i = 0; i < pkt_len; i++) {
		if (ifa.tx_buf.data[i] != iface.rx_buf.data[i]) {
			fprintf(stderr, "failed serializing arp request\n");
			printf("got:\n");
			buf_print(ifa.tx_buf);
			printf("expected:\n");
			buf_print(iface.rx_buf);
			puts("");
			return -1;
		}
	}
	return 0;
}

int net_arp_tests(void)
{
	int ret = 0, i;
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

	if (buf_alloc(&iface.tx_buf, 1024) < 0)
		return -1;

	buf_init(&iface.rx_buf, arp_request_data, sizeof(arp_request_data));

	/* printf("in pkt:\n"); */
	/* buf_print(iface.rx_buf); */
	eth_input(iface.rx_buf, &iface);
	(void)net_arp_print_entries;

	buf_init(&out, arp_reply_data, sizeof(arp_reply_data));
	if (buf_cmp(&iface.tx_buf, &out) < 0) {
		printf("out pkt:\n");
		buf_print(iface.tx_buf);
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
	if (interface != &iface) {
		fprintf(stderr, "bad interface\n");
		ret = -1;
		goto end;
	}

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
	if (net_arp_request_test(iface) < 0)
		ret = -1;
 end:
	buf_free(&iface.tx_buf);
	return ret;
}
