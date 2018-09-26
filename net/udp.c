#include "udp.h"
#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "tr-chksum.h"
#include "socket.h"
#include "../sys/hash-tables.h"

int udp_output(pkt_t *pkt, uint32_t ip_dst, uint16_t sport, uint16_t dport)
{
	udp_hdr_t *udp_hdr = btod(pkt);
	ip_hdr_t *ip_hdr;

	udp_hdr->length = htons(pkt_len(pkt));

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt);
	ip_hdr->dst = ip_dst;
	ip_hdr->p = IPPROTO_UDP;
	udp_hdr->src_port = sport;
	udp_hdr->dst_port = dport;

	return ip_output(pkt, NULL, 0);
}

void udp_input(pkt_t *pkt, const iface_t *iface)
{
	udp_hdr_t *udp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt);
	uint16_t length;
	sock_info_t *sock_info;
	uint16_t ip_hdr_len = ip_hdr->hl * 4;

	(void)iface;
	pkt_adj(pkt, ip_hdr_len);
	udp_hdr = btod(pkt);
	length = ntohs(udp_hdr->length);
	if (length < sizeof(udp_hdr_t) ||
	    length > pkt_len(pkt) + sizeof(udp_hdr_t))
		goto error;

	if ((sock_info = udpport2sockinfo(udp_hdr->dst_port)) == NULL) {
#ifdef CONFIG_ICMP
		ip_hdr_t *ip_hdr_out;
		pkt_t *out;
		buf_t data;

		if ((out = pkt_alloc()) == NULL)
			goto error;

		buf_init(&data, ip_hdr, MIN(MAX_ICMP_DATA_SIZE,
					    (uint16_t)ntohs(ip_hdr->len)));
		pkt_adj(out, (int)sizeof(eth_hdr_t));
		ip_hdr_out = btod(out);
		ip_hdr_out->dst = ip_hdr->src;
		ip_hdr_out->src = ip_hdr->dst;
		ip_hdr_out->p = IPPROTO_ICMP;
		pkt_adj(out, (int)sizeof(ip_hdr_t));

		icmp_output(out, iface, ICMP_UNREACHABLE, ICMP_UNREACH_PORT, 0,
			    0, &data, IP_DF);
#endif
		goto error;
	}

	if (udp_hdr->checksum && transport_cksum(ip_hdr, udp_hdr,
						 udp_hdr->length) != 0)
		goto error;

	pkt_adj(pkt, sizeof(udp_hdr_t));
	/* truncate pkt to the udp payload length */
	pkt->buf.len = length - sizeof(udp_hdr_t);

	pkt_adj(pkt, -(sizeof(udp_hdr_t) + ip_hdr_len));
	socket_append_pkt(&sock_info->trq.pkt_list, pkt);
#ifdef CONFIG_EVENT
	event_schedule_event(&sock_info->event, EV_READ | EV_WRITE);
#endif
	return;

 error:
	pkt_free(pkt);
	/* inc stats */
}
