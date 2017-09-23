#include "tcp.h"
#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "chksum.h"
#include "socket.h"
#include "../sys/hash-tables.h"

/* htable keys (tcp ports) are in network byte order */
hash_table_t *tcp_binds;

/* htable keys (tcp uids), values are pkt list (struct list_head) */
hash_table_t *tcp_conns;

struct tcp_syn {
	uint32_t seqid;
	uint32_t ack;
	uint16_t mss;
	tcp_uid_t tuid;
	uint8_t status;
} __attribute__((__packed__));
typedef struct tcp_syn tcp_syn_t;

struct syn_entries {
	tcp_syn_t conns[CONFIG_TCP_SYN_TABLE_SIZE];
	uint8_t pos;
} __attribute__((__packed__));
typedef struct syn_entries syn_entries_t;

syn_entries_t syn_entries;

static tcp_syn_t *syn_find_entry(const tcp_uid_t *uid)
{
	int i;

	for (i = 0; i < CONFIG_TCP_SYN_TABLE_SIZE; i++) {
		if (memcmp(&syn_entries.conns[i].tuid, uid, sizeof(tcp_uid_t)) == 0)
			return &syn_entries.conns[i];
	}
	return NULL;
}

static tcp_conn_t *tcp_conn_create(const sbuf_t *key, uint8_t status)
{
	tcp_conn_t conn, *c;
	tcp_uid_t *tuid = (tcp_uid_t *)key->data;
	sbuf_t val;

	sbuf_init(&val, &conn, sizeof(conn));
	if (htable_add(tcp_conns, key, &val) < 0)
		return NULL;
	c = (tcp_conn_t *)val.data;
	INIT_LIST_HEAD(&c->pkt_list_head);
	INIT_LIST_HEAD(&c->list);
	c->status = status;
	c->uid = *tuid;
	c->sock_info = NULL;

	return c;
}

void tcp_conn_delete(tcp_conn_t *tcp_conn)
{
	sbuf_t key;

	sbuf_init(&key, tcp_conn, sizeof(tcp_conn_t));
	htable_del(tcp_conns, &key);
}

void tcp_output(pkt_t *pkt, uint32_t ip_dst, uint8_t ctrl,
		uint16_t sport, uint16_t dport, uint32_t seqid, uint32_t ack,
		uint16_t ip_flags)
{
	tcp_hdr_t *tcp_hdr = btod(pkt, tcp_hdr_t *);
	ip_hdr_t *ip_hdr;

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);
	ip_hdr->dst = ip_dst;
	ip_hdr->p = IPPROTO_TCP;

	tcp_hdr->src_port = sport;
	tcp_hdr->dst_port = dport;
	tcp_hdr->seq = seqid;
	tcp_hdr->ack = ack;
	tcp_hdr->reserved = 0;
	tcp_hdr->hdr_len = sizeof(tcp_hdr_t) / 4;
	tcp_hdr->ctrl = ctrl;
	tcp_hdr->win_size = (ctrl & TH_RST) ? 0 : 1; /* XXX to be checked */
	tcp_hdr->urg_ptr = 0;

	ip_output(pkt, NULL, 0, ip_flags);
}

static void
tcp_send_pkt(const ip_hdr_t *ip_hdr, const tcp_hdr_t *tcp_hdr, uint8_t flags,
	     uint32_t seqid, uint32_t ack)
{
	ip_hdr_t *ip_hdr_out;
	pkt_t *out;
	uint16_t ip_flags;

	if ((out = pkt_alloc()) == NULL)
		return;
	pkt_adj(out, (int)sizeof(eth_hdr_t));
	ip_hdr_out = btod(out, ip_hdr_t *);
	ip_hdr_out->src = ip_hdr->dst;
	pkt_adj(out, (int)sizeof(ip_hdr_t));
	pkt_adj(out, (int)sizeof(tcp_hdr_t));
	pkt_adj(out, -(int)sizeof(tcp_hdr_t));
	if (flags & (TH_FIN | TH_SYN | TH_RST | TH_ACK | TH_URG))
		ip_flags = IP_DF;
	else
		ip_flags = 0;

	tcp_output(out, ip_hdr->src, flags, tcp_hdr->dst_port,
		   tcp_hdr->src_port, seqid, ack, ip_flags);
}

static void
set_tuid(tcp_uid_t *tuid, const ip_hdr_t *ip_hdr, const tcp_hdr_t *tcp_hdr)
{
	tuid->src_addr = ip_hdr->src;
	tuid->dst_addr = ip_hdr->dst;
	tuid->src_port = tcp_hdr->src_port;
	tuid->dst_port = tcp_hdr->dst_port;
}

void tcp_parse_options(tcp_syn_t *tcp_syn, tcp_hdr_t *tcp_hdr, int opt_len)
{
	uint8_t *ops = (uint8_t *)(tcp_hdr + 1);
	uint8_t *ops_end = ops + opt_len;
	uint8_t op;

	while (ops <= ops_end && (op = ops[0]) != 0) {
		int len;

		switch (op) {
		case TCPOPT_NOP:
			len = TCPOLEN_NOP;
			break;
		case TCPOPT_MAXSEG:
			tcp_syn->mss = ntohs(*(uint16_t *)(ops + 2));
			len = TCPOLEN_MAXSEG;
			break;
		case TCPOPT_WINDOW:
			len = TCPOLEN_WINDOW;
			break;
		case TCPOPT_SACK:
			ops += TCPOLEN_SACK;
		case TCPOPT_SACK_PERMITTED:
			len = TCPOLEN_SACK_PERMITTED;
			break;
		case TCPOPT_TIMESTAMP:
			len = TCPOLEN_TIMESTAMP;
			break;
		case TCPOPT_SIGNATURE:
			len = TCPOLEN_SIGNATURE;
			break;
		default:
			len = ops[1];
		}
		ops += len;
	}
}

static void tcp_socket_close(const sbuf_t *key_tuid, tcp_conn_t *tcp_conn)
{
	sock_info_t *sock_info = tcp_conn->sock_info;

	if (sock_info != NULL)
		sock_info->trq.tcp_conn = NULL;
	htable_del(tcp_conns, key_tuid);
}

static void tcp_conn_inc_ack(tcp_conn_t *tcp_conn, int cnt)
{
	tcp_conn->ack = htonl(ntohl(tcp_conn->ack) + cnt);
}

void tcp_input(pkt_t *pkt)
{
	tcp_hdr_t *tcp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt, ip_hdr_t *);
	uint16_t ip_hdr_len = ip_hdr->hl * 4;
	uint16_t ip_plen = ntohs(ip_hdr->len) - ip_hdr_len;
	uint16_t tcp_hdr_len;
	sbuf_t key_tuid, key_port, *val;
	sock_info_t *sock_info;
	tcp_uid_t tuid;
	tcp_syn_t *tsyn_entry;
	uint32_t remote_seqid;

	STATIC_ASSERT(POWEROF2(CONFIG_TCP_SYN_TABLE_SIZE));

	pkt_adj(pkt, ip_hdr_len);
	tcp_hdr = btod(pkt, tcp_hdr_t *);
	if (tcp_hdr->hdr_len < 4 || tcp_hdr->hdr_len > 15)
		goto end;

	tcp_hdr_len = tcp_hdr->hdr_len * 4;
	if (tcp_hdr->checksum &&
	    transport_cksum(ip_hdr, tcp_hdr, htons(ip_plen)) != 0) {
		goto end;
	}

	set_tuid(&tuid, ip_hdr, tcp_hdr);
	sbuf_init(&key_tuid, &tuid, sizeof(tuid));
	remote_seqid = ntohl(tcp_hdr->seq);
	if (htable_lookup(tcp_conns, &key_tuid, &val) >= 0) {
		tcp_conn_t *tcp_conn;

		tcp_conn = (tcp_conn_t *)val->data;
		if (tcp_hdr->ctrl & TH_RST) {
			tcp_socket_close(&key_tuid, tcp_conn);
			goto end;
		}
		if (tcp_hdr->seq != tcp_conn->ack || tcp_hdr->ack != tcp_conn->seqid) {
			/* drop the packet */
			/* tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, tcp_conn->seqid, 0); */
			/* tcp_socket_close(&key_tuid, tcp_conn); */
			goto end;
		}
		if (tcp_hdr->ctrl & TH_FIN) {
			tcp_conn_inc_ack(tcp_conn, 1);
			if (tcp_conn->status == SOCK_CONNECTED) {
				tcp_send_pkt(ip_hdr, tcp_hdr, TH_FIN|TH_ACK,
					     tcp_conn->seqid, tcp_conn->ack);
				tcp_conn->status = SOCK_TCP_FIN_SENT;
			} else if (tcp_conn->status == SOCK_TCP_FIN_SENT) {
				tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK,
					     tcp_conn->seqid, tcp_conn->ack);
				tcp_socket_close(&key_tuid, tcp_conn);
			}
			goto end;
		}
		if (tcp_hdr->ctrl == TH_ACK && tcp_conn->status == SOCK_TCP_FIN_SENT) {
			tcp_socket_close(&key_tuid, tcp_conn);
			goto end;
		}

		pkt_adj(pkt, tcp_hdr_len);
		/* truncate pkt to the tcp payload length */
		pkt->buf.len = ip_plen - tcp_hdr_len;

		if (pkt->buf.len)
			socket_append_pkt(&tcp_conn->pkt_list_head, pkt);
		tcp_conn->ack = htonl(remote_seqid + pkt->buf.len);
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK, tcp_conn->seqid, tcp_conn->ack);
		return;
	}

	sbuf_init(&key_port, &tcp_hdr->dst_port, sizeof(tcp_hdr->dst_port));
	if (htable_lookup(tcp_binds, &key_port, &val) < 0) {
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST|TH_ACK, 0, htonl(remote_seqid + 1));
		goto end;
	}
	sock_info = SBUF2SOCKINFO(val);
	tsyn_entry = syn_find_entry(&tuid);

	if (tcp_hdr->ctrl == TH_SYN) {
		tcp_syn_t *tcp_syn = &syn_entries.conns[syn_entries.pos];

		/* network endian for seqid and ack */
		tcp_syn->seqid = 0x1207e77b; /* XXX to be changed */
		tcp_syn->ack = htonl(remote_seqid + 1);
		tcp_syn->status = SOCK_TCP_SYN_ACK_SENT;
		tcp_parse_options(tcp_syn, tcp_hdr, tcp_hdr_len - sizeof(tcp_hdr_t));
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_SYN|TH_ACK, tcp_syn->seqid,
			     tcp_syn->ack);
		if (&tsyn_entry->tuid != &tcp_syn->tuid) {
			syn_entries.pos = (syn_entries.pos + 1)
				& (CONFIG_TCP_SYN_TABLE_SIZE - 1);
			set_tuid(&tcp_syn->tuid, ip_hdr, tcp_hdr);
		}
		goto end;
	}
	if (tcp_hdr->ctrl == (TH_SYN|TH_ACK)) {
		tcp_conn_t *tc;

		if (tsyn_entry == NULL || tsyn_entry->status != SOCK_TCP_SYN_SENT)
			goto end;
		tsyn_entry->seqid = htonl(ntohl(tsyn_entry->seqid) + 1);
		if ((tc = tcp_conn_create(&key_tuid, SOCK_CONNECTED)) == NULL)
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, tsyn_entry->seqid, 0);
		else {
			sock_info->trq.tcp_conn = tc;
			if (tsyn_entry->ack != htonl(remote_seqid))
				tsyn_entry->ack = htonl(remote_seqid + 1);
			/* else => duplicate */

			tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK, tsyn_entry->seqid, tsyn_entry->ack);
		}
		goto end;
	}
	if (tcp_hdr->ctrl == TH_ACK) {
		if (tsyn_entry == NULL) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, 0, 0);
			goto end;
		}
		if (tsyn_entry->status == SOCK_TCP_SYN_ACK_SENT) {
			listen_t *l = sock_info->listen;
			tcp_conn_t *tc;
			uint32_t local_seqid = htonl(ntohl(tsyn_entry->seqid) + 1);

			if (tcp_hdr->seq != tsyn_entry->ack ||
			    tcp_hdr->ack != local_seqid) {
				tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, local_seqid, 0);
				goto end;
			}

			if (l == NULL || l->backlog >= l->backlog_max ||
			    (tc = tcp_conn_create(&key_tuid, SOCK_CONNECTED)) == NULL)
				tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, local_seqid, 0);
			else {
				if (sock_info->listen->backlog >= sock_info->listen->backlog_max){
					tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, local_seqid, 0);
					goto end;
				}
				list_add_tail(&tc->list, &sock_info->listen->tcp_conn_list_head);
				sock_info->listen->backlog++;
				tc->seqid = local_seqid;
				tc->ack = htonl(remote_seqid);
				tc->mss = tsyn_entry->mss;
			}
		}
		goto end;
	}
	/* silently drop the packet */
 end:
	pkt_free(pkt);
}
