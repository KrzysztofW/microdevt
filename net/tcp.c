#include "tcp.h"
#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "chksum.h"
#include "socket.h"
#include "../sys/hash-tables.h"

#ifdef CONFIG_HT_STORAGE
/* htable keys (tcp uids), values are pkt list (struct list_head) */
hash_table_t *tcp_conns;
#else
struct list_head tcp_conns = LIST_HEAD_INIT(tcp_conns);
#endif
uint8_t tcp_conn_cnt;

uint32_t tcp_initial_seqid = 0x1207e77b;

struct tcp_syn {
	uint32_t seqid;
	uint32_t ack;
	tcp_options_t opts;
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

#ifdef CONFIG_TCP_CLIENT
struct list_head tcp_client_conns = LIST_HEAD_INIT(tcp_client_conns);
#endif

static tcp_syn_t *syn_find_entry(const tcp_uid_t *uid)
{
	int i;

	for (i = 0; i < CONFIG_TCP_SYN_TABLE_SIZE; i++) {
		if (memcmp(&syn_entries.conns[i].tuid, uid, sizeof(tcp_uid_t)) == 0)
			return &syn_entries.conns[i];
	}
	return NULL;
}

static inline void __tcp_conn_delete(tcp_conn_t *tcp_conn)
{
	sock_info_t *sock_info = tcp_conn->sock_info;
	pkt_t *pkt, *pkt_tmp;

	if (sock_info)
		sock_info->trq.tcp_conn = NULL;
	list_for_each_entry_safe(pkt, pkt_tmp, &tcp_conn->pkt_list_head, list) {
		list_del(&pkt->list);
		pkt_free(pkt);
	}

	tcp_conn_cnt--;
	assert(tcp_conn_cnt <= CONFIG_TCP_MAX_CONNS);

	list_del(&tcp_conn->list);
	free(tcp_conn);
}

tcp_conn_t *tcp_conn_create(const tcp_uid_t *tuid, uint8_t status)
{
	tcp_conn_t *conn;

	if (tcp_conn_cnt >= CONFIG_TCP_MAX_CONNS)
		return NULL;

	conn = malloc(sizeof(tcp_conn_t));
	if (conn == NULL)
		return NULL;

	tcp_conn_cnt++;
	INIT_LIST_HEAD(&conn->pkt_list_head);
	INIT_LIST_HEAD(&conn->list);
	conn->status = status;
	conn->uid = *tuid;
	conn->sock_info = NULL;

	return conn;
}

#ifdef CONFIG_HT_STORAGE
static int tcp_conn_add(tcp_conn_t *tcp_conn)
{
	sbuf_t key, val;

	sbuf_init(&key, &conn->uid, sizeof(tcp_uid_t));
	sbuf_init(&val, &conn, sizeof(tcp_conn_t *));
	if (htable_add(tcp_conns, &key, &val) < 0)
		return -1;
	return 0;
}

void tcp_conn_delete(tcp_conn_t *tcp_conn)
{
	sbuf_t key;

	sbuf_init(&key, &tcp_conn->uid, sizeof(tcp_uid_t));
	htable_del(tcp_conns, &key);
	__tcp_conn_delete(tcp_conn);
}

tcp_conn_t *tcp_conn_lookup(const tcp_uid_t *uid)
{
	sbuf_t key, *val;

	sbuf_init(&key, uid, sizeof(tcp_uid_t));
	if (htable_lookup(tcp_conns, &key, &val) < 0)
		return NULL;
	return (tcp_conn_t *)val->data;
}
#else /* CONFIG_HT_STORAGE */

#ifdef CONFIG_TCP_CLIENT
static inline int tcp_conn_add(tcp_conn_t *tcp_conn)
{
	list_add_tail(&tcp_conn->list, &tcp_conns);
	return 0;
}
#endif

void tcp_conn_delete(tcp_conn_t *tcp_conn)
{
	__tcp_conn_delete(tcp_conn);
}

tcp_conn_t *tcp_conn_lookup(const tcp_uid_t *uid)
{
	tcp_conn_t *tcp_conn;
	list_for_each_entry(tcp_conn, &tcp_conns, list) {
		/* tcp_conn must be packed */
		if (memcmp(uid, &tcp_conn->uid, sizeof(tcp_uid_t)) == 0)
			return tcp_conn;
	}
	return socket_tcp_conn_lookup(uid);
}

#endif

#ifdef CONFIG_TCP_CLIENT
tcp_conn_t *tcp_client_conn_lookup(const tcp_uid_t *uid)
{
	tcp_conn_t *tcp_conn;
	list_for_each_entry(tcp_conn, &tcp_client_conns, list) {
		/* tcp_conn must be packed */
		if (memcmp(uid, &tcp_conn->uid, sizeof(tcp_uid_t)) == 0)
			return tcp_conn;
	}
	return NULL;
}
#endif

static int tcp_set_options(void *data)
{
	uint8_t *opts = data;
	uint16_t opts_len;

	opts[0] = TCPOPT_MAXSEG;
	opts[1] = TCPOLEN_MAXSEG;
	opts += 2;
	opts_len = CONFIG_PKT_SIZE - sizeof(eth_hdr_t)
		- sizeof(ip_hdr_t) - sizeof(tcp_hdr_t) - TCPOLEN_MAXSEG;
	*(uint16_t *)opts = htons(opts_len);
	return TCPOLEN_MAXSEG;
}

static void __tcp_adj_out_pkt(pkt_t *out)
{
	pkt_adj(out, (int)sizeof(eth_hdr_t) + (int)sizeof(ip_hdr_t)
		+ (int)sizeof(tcp_hdr_t));
	pkt_adj(out, -(int)sizeof(tcp_hdr_t));
}

static void
__tcp_output(pkt_t *pkt, uint32_t ip_dst, uint8_t ctrl,
	     uint16_t sport, uint16_t dport, uint32_t seqid, uint32_t ack)
{
	tcp_hdr_t *tcp_hdr = btod(pkt, tcp_hdr_t *);
	ip_hdr_t *ip_hdr;
	uint8_t tcp_hdr_len = sizeof(tcp_hdr_t);

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);
	ip_hdr->dst = ip_dst;
	ip_hdr->p = IPPROTO_TCP;

	tcp_hdr->src_port = sport;
	tcp_hdr->dst_port = dport;
	tcp_hdr->seq = seqid;
	tcp_hdr->ack = ack;
	tcp_hdr->reserved = 0;
	tcp_hdr->ctrl = ctrl;
	tcp_hdr->win_size = (ctrl & TH_RST) ? 0 : 1; /* XXX to be checked */
	tcp_hdr->urg_ptr = 0;
	if (ctrl & TH_SYN) {
		int opts_len = tcp_set_options(tcp_hdr + 1);

		tcp_hdr_len += opts_len;
		pkt->buf.len += opts_len;
	}
	tcp_hdr->hdr_len = tcp_hdr_len / 4;

	ip_output(pkt, NULL, 0, IP_DF);
}

void tcp_output(pkt_t *pkt, tcp_conn_t *tcp_conn, uint8_t flags)
{
	__tcp_output(pkt, tcp_conn->uid.src_addr, flags,
		     tcp_conn->uid.dst_port, tcp_conn->uid.src_port,
		     tcp_conn->seqid, tcp_conn->ack);
}

static void
tcp_send_pkt(const ip_hdr_t *ip_hdr, const tcp_hdr_t *tcp_hdr, uint8_t flags,
	     uint32_t seqid, uint32_t ack)
{
	pkt_t *out;

	if ((out = pkt_alloc()) == NULL &&
	    (out = pkt_alloc_emergency()) == NULL) {
		return;
	}

	__tcp_adj_out_pkt(out);
	__tcp_output(out, ip_hdr->src, flags, tcp_hdr->dst_port,
		     tcp_hdr->src_port, seqid, ack);
}


	tcp_output(out, ip_hdr->src, flags, tcp_hdr->dst_port,
		   tcp_hdr->src_port, seqid, ack);
}

static void
set_tuid(tcp_uid_t *tuid, const ip_hdr_t *ip_hdr, const tcp_hdr_t *tcp_hdr)
{
	tuid->src_addr = ip_hdr->src;
	tuid->dst_addr = ip_hdr->dst;
	tuid->src_port = tcp_hdr->src_port;
	tuid->dst_port = tcp_hdr->dst_port;
}

void tcp_parse_options(tcp_options_t *tcp_opts, tcp_hdr_t *tcp_hdr, int opt_len)
{
	uint8_t *ops = (uint8_t *)(tcp_hdr + 1);
	uint8_t *ops_end = ops + opt_len;
	uint8_t op;

	while (ops < ops_end && (op = ops[0]) != 0) {
		int len;

		switch (op) {
		case TCPOPT_NOP:
			len = TCPOLEN_NOP;
			break;
		case TCPOPT_MAXSEG:
			tcp_opts->mss = ntohs(*(uint16_t *)(ops + 2));
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

#ifdef CONFIG_TCP_CLIENT
int tcp_connect(uint32_t dst_addr, uint16_t dst_port, void *si)
{
	tcp_conn_t *tcp_conn;
	tcp_uid_t tuid;
	pkt_t *pkt;
	sock_info_t *sock_info = si;

	tuid.src_addr = dst_addr;
	tuid.src_port = dst_port;
	tuid.dst_addr = 0; /* we don't know yet what src ip addr will be used */
	tuid.dst_port = sock_info->port;

	if (tcp_client_conn_lookup(&tuid) != NULL) /* connect in progress */
		return -1;

	if ((tcp_conn = tcp_conn_create(&tuid, SOCK_TCP_SYN_SENT)) == NULL)
		return -1;

	if ((pkt = pkt_alloc()) == NULL) {
		free(tcp_conn);
		return -1;
	}

	list_add(&tcp_conn->list, &tcp_client_conns);
	tcp_conn->seqid = tcp_initial_seqid;
	tcp_conn->ack = 0;
	tcp_conn->sock_info = sock_info;
	sock_info->trq.tcp_conn = tcp_conn;

	__tcp_adj_out_pkt(pkt);
	tcp_output(pkt, tcp_conn, TH_SYN);
	return 0;
}
#endif

void tcp_input(pkt_t *pkt)
{
	tcp_hdr_t *tcp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt, ip_hdr_t *);
	uint16_t ip_hdr_len = ip_hdr->hl * 4;
	uint16_t ip_plen = ntohs(ip_hdr->len) - ip_hdr_len;
	uint16_t tcp_hdr_len;
	sock_info_t *sock_info;
	tcp_uid_t tuid;
	tcp_syn_t *tsyn_entry;
	uint32_t remote_seqid, remote_ack;
	tcp_conn_t *tcp_conn;

	STATIC_ASSERT(POWEROF2(CONFIG_TCP_SYN_TABLE_SIZE));

	pkt_adj(pkt, ip_hdr_len);
	tcp_hdr = btod(pkt, tcp_hdr_t *);
	if (tcp_hdr->hdr_len < 4 || tcp_hdr->hdr_len > 15)
		goto end;

	tcp_hdr_len = tcp_hdr->hdr_len * 4;
	if (transport_cksum(ip_hdr, tcp_hdr, htons(ip_plen)) != 0)
		goto end;

	set_tuid(&tuid, ip_hdr, tcp_hdr);
	remote_seqid = ntohl(tcp_hdr->seq);
	remote_ack = ntohl(tcp_hdr->ack);

	tcp_conn = tcp_conn_lookup(&tuid);

	if ((tcp_conn = tcp_conn_lookup(&tuid)) != NULL) {
		int flags = 0, plen;
		uint32_t ack, seqid;
		uint8_t remote_fin_set = 0;

		if (tcp_hdr->ctrl & TH_RST || tcp_conn->status == SOCK_CLOSED) {
			tcp_conn->status = SOCK_CLOSED;
			goto end;
		}

		ack = ntohl(tcp_conn->ack);
		seqid = ntohl(tcp_conn->seqid);
		if (remote_seqid < ack) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK, tcp_conn->seqid,
				     tcp_conn->ack);
			goto end;
		}
		if (remote_seqid > ack) {
			goto end;
		}
		if ((tcp_hdr->ctrl & TH_ACK)) {
			if (remote_ack > seqid) {
				/* drop the packet */
				goto end;
			}
		}

		if (tcp_hdr->ctrl & TH_FIN) {
			if (tcp_conn->status == SOCK_CONNECTED) {
				flags |= TH_FIN|TH_ACK;
				remote_fin_set = 1;
				tcp_conn->status = SOCK_TCP_FIN_SENT;
			}
		} else if (tcp_hdr->ctrl == TH_ACK
			   && tcp_conn->status == SOCK_TCP_FIN_SENT
			   && tcp_hdr->ack == tcp_conn->seqid) {
			tcp_conn->status = SOCK_CLOSED;
			goto end;
		}
		pkt_adj(pkt, tcp_hdr_len);
		/* truncate pkt to the tcp payload length */
		plen = ip_plen - tcp_hdr_len;
		if (pkt->buf.len > plen)
			pkt->buf.len = plen;

		ack = remote_seqid + pkt->buf.len + remote_fin_set;
		tcp_conn->ack = htonl(ack);
		if (remote_fin_set == 0 && tcp_conn->status == SOCK_TCP_FIN_SENT) {
			/* remote closed => drop pkt */
			goto end;
		}
		if (remote_seqid < ack || remote_ack < seqid)
			tcp_send_pkt(ip_hdr, tcp_hdr, flags | TH_ACK, tcp_conn->seqid,
				     tcp_conn->ack);

		if (flags & TH_FIN) {
			seqid++;
			tcp_conn->seqid = htonl(seqid);
		}

		if (pkt->buf.len == 0)
			goto end;

		socket_append_pkt(&tcp_conn->pkt_list_head, pkt);
		return;
	}
	if (tcp_hdr->ctrl & TH_RST)
		goto end;

#ifdef CONFIG_TCP_CLIENT
	if (tcp_hdr->ctrl == (TH_SYN|TH_ACK)) {
		tcp_conn_t *tcp_conn;
		uint32_t dst_addr = tuid.dst_addr;
		uint32_t seqid;

		tuid.dst_addr = 0;
		if ((tcp_conn = tcp_client_conn_lookup(&tuid)) == NULL)
			goto end;
		tcp_conn->uid.dst_addr = dst_addr;

		if (tcp_conn->status != SOCK_TCP_SYN_SENT
		    || htonl(remote_ack - 1) != tcp_conn->seqid) {
			tcp_conn->status = SOCK_CLOSED;
			goto end;
		}

		tcp_conn->ack = htonl(remote_seqid + 1);
		tcp_conn->status = SOCK_CONNECTED;
		tcp_parse_options(&tcp_conn->opts, tcp_hdr,
				  tcp_hdr_len - sizeof(tcp_hdr_t));

		seqid = ntohl(tcp_conn->seqid) + 1;
		tcp_conn->seqid = htonl(seqid);
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK, tcp_conn->seqid,
			     tcp_conn->ack);
		list_del(&tcp_conn->list);
		if (tcp_conn_add(tcp_conn) < 0)
			__tcp_conn_delete(tcp_conn);
		goto end;
	}
#endif

	if ((sock_info = tcpport2sockinfo(tcp_hdr->dst_port)) == NULL) {
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST|TH_ACK, 0,
			     htonl(remote_seqid + 1));
		goto end;
	}

	tsyn_entry = syn_find_entry(&tuid);
	if (tcp_hdr->ctrl == TH_SYN) {
		tcp_syn_t *tcp_syn = &syn_entries.conns[syn_entries.pos];

		/* network endian for seqid and ack */
		tcp_syn->seqid = tcp_initial_seqid;
		tcp_syn->ack = htonl(remote_seqid + 1);
		tcp_syn->status = SOCK_TCP_SYN_ACK_SENT;
		tcp_parse_options(&tcp_syn->opts, tcp_hdr, tcp_hdr_len - sizeof(tcp_hdr_t));
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_SYN|TH_ACK, tcp_syn->seqid,
			     tcp_syn->ack);
		syn_entries.pos = (syn_entries.pos + 1)
			& (CONFIG_TCP_SYN_TABLE_SIZE - 1);
		set_tuid(&tcp_syn->tuid, ip_hdr, tcp_hdr);
		goto end;
	}

	if (tcp_hdr->ctrl & TH_FIN) {
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, tcp_hdr->ack,
			     htonl(remote_seqid + 1));
		goto end;
	}
	if (tcp_hdr->ctrl & TH_ACK) {
		uint32_t local_seqid;

		if (tsyn_entry == NULL) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, tcp_hdr->ack,
				     tcp_hdr->seq);
			goto end;
		}
		local_seqid = htonl(ntohl(tsyn_entry->seqid) + 1);
		if (tsyn_entry->status == SOCK_TCP_SYN_ACK_SENT) {
			listen_t *l = sock_info->listen;

			if (tcp_hdr->seq != tsyn_entry->ack ||
			    tcp_hdr->ack != local_seqid) {
				tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST|TH_ACK,
					     tcp_hdr->ack, htonl(remote_seqid));
				goto end;
			}

			if (l == NULL || l->backlog >= l->backlog_max ||
			    (tcp_conn = tcp_conn_create(&tuid,
						  SOCK_CONNECTED)) == NULL) {
				tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST,
					     local_seqid, tcp_hdr->seq);
			} else {
				if (socket_add_backlog(l, tcp_conn) < 0) {
					tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST,
						     local_seqid, tcp_hdr->seq);
					tcp_conn_delete(tcp_conn);
					goto end;
				}
				tcp_conn->seqid = local_seqid;
				tcp_conn->ack = tcp_hdr->seq;
				tcp_conn->opts = tsyn_entry->opts;
				goto end;
			}
		}
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, local_seqid, tcp_hdr->seq);
	}
	/* silently drop the packet */
 end:
	pkt_free(pkt);
}
