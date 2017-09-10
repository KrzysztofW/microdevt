#include "udp.h"
#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "chksum.h"
#include "../sys/hash-tables.h"

/* htable keys (udp ports) are in network byte order */
static hash_table_t *udp_binds;

void udp_output(pkt_t *out, iface_t *iface, const buf_t *buf)
{
	static unsigned udp_ephemeral_port = EPHEMERAL_PORT_START;
	(void)out;
	(void)iface;
	(void)buf;
	udp_ephemeral_port++;
}

static uint16_t udp_cksum(const ip_hdr_t *ip, const udp_hdr_t *udp)
{
	uint32_t udp_csum;
	uint16_t udp_length = ntohs(udp->length);

	/* if (udp->checksum == 0xFFFF) */
	/* 	udp->checksum = 0; */

	udp_csum  = cksum_partial(&ip->src, sizeof(ip->src));
	udp_csum += cksum_partial(&ip->dst, sizeof(ip->dst));
	udp_csum += htons(IPPROTO_UDP);
	udp_csum += cksum_partial(&udp->length, sizeof(udp->length));
	udp_csum += cksum_partial(udp, udp_length);
	udp_csum = (udp_csum & 0xFFFF) + (udp_csum >> 16);
	udp_csum += udp_csum >> 16;

	udp_csum = ~udp_csum;
	return udp_csum;
}

void udp_input(pkt_t *pkt, iface_t *iface)
{
	udp_hdr_t *udp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt, ip_hdr_t *);
	ip_hdr_t *ip_hdr_out;
	uint16_t length;
	sbuf_t key, *val;

	pkt_adj(pkt, ip_hdr->hl * 4);
	udp_hdr = btod(pkt, udp_hdr_t *);
	length = ntohs(udp_hdr->length);
	if (length < sizeof(udp_hdr_t) ||
	    length > pkt_len(pkt) + sizeof(udp_hdr_t))
		goto error;

	sbuf_init(&key, &udp_hdr->dst_port, sizeof(udp_hdr->dst_port));
	if (htable_lookup(udp_binds, &key, &val) < 0) {
		pkt_t *out;
		buf_t data;

		if ((out = pkt_alloc()) == NULL)
			goto error;

		buf_init(&data, ip_hdr, MIN(MAX_ICMP_DATA_SIZE,
					    (uint16_t)ntohs(ip_hdr->len)));
		pkt_adj(out, (int)sizeof(eth_hdr_t));
		ip_hdr_out = btod(out, ip_hdr_t *);
		ip_hdr_out->dst = ip_hdr->src;
		ip_hdr_out->src = ip_hdr->dst;
		ip_hdr_out->p = IPPROTO_ICMP;
		pkt_adj(out, (int)sizeof(ip_hdr_t));

		icmp_output(out, iface, ICMP_UNREACHABLE, ICMP_UNREACH_PORT, 0,
			    0, &data, IP_DF);
		goto error;
	}

	pkt_adj(pkt, sizeof(udp_hdr_t));
	if (udp_hdr->checksum && udp_cksum(ip_hdr, udp_hdr) != 0)
		goto error;

#ifdef DEBUG
	{
		unsigned int i;
		uint8_t *data = (uint8_t *)(udp_hdr + 1);

		puts("udp data:");
		for (i = 0; i < length - sizeof(udp_hdr_t); i++) {
			printf(" 0x%0X", data[i]);
		}
		puts("");
	}
#endif
 error:
	pkt_free(pkt);
	/* inc stats */
	return;
}

int udp_bind(int fd, uint16_t port)
{
	sbuf_t key, val;
	uint16_t be_port = htons(port);

	sbuf_init(&key, &be_port, sizeof(port));
	sbuf_init(&val, &fd, sizeof(fd));

	return htable_add(udp_binds, &key, &val);
}

int udp_init(void)
{
	if ((udp_binds = htable_create(UDP_MAX_BINDS)) == NULL)
		return -1;
	return 0;
}

void udp_shutdown(void)
{
	htable_free(udp_binds);
}
