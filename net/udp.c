#include "udp.h"
#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "chksum.h"
#include "socket.h"
#include "../sys/hash-tables.h"

/* htable keys (udp ports) are in network byte order */
static hash_table_t *udp_binds;

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

void udp_set_cksum(const void *iph, void *udph)
{
	udp_hdr_t *udp_hdr = udph;
	const ip_hdr_t *ip_hdr = iph;

	udp_hdr->checksum = 0;
	udp_hdr->checksum = udp_cksum(ip_hdr, udp_hdr);
	if (udp_hdr->checksum == 0)
		udp_hdr->checksum = 0xFFFF;
}

void udp_output(pkt_t *pkt, uint32_t ip_dst, uint16_t sport, uint16_t dport)
{
	udp_hdr_t *udp_hdr = btod(pkt, udp_hdr_t *);
	ip_hdr_t *ip_hdr;

	udp_hdr->length = htons(pkt_len(pkt));

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);
	ip_hdr->dst = ip_dst;
	udp_hdr->src_port = sport;
	udp_hdr->dst_port = dport;

	ip_output(pkt, NULL, 0, 0);
}

void udp_input(pkt_t *pkt, iface_t *iface)
{
	udp_hdr_t *udp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt, ip_hdr_t *);
	ip_hdr_t *ip_hdr_out;
	uint16_t length;
	sbuf_t key, *fd;

	pkt_adj(pkt, ip_hdr->hl * 4);
	udp_hdr = btod(pkt, udp_hdr_t *);
	length = ntohs(udp_hdr->length);
	if (length < sizeof(udp_hdr_t) ||
	    length > pkt_len(pkt) + sizeof(udp_hdr_t))
		goto error;

	sbuf_init(&key, &udp_hdr->dst_port, sizeof(udp_hdr->dst_port));
	if (htable_lookup(udp_binds, &key, &fd) < 0) {
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

	if (udp_hdr->checksum && udp_cksum(ip_hdr, udp_hdr) != 0)
		goto error;

	pkt_adj(pkt, sizeof(udp_hdr_t));
	/* truncate pkt to the udp payload length */
	pkt->buf.len = length - sizeof(udp_hdr_t);

	if (socket_append_pkt(fd, pkt) < 0)
		goto error;

	return;

 error:
	pkt_free(pkt);
	/* inc stats */
	return;
}

int max_retries = EPHEMERAL_PORT_END - EPHEMERAL_PORT_START;

/* port - network endian format */
int udp_bind(uint8_t fd, uint16_t *port)
{
	sbuf_t key, val;
	static unsigned udp_ephemeral_port = EPHEMERAL_PORT_START;
	int ret = -1, retries = 0;
	uint16_t p = *port;
	int client = p ? 0 : 1;

	do {
		if (client) {
			p = htons(udp_ephemeral_port);
			udp_ephemeral_port++;
			if (udp_ephemeral_port >= EPHEMERAL_PORT_END)
				udp_ephemeral_port = EPHEMERAL_PORT_START;
		}

		sbuf_init(&key, &p, sizeof(p));
		sbuf_init(&val, &fd, sizeof(fd));

		if ((ret = htable_add(udp_binds, &key, &val)) < 0)
			retries++;
		else {
			*port = p;
			break;
		}
	} while (client && retries < max_retries);

	return ret;
}

/* port - network endian format */
int udp_unbind(uint16_t port)
{
	sbuf_t key;

	sbuf_init(&key, &port, sizeof(port));
	return htable_del(udp_binds, &key);
}

int udp_init(void)
{
	if ((udp_binds = htable_create(UDP_MAX_HT)) == NULL)
		return -1;
	return 0;
}

void udp_shutdown(void)
{
	htable_free(udp_binds);
}
