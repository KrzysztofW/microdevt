#include "config.h"
#include "tests.h"
#include "arp.h"
#include "eth.h"
#include "udp.h"
#include "route.h"
#include "socket.h"
#include "pkt-mempool.h"

void recv(const iface_t *iface) {}

static int send(const struct iface *iface, pkt_t *pkt)
{
	if (pkt_put(iface->tx, pkt) < 0)
		return -1;
	return 0;
}

static uint8_t ip[] = { 192, 168, 2, 32 };
static uint8_t ip_mask[] = { 255, 255, 255, 0 };
static uint8_t mac[] = { 0x54, 0x52, 0x00, 0x02, 0x00, 0x40 };

static iface_t iface = {
	.flags = IF_UP|IF_RUNNING,
	.hw_addr = mac,
	.ip4_addr = ip,
	.ip4_mask = ip_mask,
	.send = &send,
	.recv = &recv,
};

static unsigned char arp_request_pkt[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0x4D, 0x7E,
	0xE4, 0xDA, 0x65, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00,
	0x06, 0x04, 0x00, 0x01, 0x48, 0x4D, 0x7E, 0xE4, 0xDA,
	0x65, 0xC0, 0xA8, 0x02, 0xA3, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xC0, 0xA8, 0x02, 0x20
};
static unsigned char arp_reply_pkt[] = {
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
static unsigned char icmp_echo_pkt[] = {
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

static unsigned char icmp_reply_pkt[] = {
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

static int net_arp_request_test(iface_t *ifa)
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
	pkt_t *out, *pkt;

	ifa->hw_addr = src_mac;
	ifa->ip4_addr = src_ip;

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "%s: can't alloc a packet\n", __func__);
		return -1;
	}
	buf_init(&pkt->buf, arp_request_pkt, sizeof(arp_request_pkt));

	if (arp_output(ifa, ARPOP_REQUEST, dst_mac, (uint8_t *)&dst_ip) < 0) {
		fprintf(stderr, "%s:%d failed\n", __func__, __LINE__);
		return -1;
	}
	if ((out = pkt_get(ifa->tx)) == NULL) {
		fprintf(stderr, "%s: can't get tx packet\n", __func__);
		return -1;
	}
	if (out->buf.len == 0) {
		fprintf(stderr, "%s: got empty packet\n", __func__);
		return -1;
	}
	for (i = 0; i < out->buf.len; i++) {
		if (pkt->buf.data[i] != out->buf.data[i]) {
			fprintf(stderr, "%s: failed serializing arp request\n",
				__func__);
			printf("got:\n");
			buf_print(&out->buf);
			printf("expected:\n");
			buf_print(&pkt->buf);
			puts("");
			return -1;
		}
	}
	pkt_free(out);
	return 0;
}

int net_arp_tests(void)
{
	int i, ret = 0;
	pkt_t *pkt;
	buf_t out;
	/* ip addr 192.168.2.163 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip = 0xC0A802A3;
#else
	uint32_t ip = 0xA302A8C0;
#endif
	uint8_t mac_dst[] = { 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65 };
	const uint8_t *mac = NULL;
	const iface_t *interface = NULL;

	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	if_init(&iface, IF_TYPE_ETHERNET, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 0);

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "%s: can't alloc a packet\n", __func__);
		ret = -1;
		goto end;
	}
	memset(pkt->buf.data, 0, pkt->buf.size);
	buf_init(&pkt->buf, arp_request_pkt, sizeof(arp_request_pkt));

	printf("in pkt:\n");
	buf_print_hex(&pkt->buf);
	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end;
	}

	eth_input(&iface);
	(void)net_arp_print_entries;

	buf_init(&out, arp_reply_pkt, sizeof(arp_reply_pkt));
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: can't get tx packet 2\n", __func__);
		ret = -1;
		goto end;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		goto end;
	}
	if (arp_find_entry(&ip, &mac, &interface) < 0) {
		fprintf(stderr, "%s: failed find arp entry\n", __func__);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
#ifdef CONFIG_MORE_THAN_ONE_INTERFACE
	if (interface != &iface) {
		fprintf(stderr, "%s: bad interface\n", __func__);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
#endif
	if (mac == NULL) {
		fprintf(stderr, "%s: mac address is null\n", __func__);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (mac[i] != mac_dst[i]) {
			fprintf(stderr, "%s: bad mac address\n", __func__);
			pkt_free(pkt);
			ret = -1;
			goto end;
		}
	}
	if (net_arp_request_test(&iface) < 0)
		ret = -1;

 end:
	pkt_mempool_shutdown();
	if_shutdown(&iface);
	return ret;
}

/* mac_src: 0x48, 0x4d, 0x7e, 0xe4, 0xda, 0x65,
 * mac_dst: 0xe8, 0x39, 0x35, 0x10, 0xfc, 0xed
 * ip_src:  192.168.2.163
 * ip_dst:  172.217.22.131
 */

int net_icmp_tests(void)
{
	int ret = 0;
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

	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	iface.ip4_addr = (void *)&ip_src;
	iface.hw_addr = mac_src;
	if_init(&iface, IF_TYPE_ETHERNET, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 0);

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "%s: can't alloc a packet\n", __func__);
		ret = -1;
		goto end;
	}
	memset(pkt->buf.data, 0, pkt->buf.size);
	buf_init(&pkt->buf, icmp_echo_pkt, sizeof(icmp_echo_pkt));

	arp_add_entry(mac_dst, (uint8_t *)&ip_dst, &iface);
	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}

	eth_input(&iface);

	buf_init(&out, icmp_reply_pkt, sizeof(icmp_reply_pkt));
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: can't get tx packet\n", __func__);
		ret = -1;
		goto end;
	}
	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}

 end:
	pkt_mempool_shutdown();
	if_shutdown(&iface);
	return ret;
}

unsigned char udp_pkt[] = {
	0x48, 0x83, 0xc7, 0xbc, 0x7d, 0x06, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x29, 0xdd, 0x22, 0x40, 0x00, 0x40, 0x11, 0xdc, 0x44, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x01, 0xbb, 0x1b, 0x03, 0x09, 0x00, 0x15, 0x55, 0xe2, 0x62, 0x6c, 0x61, 0x62, 0x6c, 0x61, 0x62, 0x6c, 0x61, 0x62, 0x6c, 0x61, 0x0a,
};

/* icmp answer to udp_pkt above */
unsigned char icmp_port_unrecheable_pkt[] = {
	/* modified icmp pkt to match checksums & sizes */
	0x9C, 0xD6, 0x43, 0xAE, 0x22, 0x6C, 0x48, 0x83, 0xC7, 0xBC, 0x7D, 0x06, 0x08, 0x00, 0x45, 0x00, 0x00, 0x45, 0x00, 0x00, 0x40, 0x00, 0x38, 0x01, 0xC1, 0x5B, 0xC0, 0xA8, 0x00, 0x01, 0xC0, 0xA8, 0x00, 0x0B, 0x03, 0x03, 0x7E, 0x80, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x29, 0xDD, 0x22, 0x40, 0x00, 0x40, 0x11, 0xDC, 0x44, 0xC0, 0xA8, 0x00, 0x0B, 0xC0, 0xA8, 0x00, 0x01, 0xBB, 0x1B, 0x03, 0x09, 0x00, 0x15, 0x55, 0xE2, 0x62, 0x6C, 0x61, 0x62, 0x6C, 0x61, 0x62, 0x6C, 0x61, 0x62, 0x6C, 0x61, 0x0A,

	/* unused original icmp response pkt */
	/*	0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x48, 0x83, 0xc7, 0xbc, 0x7d, 0x06, 0x08, 0x00, 0x45, 0x00, 0x00, 0x38, 0xf6, 0xe8, 0x40, 0x00, 0x40, 0x01, 0xc2, 0x7f, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0x0b, 0x03, 0x03, 0x3e, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x29, 0xdd, 0x22, 0x40, 0x00, 0x40, 0x11, 0xdc, 0x44, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x01, 0xbb, 0x1b, 0x03, 0x09, 0x00, 0x15, 0x00, 0x00, 0x20, 0x08, */
};

#ifdef CONFIG_UDP
#ifdef CONFIG_BSD_COMPAT
static int udp_fd;
static int udp_server(uint16_t port)
{
	int fd;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "%s: can't create socket\n", __func__);
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "%s: can't bind\n", __func__);
		return -1;
	}
	return fd;
}
#endif
int net_udp_tests(void)
{
	pkt_t *pkt;
	buf_t out;
	/* ip_src: 192.168.0.11 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip_src = 0xc0a8000b;
#else
	uint32_t ip_src = 0x0b00a8c0;
#endif
	/* ip_dst: 192.168.0.1 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip_dst = 0xc0a80001;
#else
	uint32_t ip_dst = 0x0100a8c0;
#endif
	uint8_t mac_dst[] = { 0x48, 0x83, 0xc7, 0xbc, 0x7d, 0x06 };
	uint8_t mac_src[] = { 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c };
	uint16_t port = 777;
#ifdef CONFIG_BSD_COMPAT
	struct sockaddr_in addr;
	socklen_t addrlen;
#else
	sock_info_t sock_info;
	uint32_t src_addr;
	uint16_t src_port;
#endif
	sbuf_t sb;
	int buf_size = 512, len, ret = 0;
	char buf[buf_size];

	memset(buf, 0, buf_size);
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	iface.ip4_addr = (void *)&ip_dst;
	iface.hw_addr = mac_dst;
	if_init(&iface, IF_TYPE_ETHERNET, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 0);

	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "%s: can't alloc a packet\n", __func__);
		ret = -1;
		goto end;
	}
	memset(pkt->buf.data, 0, pkt->buf.size);
	buf_init(&pkt->buf, udp_pkt, sizeof(udp_pkt));

#ifdef CONFIG_HT_STORAGE
	if (socket_init() < 0) {
		fprintf(stderr, "%s: can't init socket hash tables\n", __func__);
		ret = -1;
		pkt_free(pkt);
		goto end;
	}
#endif

	arp_add_entry(mac_src, (uint8_t *)&ip_src, &iface);

	(void)out;
#ifdef CONFIG_ICMP
	buf_init(&out, icmp_port_unrecheable_pkt,
		 sizeof(icmp_port_unrecheable_pkt));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: can't get tx packet\n", __func__);
		ret = -1;
		goto end;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
#endif
	dft_route.iface = &iface;
#ifdef CONFIG_BSD_COMPAT
	if ((udp_fd = udp_server(port)) < 0) {
		fprintf(stderr, "%s: can't start udp server\n", __func__);
		ret = -1;
		pkt_free(pkt);
		goto end;
	}
#else
	if (sock_info_init(&sock_info, 0, SOCK_DGRAM, htons(port)) < 0) {
		fprintf(stderr, "%s: can't init udp sock_info\n", __func__);
		return -1;
	}

	__sock_info_add(&sock_info);
	if (sock_info_bind(&sock_info) < 0) {
		fprintf(stderr, "%s: can't start udp server\n", __func__);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
#endif

	buf_init(&pkt->buf, udp_pkt, sizeof(udp_pkt));
	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		pkt_free(pkt);
		goto end;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) != NULL) {
		fprintf(stderr, "shouldn't get tx packet\n");
		ret = -1;
		goto end;
	}
#ifdef CONFIG_BSD_COMPAT
	if ((len = recvfrom(udp_fd, buf, buf_size, 0, (struct sockaddr *)&addr,
			    &addrlen)) < 0) {
		fprintf(stderr, "%s: can't get udp pkt\n", __func__);
		ret = -1;
		goto end;
	}
	printf("udp data: %.*s\n", len, buf);
#else
	if (__socket_get_pkt(&sock_info, &pkt, &src_addr, &src_port) < 0) {
		fprintf(stderr, "%s: can't get udp pkt 2\n", __func__);
		ret = -1;
		goto end;
	}
	len = pkt->buf.len;
	printf("udp data: %.*s\n", pkt->buf.len, buf_data(&pkt->buf));
#endif

	sbuf_init(&sb, buf, len);
#ifdef CONFIG_BSD_COMPAT
	if (sendto(udp_fd, sb.data, sb.len, 0, (struct sockaddr *)&addr,
		   sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "%s: can't put sbuf to udp socket\n", __func__);
		ret = -1;
		pkt_free(pkt);
		goto end;
	}
#else
	if (__socket_put_sbuf(&sock_info, &sb, src_port, src_addr) < 0) {
		fprintf(stderr, "%s: can't put sbuf to udp socket\n", __func__);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
#endif
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: can't get udp echo packet\n", __func__);
		ret = -1;
		goto end;
	}

	buf_print_hex(&pkt->buf);
	pkt_free(pkt);

#ifdef CONFIG_BSD_COMPAT
	if (close(udp_fd) < 0) {
		fprintf(stderr, "%s: can't close udp socket\n", __func__);
		ret = -1;
		goto end;
	}
#else
	if (sock_info_unbind(&sock_info) < 0) {
		fprintf(stderr, "%s: can't close udp socket\n", __func__);
		ret = -1;
		goto end;
	}
#endif
 end:
	if_shutdown(&iface);
	socket_shutdown();
	pkt_mempool_shutdown();
	return ret;
}
#endif
#ifdef CONFIG_TCP
/* SYN => RST */
unsigned char tcp_pkt[] = {
	0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x3c, 0xf7, 0x56, 0x40, 0x00, 0x40, 0x06, 0xc1, 0xc5, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x44, 0xce, 0x5c, 0x03, 0x09, 0xff, 0x0e, 0x4c, 0xe1, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x72, 0x10, 0x7b, 0x05, 0x00, 0x00, 0x02, 0x04, 0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x00, 0x3d, 0xbb, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x07,
};

unsigned char tcp_pkt_rst_reply[] = {
	0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0x38, 0x06, 0xC1, 0x30, 0xc0, 0xa8, 0x00, 0x44, 0xc0, 0xa8, 0x00, 0x0b, 0x03, 0x09, 0xce, 0x5c, 0x00, 0x00, 0x00, 0x00, 0xff, 0x0e, 0x4c, 0xe2, 0x50, 0x14, 0x00, 0x00, 0x10, 0xda, 0x00, 0x00,
};

/* COMPLETE TCP COMM */
unsigned char tcp_syn_pkt[] = {
	0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x3c, 0x9c, 0x3a, 0x40, 0x00, 0x40, 0x06, 0x1c, 0xe2, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x44, 0xce, 0x18, 0x03, 0x09, 0x76, 0xde, 0x61, 0x18, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02, 0x72, 0x10, 0xad, 0xde, 0x00, 0x00, 0x02, 0x04, 0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x00, 0x36, 0xfd, 0x22, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x07,
};

unsigned char tcp_syn_ack_pkt[] = {
	0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x08, 0x00, 0x45, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x40, 0x00, 0x38, 0x06, 0xC1, 0x2c, 0xc0, 0xa8, 0x00, 0x44, 0xc0, 0xa8, 0x00, 0x0b, 0x03, 0x09, 0xce, 0x18, 0x7b, 0xe7, 0x07, 0x12, 0x76, 0xde, 0x61, 0x19, 0x60, 0x12, 0x01, 0x00, 0xEE, 0xD1, 0x00, 0x00, 0x02, 0x04, 0x00, 0x46,
};

unsigned char tcp_ack_pkt[] = {
	0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x34, 0x9c, 0x3b, 0x40, 0x00, 0x40, 0x06, 0x1c, 0xe9, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x44, 0xce, 0x18, 0x03, 0x09, 0x76, 0xde, 0x61, 0x19, 0x7b, 0xe7, 0x07, 0x13, 0x80, 0x10, 0x00, 0xe5, 0x9f, 0xc5, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x00, 0x36, 0xfd, 0x22, 0x00, 0xb4, 0x2a, 0x52,
};

unsigned char tcp_data_pkt[] = {
	0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x3b, 0x9c, 0x3c, 0x40, 0x00, 0x40, 0x06, 0x1c, 0xe1, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x44, 0xce, 0x18, 0x03, 0x09, 0x76, 0xde, 0x61, 0x19, 0x7b, 0xe7, 0x07, 0x13, 0x80, 0x18, 0x00, 0xe5, 0x65, 0x86, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x00, 0x36, 0xfd, 0x22, 0x00, 0xb4, 0x2a, 0x52, 0x62, 0x6c, 0x61, 0x62, 0x6c, 0x61, 0x0a,
};

unsigned char tcp_data_ack_pkt[] = {
	0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0x38, 0x06, 0xC1, 0x30, 0xc0, 0xa8, 0x00, 0x44, 0xc0, 0xa8, 0x00, 0x0b, 0x03, 0x09, 0xce, 0x18, 0x7b, 0xe7, 0x07, 0x13, 0x76, 0xde, 0x61, 0x20, 0x50, 0x10, 0x01, 0x00, 0x01, 0x1A, 0x00, 0x00,
};

unsigned char tcp_fin_ack_client_pkt[] = {
	0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x34, 0x9c, 0x3d, 0x40, 0x00, 0x40, 0x06, 0x1c, 0xe7, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x44, 0xce, 0x18, 0x03, 0x09, 0x76, 0xde, 0x61, 0x20, 0x7b, 0xe7, 0x07, 0x13, 0x80, 0x11, 0x00, 0xe5, 0x9f, 0xbd, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x00, 0x36, 0xfd, 0x22, 0x00, 0xb4, 0x2a, 0x52,
};

unsigned char tcp_fin_ack_server_pkt[] = {
	0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0x38, 0x06, 0xC1, 0x30, 0xc0, 0xa8, 0x00, 0x44, 0xc0, 0xa8, 0x00, 0x0b, 0x03, 0x09, 0xce, 0x18, 0x7b, 0xe7, 0x07, 0x13, 0x76, 0xde, 0x61, 0x21, 0x50, 0x11, 0x01, 0x00, 0x01, 0x18, 0x00, 0x00,
};

unsigned char tcp_ack_client_pkt[] = {
	0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba, 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c, 0x08, 0x00, 0x45, 0x00, 0x00, 0x34, 0x9c, 0x3e, 0x40, 0x00, 0x40, 0x06, 0x1c, 0xe6, 0xc0, 0xa8, 0x00, 0x0b, 0xc0, 0xa8, 0x00, 0x44, 0xce, 0x18, 0x03, 0x09, 0x76, 0xde, 0x61, 0x21, 0x7b, 0xe7, 0x07, 0x14, 0x80, 0x10, 0x00, 0xe5, 0x9f, 0xba, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x00, 0x36, 0xfd, 0x23, 0x00, 0xb4, 0x2a, 0x53,
};

#ifdef CONFIG_BSD_COMPAT
static int tcp_fd;
static int tcp_server(uint16_t port)
{
	struct sockaddr_in sockaddr;

	if ((tcp_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "%s: can't create socket\n", __func__);
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(tcp_fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "%s: can't bind tcp port\n", __func__);
		return -1;
	}
	if (listen(tcp_fd, 5) < 0) {
		fprintf(stderr, "%s: can't listen on tcp fd\n", __func__);
		return -1;
	}
	return 0;
}
#endif

int net_tcp_tests(void)
{
	pkt_t *pkt;
	buf_t out;
	int ret = 0;
	/* ip_src: 192.168.0.11 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip_src = 0xc0a8000b;
#else
	uint32_t ip_src = 0x0b00a8c0;
#endif
	/* ip_dst: 192.168.0.1 */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint32_t ip_dst = 0xc0a80044;
#else
	uint32_t ip_dst = 0x4400a8c0;
#endif
	uint8_t mac_dst[] = { 0x00, 0x1c, 0xbf, 0xca, 0x8e, 0xba };
	uint8_t mac_src[] = { 0x9c, 0xd6, 0x43, 0xae, 0x22, 0x6c };
	uint16_t port = 777;
#ifdef CONFIG_BSD_COMPAT
	struct sockaddr_in addr;
	socklen_t addr_len;
	int client_fd;
#else
	sock_info_t sock_info_server;
	sock_info_t sock_info_client;
	uint32_t src_addr;
	uint16_t src_port;
#endif
	sbuf_t sb;

	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	iface.ip4_addr = (void *)&ip_dst;
	iface.hw_addr = mac_dst;
	if_init(&iface, IF_TYPE_ETHERNET, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_NB_MAX, CONFIG_PKT_DRIVER_NB_MAX, 0);

	if ((pkt = pkt_alloc()) == NULL) {
		if_shutdown(&iface);
		fprintf(stderr, "%s: can't alloc a packet\n", __func__);
		return -1;
	}

	socket_init();
	dft_route.iface = &iface;
	arp_add_entry(mac_src, (uint8_t *)&ip_src, &iface);

	/* SYN => RST */
	memset(pkt->buf.data, 0, pkt->buf.size);
	buf_init(&pkt->buf, tcp_pkt, sizeof(tcp_pkt));
	buf_init(&out, tcp_pkt_rst_reply, sizeof(tcp_pkt_rst_reply));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: TCP SYN => RST: can't get tx packet\n",
			__func__);
		ret = -1;
		goto end;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("SYN => RST:\n");
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}

	/* COMPLETE TCP COMM */
#ifdef CONFIG_BSD_COMPAT
	if (tcp_server(port) < 0) {
		fprintf(stderr, "%s: can't start tcp server on port: %d\n",
			__func__, port);
		ret = -1;
		goto end;
	}
#else
	if (sock_info_init(&sock_info_server, 0, SOCK_STREAM, htons(port)) < 0) {
		fprintf(stderr, "%s: can't init tcp sock_info\n", __func__);
		ret = -1;
		goto end;
	}
	__sock_info_add(&sock_info_server);
	if (sock_info_listen(&sock_info_server, 5) < 0) {
		fprintf(stderr, "%s: can't listen on tcp sock_info\n", __func__);
		ret = -1;
		goto end;
	}

	if (sock_info_bind(&sock_info_server) < 0) {
		fprintf(stderr, "%s: can't start tcp server\n", __func__);
		ret = -1;
		goto end;
	}
#endif
	/* SYN => SYN_ACK */
	buf_init(&pkt->buf, tcp_syn_pkt, sizeof(tcp_syn_pkt));
	buf_init(&out, tcp_syn_ack_pkt, sizeof(tcp_syn_ack_pkt));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: TCP SYN: can't get SYN_ACK packet\n",
			__func__);
		ret = -1;
		goto end;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("SYN => SYN_ACK:\n");
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}

	/* SYN_ACK => ACK */
	buf_init(&pkt->buf, tcp_ack_pkt, sizeof(tcp_ack_pkt));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) != NULL) {
		fprintf(stderr, "%s: TCP EST: shouldn't get tx packet after ACK\n",
			__func__);
		ret = -1;
		goto end;
	}

	/* DATA => ACK */
	if ((pkt = pkt_alloc()) == NULL) {
		fprintf(stderr, "%s: can't alloc a packet\n", __func__);
		ret = -1;
		goto end;
	}

	buf_init(&pkt->buf, tcp_data_pkt, sizeof(tcp_data_pkt));
	buf_init(&out, tcp_data_ack_pkt, sizeof(tcp_data_ack_pkt));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: TCP EST: can't get ACK to data packet\n",
			__func__);
		ret = -1;
		goto end;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("DATA => ACK:\n");
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		ret = -1;
		goto end;
	}
	pkt_free(pkt);
#ifdef CONFIG_BSD_COMPAT
	if ((client_fd = accept(tcp_fd,
				(struct sockaddr *)&addr, &addr_len)) < 0) {
		fprintf(stderr, "%s: TCP: cannot accept connections\n", __func__);
		ret = -1;
		goto end2;
	}
	if (socket_get_pkt(client_fd, &pkt, &addr) < 0) {
		fprintf(stderr, "%s: TCP: can't get pkt from a tcp connection\n",
			__func__);
		ret = -1;
		goto end2;
	}
#else
	if (sock_info_accept(&sock_info_server, &sock_info_client, &src_addr,
			     &src_port) < 0) {
		fprintf(stderr, "%s: TCP: cannot accept connections\n", __func__);
		ret = -1;
		goto end2;
	}
	if (__socket_get_pkt(&sock_info_client, &pkt, &src_addr, &src_port) < 0) {
		fprintf(stderr, "%s: TCP: can't get pkt from a tcp connection\n",
			__func__);
		ret = -1;
		goto end;
	}
#endif
	sb = PKT2SBUF(pkt);
	sbuf_print_hex(&sb);
	printf("TCP: got: %.*s\n", sb.len, sb.data);

	/* TCP CLIENT CLOSE */
	buf_init(&pkt->buf, tcp_fin_ack_client_pkt, sizeof(tcp_fin_ack_client_pkt));
	buf_init(&out, tcp_fin_ack_server_pkt, sizeof(tcp_fin_ack_server_pkt));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end2;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) == NULL) {
		fprintf(stderr, "%s: TCP close: can't get FIN from server\n",
			__func__);
		ret = -1;
		goto end2;
	}

	if (buf_cmp(&pkt->buf, &out) < 0) {
		printf("TCP CLIENT CLOSE:\n");
		printf("out pkt:\n");
		buf_print_hex(&pkt->buf);
		printf("expected:\n");
		buf_print_hex(&out);
		pkt_free(pkt);
		ret = -1;
		goto end2;
	}

	buf_init(&pkt->buf, tcp_ack_client_pkt, sizeof(tcp_ack_client_pkt));

	if (pkt_put(iface.rx, pkt) < 0) {
		fprintf(stderr , "%s: can't put rx packet\n", __func__);
		ret = -1;
		goto end2;
	}

	eth_input(&iface);
	if ((pkt = pkt_get(iface.tx)) != NULL) {
		fprintf(stderr, "%s: TCP client close: shouldn't get any pkt "
			"after FIN ACK\n", __func__);
		ret = -1;
		goto end2;
	}

 end2:
#ifdef CONFIG_BSD_COMPAT
	if (close(client_fd) < 0) {
		fprintf(stderr, "%s: can't close tcp client fd\n", __func__);
		ret = -1;
		goto end;
	}
	if (close(tcp_fd) < 0) {
		fprintf(stderr, "%s: can't close tcp server fd\n", __func__);
		ret = -1;
		goto end;
	}
#else
	if (sock_info_unbind(&sock_info_server) < 0) {
		fprintf(stderr, "%s: can't unbind tcp server socket\n", __func__);
		ret = -1;
		goto end;
	}
	if (sock_info_unbind(&sock_info_client) < 0) {
		fprintf(stderr, "%s: can't unbind tcp client socket\n", __func__);
		ret = -1;
		goto end;
	}
#endif
 end:
	socket_shutdown();
	if_shutdown(&iface);
	pkt_mempool_shutdown();

	return ret;
}

#endif
