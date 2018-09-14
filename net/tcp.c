#include "tcp.h"
#include "ip.h"
#include "icmp.h"
#include "eth.h"
#include "tr-chksum.h"
#include "socket.h"
#include "../sys/hash-tables.h"

#ifdef CONFIG_HT_STORAGE
/* htable keys (tcp uids), values are pkt list (list_t) */
static hash_table_t *tcp_conns;
#else
static list_t tcp_conns = LIST_HEAD_INIT(tcp_conns);
#endif
static uint8_t tcp_conn_cnt;

struct syn_entries {
	tcp_syn_t conns[CONFIG_TCP_SYN_TABLE_SIZE];
	uint8_t pos;
} __attribute__((__packed__));
typedef struct syn_entries syn_entries_t;

syn_entries_t syn_entries;

#ifdef CONFIG_TCP_CLIENT
list_t tcp_client_conns = LIST_HEAD_INIT(tcp_client_conns);
#endif

#ifdef CONFIG_TCP_RETRANSMIT
#define TCP_IN_PROGRESS_RETRIES 3
#endif

static tcp_syn_t *syn_find_entry(const tcp_uid_t *uid)
{
	int i;

	for (i = 0; i < CONFIG_TCP_SYN_TABLE_SIZE; i++) {
		if (memcmp(&syn_entries.conns[i].tuid, uid,
			   sizeof(tcp_uid_t)) == 0)
			return &syn_entries.conns[i];
	}
	return NULL;
}

#ifdef CONFIG_TCP_RETRANSMIT
static inline void tcp_retransmit_init(tcp_retrn_t *retrn)
{
	memset(&retrn->timer, 0, sizeof(tim_t));
	INIT_LIST_HEAD(&retrn->retrn_pkt_list);
}

static void tcp_retrn_wipe(tcp_conn_t *tcp_conn)
{
	tcp_retrn_pkt_t *retrn_pkt, *retrn_pkt_tmp;

	list_for_each_entry_safe(retrn_pkt, retrn_pkt_tmp,
				 &tcp_conn->retrn.retrn_pkt_list, list) {
		list_del(&retrn_pkt->list);
		pkt_free(retrn_pkt->pkt);
		free(retrn_pkt);
	}
	timer_del(&tcp_conn->retrn.timer);
}
#endif

static void __tcp_adj_out_pkt(pkt_t *out)
{
	pkt_adj(out, (int)sizeof(eth_hdr_t) + (int)sizeof(ip_hdr_t)
		+ (int)sizeof(tcp_hdr_t));
	pkt_adj(out, -(int)sizeof(tcp_hdr_t));
}

static void tcp_close(tcp_conn_t *tcp_conn, pkt_t *fin_pkt)
{
	tcp_conn->syn.status = SOCK_TCP_FIN_SENT;
	__tcp_adj_out_pkt(fin_pkt);
	tcp_output(fin_pkt, tcp_conn, TH_FIN|TH_ACK);
	tcp_conn->syn.seqid = htonl(ntohl(tcp_conn->syn.seqid) + 1);
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

	if (tcp_conn->syn.status == SOCK_CONNECTED) {
		pkt_t *fin_pkt = pkt_alloc();

		if (fin_pkt != NULL) {
			tcp_close(tcp_conn, fin_pkt);
			return;
		}
	}
#ifdef CONFIG_TCP_RETRANSMIT
	tcp_retrn_wipe(tcp_conn);
#endif
	assert(tcp_conn_cnt <= CONFIG_TCP_MAX_CONNS);

	if (!list_empty(&tcp_conn->list))
		list_del(&tcp_conn->list);
	free(tcp_conn);
	tcp_conn_cnt--;

#ifdef CONFIG_EVENT
	socket_schedule_ev(sock_info, EV_READ);
#endif
}

static tcp_conn_t *
tcp_conn_create(const tcp_uid_t *tuid, uint8_t status, sock_info_t *sock_info)
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
	conn->syn.status = status;
	conn->syn.tuid = *tuid;
	conn->sock_info = sock_info;
#ifdef CONFIG_TCP_RETRANSMIT
	tcp_retransmit_init(&conn->retrn);
#endif

	return conn;
}

#ifdef CONFIG_HT_STORAGE
int tcp_conn_add(tcp_conn_t *tcp_conn)
{
	sbuf_t key, val;

	sbuf_init(&key, &tcp_conn->syn.tuid, sizeof(tcp_uid_t));
	sbuf_init(&val, &tcp_conn, sizeof(tcp_conn_t *));
	if (htable_add(tcp_conns, &key, &val) < 0)
		return -1;
	return 0;
}

void tcp_conn_delete(tcp_conn_t *tcp_conn)
{
	sbuf_t key;

	sbuf_init(&key, &tcp_conn->syn.tuid, sizeof(tcp_uid_t));
	htable_del(tcp_conns, &key);
	__tcp_conn_delete(tcp_conn);
}

tcp_conn_t *tcp_conn_lookup(const tcp_uid_t *uid)
{
	sbuf_t key, *val;

	sbuf_init(&key, uid, sizeof(tcp_uid_t));
	if (htable_lookup(tcp_conns, &key, &val) < 0)
		return socket_tcp_conn_lookup(uid);
	return *(tcp_conn_t **)val->data;
}
#else /* CONFIG_HT_STORAGE */

int tcp_conn_add(tcp_conn_t *tcp_conn)
{
	list_add_tail(&tcp_conn->list, &tcp_conns);
	return 0;
}

void tcp_conn_delete(tcp_conn_t *tcp_conn)
{
	__tcp_conn_delete(tcp_conn);
}

tcp_conn_t *tcp_conn_lookup(const tcp_uid_t *uid)
{
	tcp_conn_t *tcp_conn;
	list_for_each_entry(tcp_conn, &tcp_conns, list) {
		/* tcp_conn must be packed */
		if (memcmp(uid, &tcp_conn->syn.tuid, sizeof(tcp_uid_t)) == 0)
			return tcp_conn;
	}
	return socket_tcp_conn_lookup(uid);
}

#endif

#ifdef CONFIG_TCP_CLIENT
static tcp_conn_t *tcp_client_conn_lookup(const tcp_uid_t *uid)
{
	tcp_conn_t *tcp_conn;
	list_for_each_entry(tcp_conn, &tcp_client_conns, list) {
		/* tcp_conn must be packed */
		if (memcmp(uid, &tcp_conn->syn.tuid, sizeof(tcp_uid_t)) == 0)
			return tcp_conn;
	}
	return NULL;
}
#endif

static int tcp_set_options(void *data, const tcp_options_t *tcp_opts)
{
	uint8_t *opts = data;
	uint16_t opts_len = 0;
	uint16_t mss;

	/* MSS */
	opts[0] = TCPOPT_MAXSEG;
	opts[1] = TCPOLEN_MAXSEG;
	opts += 2;
	mss = CONFIG_PKT_SIZE - sizeof(eth_hdr_t)
		- sizeof(ip_hdr_t) - sizeof(tcp_hdr_t) - TCPOLEN_MAXSEG;
	*(uint16_t *)opts = htons(mss);
	opts_len += TCPOLEN_MAXSEG;
	return opts_len;
}

#ifdef CONFIG_TCP_RETRANSMIT
static void __tcp_pkt_adj_reset(pkt_t *pkt, int len)
{
	pkt_adj(pkt, -(pkt->buf.skip));
	pkt_adj(pkt, len);
}

static void tcp_retransmit(void *tcp_conn);
static inline void tcp_arm_retrn_timer(tcp_conn_t *tcp_conn, pkt_t *pkt)
{
	if (pkt) {
		tcp_retrn_pkt_t *retrn_pkt;

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
		/* no retransmission for emergency packets */
		if (pkt_is_emergency(pkt))
			return;
#endif
		/* XXX TODO: check if there is room at the begining or
		 * the end of the packet to avoid this malloc. */
		if ((retrn_pkt = malloc(sizeof(tcp_retrn_pkt_t))) == NULL)
			return;
		tcp_conn->retrn.cnt = 0;
		INIT_LIST_HEAD(&retrn_pkt->list);
		retrn_pkt->pkt = pkt;
		pkt_retain(pkt);
		list_add_tail(&retrn_pkt->list,
			      &tcp_conn->retrn.retrn_pkt_list);
	}

	if (timer_is_pending(&tcp_conn->retrn.timer))
		return;

	timer_add(&tcp_conn->retrn.timer, CONFIG_TCP_RETRANSMIT_TIMEOUT * 1000UL
		  * (tcp_conn->retrn.cnt + 1), tcp_retransmit, tcp_conn);
}

static void tcp_conn_mark_closed(tcp_conn_t *tcp_conn, uint8_t error)
{
	tcp_conn->syn.status = SOCK_CLOSED;
#ifdef CONFIG_EVENT
	socket_schedule_ev(tcp_conn->sock_info, error ? EV_ERROR : EV_READ);
#endif
}

static void tcp_retransmit(void *arg)
{
	tcp_retrn_pkt_t *retrn_pkt;
	tcp_conn_t *tcp_conn = arg;

	if (tcp_conn->retrn.cnt >= TCP_IN_PROGRESS_RETRIES) {
		if (tcp_conn->syn.status != SOCK_CLOSED)
			tcp_conn_mark_closed(tcp_conn, 1);
		tcp_retrn_wipe(tcp_conn);
		return;
	}

	list_for_each_entry(retrn_pkt, &tcp_conn->retrn.retrn_pkt_list, list) {
		pkt_t *pkt = retrn_pkt->pkt;

		pkt_retain(pkt);
		/* adjust the pkt to ip header */
		__tcp_pkt_adj_reset(pkt, (int)sizeof(eth_hdr_t));

		ip_output(pkt, NULL, IP_DF);
	}

	tcp_conn->retrn.cnt++;
	tcp_arm_retrn_timer(tcp_conn, NULL);
}
#endif

static int
__tcp_output(pkt_t *pkt, uint32_t ip_dst, uint8_t ctrl, uint16_t sport,
	     uint16_t dport, tcp_syn_t *tcp_syn)
{
	tcp_hdr_t *tcp_hdr = btod(pkt);
	ip_hdr_t *ip_hdr;
	uint8_t tcp_hdr_len = sizeof(tcp_hdr_t);

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt);
	ip_hdr->dst = ip_dst;
	ip_hdr->p = IPPROTO_TCP;

	tcp_hdr->src_port = sport;
	tcp_hdr->dst_port = dport;
	tcp_hdr->seq = tcp_syn->seqid;
	tcp_hdr->ack = tcp_syn->ack;
	tcp_hdr->reserved = 0;
	tcp_hdr->ctrl = ctrl;
	tcp_hdr->win_size = (ctrl & TH_RST) ? 0 : htons(pkt->buf.size * 2);
	tcp_hdr->urg_ptr = 0;
	if (ctrl & TH_SYN) {
		int opts_len = tcp_set_options(tcp_hdr + 1, &tcp_syn->opts);

		tcp_hdr_len += opts_len;
		pkt->buf.len += opts_len;
	}
	tcp_hdr->hdr_len = tcp_hdr_len / 4;

	return ip_output(pkt, NULL, IP_DF);
}

int tcp_output(pkt_t *pkt, tcp_conn_t *tcp_conn, uint8_t flags)
{
#ifdef CONFIG_TCP_RETRANSMIT
	tcp_arm_retrn_timer(tcp_conn, pkt);
#endif
	/* XXX */
	return __tcp_output(pkt, tcp_conn->syn.tuid.src_addr, flags,
			    tcp_conn->syn.tuid.dst_port,
			    tcp_conn->syn.tuid.src_port, &tcp_conn->syn);
}

static int
tcp_send_pkt(const ip_hdr_t *ip_hdr, const tcp_hdr_t *tcp_hdr, uint8_t flags,
	     tcp_syn_t *tcp_syn)
{
	pkt_t *out;

	if ((out = pkt_alloc()) == NULL &&
	    (out = pkt_alloc_emergency()) == NULL) {
		return -1;
	}

	__tcp_adj_out_pkt(out);
	return __tcp_output(out, ip_hdr->src, flags, tcp_hdr->dst_port,
			    tcp_hdr->src_port, tcp_syn);
}

#ifdef CONFIG_TCP_RETRANSMIT
static void tcp_retrn_ack_pkts(tcp_conn_t *tcp_conn, uint32_t remote_ack)
{
	tcp_retrn_pkt_t *retrn_pkt, *retrn_pkt_tmp;

	list_for_each_entry_safe(retrn_pkt, retrn_pkt_tmp,
				 &tcp_conn->retrn.retrn_pkt_list, list) {
		pkt_t *pkt = retrn_pkt->pkt;
		tcp_hdr_t *tcp_hdr;
		int payload_len;
		uint32_t seqid;

		/* get tcp payload size */
		__tcp_pkt_adj_reset(pkt, (int)sizeof(eth_hdr_t) +
				    (int)sizeof(ip_hdr_t));
		tcp_hdr = btod(pkt);
		seqid = ntohl(tcp_hdr->seq);
		payload_len = pkt->buf.len - tcp_hdr->hdr_len * 4;

		if (seqid + payload_len <= remote_ack) {
			pkt_free(retrn_pkt->pkt);
			list_del(&retrn_pkt->list);
			free(retrn_pkt);
		}
#if 0
		else if (seqid >= remote_ack)
			break;
#endif
	}

	if (list_empty(&tcp_conn->retrn.retrn_pkt_list))
		timer_del(&tcp_conn->retrn.timer);
}
#endif


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

	tcp_conn = tcp_conn_create(&tuid, SOCK_TCP_SYN_SENT, sock_info);
	if (tcp_conn == NULL)
		return -1;

	if ((pkt = pkt_alloc()) == NULL) {
		free(tcp_conn);
		return -1;
	}

	list_add(&tcp_conn->list, &tcp_client_conns);
	tcp_conn->syn.seqid = rand();
	tcp_conn->syn.ack = 0;
	sock_info->trq.tcp_conn = tcp_conn;

	__tcp_adj_out_pkt(pkt);
	return tcp_output(pkt, tcp_conn, TH_SYN);
}
#endif

void tcp_input(pkt_t *pkt)
{
	tcp_hdr_t *tcp_hdr;
	ip_hdr_t *ip_hdr = btod(pkt);
	uint16_t ip_hdr_len = ip_hdr->hl * 4;
	uint16_t ip_plen = ntohs(ip_hdr->len) - ip_hdr_len;
	uint16_t tcp_hdr_len;
	sock_info_t *sock_info = NULL;
	tcp_uid_t tuid;
	tcp_syn_t *tsyn_entry;
	uint32_t remote_seqid, remote_ack;
#ifdef CONFIG_TCP_CLIENT
	uint32_t dst_addr;
#endif
	tcp_conn_t *tcp_conn;
	tcp_syn_t ts;

	STATIC_ASSERT(POWEROF2(CONFIG_TCP_SYN_TABLE_SIZE));

	pkt_adj(pkt, ip_hdr_len);
	tcp_hdr = btod(pkt);
	if (tcp_hdr->hdr_len < 4 || tcp_hdr->hdr_len > 15)
		goto end;

	tcp_hdr_len = tcp_hdr->hdr_len * 4;
	if (transport_cksum(ip_hdr, tcp_hdr, htons(ip_plen)) != 0)
		goto end;

	set_tuid(&tuid, ip_hdr, tcp_hdr);
	remote_seqid = ntohl(tcp_hdr->seq);
	remote_ack = ntohl(tcp_hdr->ack);

	ts = (tcp_syn_t){
		.seqid = tcp_hdr->ack,
		.ack = tcp_hdr->seq,
	};

	if ((tcp_conn = tcp_conn_lookup(&tuid)) != NULL) {
		int flags = 0, plen;
		uint32_t ack, seqid;

		if (tcp_hdr->ctrl & TH_RST) {
			if (tcp_conn->syn.status != SOCK_CLOSED)
				tcp_conn_mark_closed(tcp_conn, 1);
			goto end;
		}

		ack = ntohl(tcp_conn->syn.ack);
		seqid = ntohl(tcp_conn->syn.seqid);
		if (remote_seqid < ack) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK, &tcp_conn->syn);
			goto end;
		}
		if (remote_seqid > ack)
			goto end;

		if ((tcp_hdr->ctrl & TH_ACK)) {
			if (remote_ack > seqid) {
				/* drop the packet */
				goto end;
			}
#ifdef CONFIG_TCP_RETRANSMIT
			tcp_retrn_ack_pkts(tcp_conn, remote_ack);
#endif
		}
		if (tcp_hdr->ctrl & TH_FIN) {
			ack++;
			if (tcp_conn->syn.status == SOCK_CONNECTED) {
				flags |= TH_FIN|TH_ACK;
				tcp_conn->syn.status = SOCK_TCP_FIN_SENT;
			}
		} else if (tcp_hdr->ctrl == TH_ACK
			   && tcp_conn->syn.status == SOCK_TCP_FIN_SENT
			   && tcp_hdr->ack == tcp_conn->syn.seqid) {
			tcp_conn_mark_closed(tcp_conn, 0);
			goto end;
		}

		pkt_adj(pkt, tcp_hdr_len);
		/* truncate pkt to the tcp payload length */
		plen = ip_plen - tcp_hdr_len;
		if (pkt->buf.len > plen)
			pkt->buf.len = plen;

		ack += pkt->buf.len;
		tcp_conn->syn.ack = htonl(ack);
		if (remote_seqid < ack)
			tcp_send_pkt(ip_hdr, tcp_hdr, flags | TH_ACK,
				     &tcp_conn->syn);

		if (flags & TH_FIN) {
			seqid++;
			tcp_conn->syn.seqid = htonl(seqid);
		}

		if (pkt->buf.len == 0)
			goto end;

		pkt_adj(pkt, -(tcp_hdr_len + ip_hdr_len));
		socket_append_pkt(&tcp_conn->pkt_list_head, pkt);
#ifdef CONFIG_EVENT
		socket_schedule_ev(tcp_conn->sock_info, EV_READ);
#endif
		return;
	}

#ifdef CONFIG_TCP_CLIENT
	dst_addr = tuid.dst_addr;
	tuid.dst_addr = 0;
	tcp_conn = tcp_client_conn_lookup(&tuid);
	if (tcp_hdr->ctrl & TH_RST) {
		if (tcp_conn != NULL)
			tcp_conn_mark_closed(tcp_conn, 1);
		goto end;
	}
	tuid.dst_addr = dst_addr;

	if (tcp_hdr->ctrl == (TH_SYN|TH_ACK)) {
		uint32_t seqid;

		if (tcp_conn == NULL)
			goto end;

		if (tcp_conn->syn.status != SOCK_TCP_SYN_SENT
		    || htonl(remote_ack - 1) != tcp_conn->syn.seqid) {
			tcp_conn_mark_closed(tcp_conn, 0);
			goto end;
		}

		tcp_conn->syn.tuid.dst_addr = dst_addr;
		tcp_conn->syn.ack = htonl(remote_seqid + 1);
		tcp_conn->syn.status = SOCK_CONNECTED;
		tcp_parse_options(&tcp_conn->syn.opts, tcp_hdr,
				  tcp_hdr_len - sizeof(tcp_hdr_t));
		seqid = ntohl(tcp_conn->syn.seqid) + 1;
		tcp_conn->syn.seqid = htonl(seqid);
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_ACK, &tcp_conn->syn);
		list_del(&tcp_conn->list);
#ifdef CONFIG_TCP_RETRANSMIT
		tcp_retrn_ack_pkts(tcp_conn, remote_ack);
#endif
		if (tcp_conn_add(tcp_conn) < 0) {
			__tcp_conn_delete(tcp_conn);
			goto end;
		}
#ifdef CONFIG_EVENT
		socket_schedule_ev(tcp_conn->sock_info, EV_WRITE);
#endif
		goto end;
	}
#endif

	sock_info = tcpport2sockinfo(tcp_hdr->dst_port);
	if (tcp_hdr->ctrl == TH_SYN) {
		if (sock_info == NULL) {
			ts.ack = htonl(remote_seqid + 1);
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST|TH_ACK, &ts);
			goto end;
		}

		tsyn_entry = &syn_entries.conns[syn_entries.pos];

		/* network endian for seqid and ack */
#ifdef TEST
		tsyn_entry->seqid = 0x1207E77B;
#else
		tsyn_entry->seqid = rand();
#endif
		tsyn_entry->ack = htonl(remote_seqid + 1);
		tsyn_entry->status = SOCK_TCP_SYN_ACK_SENT;
		tcp_parse_options(&tsyn_entry->opts, tcp_hdr,
				  tcp_hdr_len - sizeof(tcp_hdr_t));
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_SYN|TH_ACK, tsyn_entry);
		tsyn_entry->seqid = htonl(ntohl(tsyn_entry->seqid) + 1);
		syn_entries.pos = (syn_entries.pos + 1)
			& (CONFIG_TCP_SYN_TABLE_SIZE - 1);
		set_tuid(&tsyn_entry->tuid, ip_hdr, tcp_hdr);
		goto end;
	}

	if (tcp_hdr->ctrl & TH_FIN) {
		ts.seqid = htonl(remote_seqid + 1);
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, &ts);
		goto end;
	}

	if (!(tcp_hdr->ctrl & TH_ACK)) {
		goto end;
	}

	if ((tsyn_entry = syn_find_entry(&tuid)) == NULL) {
		tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, &ts);
		goto end;
	}

	if (sock_info && tsyn_entry->status == SOCK_TCP_SYN_ACK_SENT) {
		listen_t *l = sock_info->listen;

		if (tcp_hdr->seq != tsyn_entry->ack ||
		    tcp_hdr->ack != tsyn_entry->seqid) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST|TH_ACK, &ts);
			goto end;
		}

		if (l == NULL || l->backlog >= l->backlog_max ||
		    (tcp_conn = tcp_conn_create(&tuid,
						SOCK_CONNECTED,
						sock_info)) == NULL) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, &ts);
			goto end;
		}
		if (socket_add_backlog(l, tcp_conn) < 0) {
			tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, &ts);
			tcp_conn_delete(tcp_conn);
			goto end;
		}
		tcp_conn->syn.seqid = tsyn_entry->seqid;
		tcp_conn->syn.ack = tcp_hdr->seq;
		tcp_conn->syn.opts = tsyn_entry->opts;
#ifdef CONFIG_EVENT
		socket_schedule_ev(sock_info, EV_READ);
#endif
		goto end;
	}
	tcp_send_pkt(ip_hdr, tcp_hdr, TH_RST, &ts);

	/* silently drop the packet */
 end:
	pkt_free(pkt);
}

#ifdef CONFIG_HT_STORAGE
void tcp_init(void)
{
	if ((tcp_conns = htable_create(CONFIG_MAX_SOCK_HT_SIZE)) == NULL)
		__abort();
}

void tcp_shutdown(void)
{
	htable_free(tcp_conns);

}
#endif
