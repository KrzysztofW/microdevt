#include "tcp.h"

#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "chksum.h"
#include "socket.h"
#include "../sys/hash-tables.h"

/* htable keys (tcp ports) are in network byte order */
hash_table_t *tcp_binds;

void tcp_output(pkt_t *pkt, uint32_t ip_dst, uint8_t ctrl,
		uint16_t sport, uint16_t dport)
{
	tcp_hdr_t *tcp_hdr = btod(pkt, tcp_hdr_t *);
	ip_hdr_t *ip_hdr;

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);
	ip_hdr->dst = ip_dst;
	ip_hdr->p = IPPROTO_TCP;

	tcp_hdr->src_port = sport;
	tcp_hdr->dst_port = dport;
	tcp_hdr->seq = 0;
	tcp_hdr->ack = 0;
	tcp_hdr->reserved = 0;
	tcp_hdr->hdr_len = sizeof(tcp_hdr_t) / 4;
	tcp_hdr->ctrl = ctrl;
	tcp_hdr->urg_ptr = 0;

	ip_output(pkt, NULL, 0, 0);
}

void tcp_input(pkt_t *pkt)
{
	tcp_hdr_t *tcp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt, ip_hdr_t *);
	uint16_t ip_hdr_len = ip_hdr->hl * 4;
	uint16_t payload_len = ip_hdr->len - ip_hdr_len;
	sbuf_t key, *fd;

	pkt_adj(pkt, ip_hdr_len);
	tcp_hdr = btod(pkt, tcp_hdr_t *);
	if (tcp_hdr->hdr_len < 4 || tcp_hdr->hdr_len > 15)
		goto error;

	sbuf_init(&key, &tcp_hdr->dst_port, sizeof(tcp_hdr->dst_port));
	if (htable_lookup(tcp_binds, &key, &fd) < 0) {
		ip_hdr_t *ip_hdr_out;
		pkt_t *out;

		if ((out = pkt_alloc()) == NULL)
			goto error;

		pkt_adj(out, (int)sizeof(eth_hdr_t));
		ip_hdr_out = btod(out, ip_hdr_t *);
		ip_hdr_out->src = ip_hdr->dst;
		pkt_adj(out, (int)sizeof(ip_hdr_t));
		pkt_adj(out, (int)sizeof(tcp_hdr_t));
		pkt_adj(out, -(int)sizeof(tcp_hdr_t));

		tcp_output(out, ip_hdr->src, TH_RST, tcp_hdr->dst_port,
			   tcp_hdr->src_port);
		goto error;
	}
	if (tcp_hdr->checksum &&
	    transport_cksum(ip_hdr, tcp_hdr, sizeof(tcp_hdr_t) + payload_len) != 0) {
		goto error;
	}

	pkt_adj(pkt, sizeof(tcp_hdr_t));
	/* truncate pkt to the tcp payload length */
	pkt->buf.len = payload_len;

	if (socket_append_pkt(fd, pkt) < 0)
		goto error;

	return;

 error:
	pkt_free(pkt);
	/* inc stats */
	return;
}
