#include "config.h"
#include "tests.h"

iface_t iface = {
	.flags = IFF_UP|IFF_RUNNING,
	.mac_addr = { 0x62, 0x5F, 0x70, 0x72, 0x61, 0x79 },
	.ip4_addr = { 192, 168, 0, 99 },
};

static int net_pkt_parse(buf_t pkt)
{
	eth_input(pkt, &iface);
	return 0;
}

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

int net_tests(void)
{
	buf_t pkt;

	if (buf_read_file(&pkt, "net/arp.raw") < 0)
		return -1;
	buf_print(pkt);
	net_pkt_parse(pkt);
	net_arp_print_entries();
	buf_free(&pkt);
	return 0;
}

