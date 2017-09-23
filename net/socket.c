#include "socket.h"
#include "eth.h"
#include "ip.h"
#include "udp.h"
#include "../sys/list.h"
#include "../sys/hash-tables.h"

#define TRANSPORT_MAX_HT 4
#define MAX_SOCK_HT_SIZE (TRANSPORT_MAX_HT * 2)
#define MAX_NB_SOCKETS MAX_SOCK_HT_SIZE

hash_table_t *fd_to_sock;
uint8_t cur_fd = 3;
uint8_t max_fds = 100;
extern hash_table_t *udp_binds;
extern hash_table_t *tcp_binds;
extern hash_table_t *tcp_conns;

static inline sock_info_t *fd2sockinfo(int fd)
{
	sbuf_t key = FD2SBUF(fd);
	sbuf_t *val;

	if (htable_lookup(fd_to_sock, &key, &val) < 0)
		return NULL;
	return *(sock_info_t **)val->data;
}

int socket(int family, int type, int protocol)
{
	sock_info_t *sock_info;
	uint8_t fd;
	sbuf_t key, val;
	int retries = 0;

	(void)protocol;
	if (family != AF_INET)
		return -1;

	if (family >= SOCK_LAST)
		return -1;

	if ((sock_info = malloc(sizeof(sock_info_t))) == NULL)
		return -1;

	val = SOCKINFO2SBUF(sock_info);
 again:
	fd = cur_fd;
	key = FD2SBUF(fd);
	if (htable_add(fd_to_sock, &key, &val) < 0) {
		if (retries > max_fds) {
			free(sock_info);
			return -1;
		}
		retries++;
		cur_fd++;
		if (cur_fd > max_fds)
			cur_fd = 3;
		goto again;
	}
	memset(sock_info, 0, sizeof(sock_info_t));
	if (type == SOCK_DGRAM)
		INIT_LIST_HEAD(&sock_info->trq.pkt_list);
	sock_info->type = type;
	sock_info->family = family;

	cur_fd++;
	return fd;
}

int listen(int fd, int backlog)
{
	sock_info_t *sock_info = fd2sockinfo(fd);
	listen_t *listen;

	if (sock_info == NULL)
		return -1;
	if ((listen = malloc(sizeof(listen_t)))
	    == NULL)
		return -1;
	sock_info->listen = listen;
	listen->backlog = 0;
	listen->backlog_max = backlog;
	INIT_LIST_HEAD(&listen->tcp_conn_list_head);
	return 0;
}

int max_retries = CONFIG_EPHEMERAL_PORT_END - CONFIG_EPHEMERAL_PORT_START;

/* port - network endian format */
static int
transport_bind(hash_table_t *ht_binds, sock_info_t *sock_info)
{
	sbuf_t key, val;
	static unsigned ephemeral_port = CONFIG_EPHEMERAL_PORT_START;
	int ret = -1, retries = 0;
	uint16_t p = sock_info->port;
	int client = p ? 0 : 1;

	STATIC_ASSERT(CONFIG_EPHEMERAL_PORT_END > CONFIG_EPHEMERAL_PORT_START);
	do {
		if (client) {
			p = htons(ephemeral_port);
			ephemeral_port++;
			if (ephemeral_port >= EPHEMERAL_PORT_END)
				ephemeral_port = EPHEMERAL_PORT_START;
		}

		sbuf_init(&key, &p, sizeof(p));
		val = SOCKINFO2SBUF(sock_info);

		if ((ret = htable_add(ht_binds, &key, &val)) < 0)
			retries++;
		else {
			sock_info->port = p;
			break;
		}
	} while (client && retries < max_retries);

	return ret;
}

static int transport_unbind(hash_table_t *ht_binds, sock_info_t *sock_info)
{
	sbuf_t key;

	sbuf_init(&key, &sock_info->port, sizeof(sock_info->port));
	htable_del(ht_binds, &key);
	htable_del(fd_to_sock, &key);
	return 0;
}

static int udp_bind(sock_info_t *sock_info)
{
	return transport_bind(udp_binds, sock_info);
}

static int udp_unbind(sock_info_t *sock_info)
{
	return transport_unbind(udp_binds, sock_info);
}

static int tcp_bind(sock_info_t *sock_info)
{
	return transport_bind(tcp_binds, sock_info);
}

static int tcp_unbind(sock_info_t *sock_info)
{
	return transport_unbind(tcp_binds, sock_info);
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	sock_info_t *sock_info;
	struct sockaddr_in *sockaddr = (struct sockaddr_in *)addr;

	if (sockaddr->sin_family != AF_INET)
		return -1;

	if (addrlen != sizeof(struct sockaddr_in))
		return -1;

	if ((sock_info = fd2sockinfo(sockfd)) == NULL)
		return -1;

	sock_info->addr.ip4_addr = sockaddr->sin_addr.s_addr;
	sock_info->port = sockaddr->sin_port;
	sock_info->fd = sockfd;

	/* TODO check sin_addr for ip addresses on available interfaces */

	if (sock_info->type == SOCK_DGRAM)
		return udp_bind(sock_info);

	if (sock_info->type == SOCK_STREAM)
		return tcp_bind(sock_info);

	return -1;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	sock_info_t *sock_info;
	tcp_conn_t *tcp_conn;
	struct sockaddr_in *sockaddr = (struct sockaddr_in *)addr;
	int fd;

	if ((sock_info = fd2sockinfo(sockfd)) == NULL)
		return -1;

	if (sock_info->type != SOCK_STREAM)
		return -1;

	if (sock_info->listen == NULL || sock_info->listen->backlog == 0)
		return -1;

	if (list_empty(&sock_info->listen->tcp_conn_list_head))
		return -1;

	tcp_conn = list_first_entry(&sock_info->listen->tcp_conn_list_head,
				    tcp_conn_t, list);
	list_del(&tcp_conn->list);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		tcp_conn_delete(tcp_conn);
		return -1;
	}
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = tcp_conn->uid.src_addr;
	sockaddr->sin_port = tcp_conn->uid.src_port;
	*addrlen = sizeof(struct sockaddr_in);
	sock_info = fd2sockinfo(fd);
	sock_info->trq.tcp_conn = tcp_conn;
	tcp_conn->sock_info = sock_info;

	return fd;
}

int socket_put_sbuf(int fd, const sbuf_t *sbuf, const struct sockaddr *addr)
{
	sock_info_t *sockinfo = fd2sockinfo(fd);
	const struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	pkt_t *pkt;

	if (sockinfo == NULL)
		return -1;

	if ((pkt = pkt_alloc()) == NULL)
		return -1;

	pkt_adj(pkt, (int)sizeof(eth_hdr_t));
	pkt_adj(pkt, (int)sizeof(ip_hdr_t));

	switch (sockinfo->type) {
	case SOCK_TYPE_UDP:
		if (sockinfo->port == 0 && udp_bind(sockinfo) < 0)
			goto error;

		pkt_adj(pkt, (int)sizeof(udp_hdr_t));

		if (buf_addsbuf(&pkt->buf, sbuf) < 0)
			goto error;

		pkt_adj(pkt, -(int)sizeof(udp_hdr_t));
		udp_output(pkt, addr_in->sin_addr.s_addr, sockinfo->port,
			   addr_in->sin_port);
		return 0;

	case SOCK_TYPE_TCP:
		if (sockinfo->port == 0 && tcp_bind(sockinfo) < 0)
			goto error;

		pkt_adj(pkt, (int)sizeof(tcp_hdr_t));

		if (buf_addsbuf(&pkt->buf, sbuf) < 0)
			goto error;

		pkt_adj(pkt, -(int)sizeof(tcp_hdr_t));
		tcp_output(pkt, addr_in->sin_addr.s_addr, TH_PUSH | TH_ACK, sockinfo->port,
			   addr_in->sin_port, 0, 0, IP_DF);
		return 0;
	default:
		goto error;
	}

 error:
	pkt_free(pkt);
	return -1;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	       const struct sockaddr *dest_addr, socklen_t addrlen)
{
	sbuf_t sb;

	(void)flags;
	if (addrlen != sizeof(struct sockaddr_in))
		return -1;
	sbuf_init(&sb, buf, len);
	return socket_put_sbuf(sockfd, &sb, dest_addr);
}

int socket_get_pkt(int fd, pkt_t **pktp, struct sockaddr *addr)
{
	sock_info_t *sockinfo = fd2sockinfo(fd);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	udp_hdr_t *udp_hdr;
	ip_hdr_t *ip_hdr;
	pkt_t *pkt;
	int transport_hdr_len;

	if (sockinfo == NULL) {
		/* errno = CLOSED */
		return -1;
	}

	if (sockinfo->type == SOCK_DGRAM) {
		if (list_empty(&sockinfo->trq.pkt_list)) {
			/* errno = EAGAIN; */
			return -1;
		}

		pkt = list_first_entry(&sockinfo->trq.pkt_list, pkt_t, list);
		list_del(&pkt->list);

		*pktp = pkt;
		pkt_adj(pkt, -(int)sizeof(udp_hdr_t));
		udp_hdr = btod(pkt, udp_hdr_t *);
		addr_in->sin_port = udp_hdr->src_port;
		transport_hdr_len = (int)sizeof(udp_hdr_t);
	} else if (sockinfo->type == SOCK_STREAM) {
		sbuf_t key, *val;
		tcp_conn_t *tcp_conn;
		tcp_hdr_t *tcp_hdr;

		if (sockinfo->trq.tcp_conn == NULL)
			return -1;
		sbuf_init(&key, &sockinfo->trq.tcp_conn->uid,
			  sizeof(sockinfo->trq.tcp_conn->uid));
		if (htable_lookup(tcp_conns, &key, &val) < 0)
			return -1;
		tcp_conn = (tcp_conn_t *)val->data;
		if (list_empty(&tcp_conn->pkt_list_head))
			return -1;

		pkt = list_first_entry(&tcp_conn->pkt_list_head, pkt_t, list);
		list_del(&pkt->list);

		*pktp = pkt;
		pkt_adj(pkt, -(int)sizeof(tcp_hdr_t));
		tcp_hdr = btod(pkt, tcp_hdr_t *);
		addr_in->sin_port = tcp_hdr->src_port;
		transport_hdr_len = (int)sizeof(tcp_hdr_t);
	} else
		return -1;

	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);

	addr_in->sin_family = AF_INET;
	addr_in->sin_addr.s_addr = ip_hdr->src;

	pkt_adj(pkt, sizeof(ip_hdr_t));
	pkt_adj(pkt, transport_hdr_len);

	return 0;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
		 struct sockaddr *src_addr, socklen_t *addrlen)
{
	pkt_t *pkt;
	int __len;

	(void)flags;
	if (socket_get_pkt(sockfd, &pkt, src_addr) < 0)
		return -1;
	*addrlen = sizeof(struct sockaddr_in);
	__len = MIN((int)len, pkt->buf.len);
	memcpy(buf, buf_data(&pkt->buf), __len);
	pkt_free(pkt);
	return __len;
}

int socket_close(int fd)
{
	sock_info_t *sockinfo;

	if ((sockinfo = fd2sockinfo(fd)) == NULL)
		return -1;

	if (sockinfo->type == SOCK_DGRAM)
		udp_unbind(sockinfo);
	else if (sockinfo->type == SOCK_STREAM)
		tcp_unbind(sockinfo);

	if (fd == cur_fd - 1)
		cur_fd--;
	if (sockinfo->listen)
		free(sockinfo->listen);
	free(sockinfo);
	return 0;
}

void socket_append_pkt(struct list_head *list_head, pkt_t *pkt)
{
	list_add_tail(&pkt->list, list_head);
}

int socket_init(void)
{
	if ((fd_to_sock = htable_create(MAX_SOCK_HT_SIZE)) == NULL)
		return -1;
	if ((udp_binds = htable_create(TRANSPORT_MAX_HT)) == NULL)
		return -1;
	if ((tcp_binds = htable_create(TRANSPORT_MAX_HT)) == NULL)
		return -1;
	if ((tcp_conns = htable_create(TRANSPORT_MAX_HT)) == NULL)
		return -1;
	return 0;
}

#if 0 /* this code prevents from finding leaks */
static void socket_free_cb(sbuf_t *key, sbuf_t *val)
{
	sock_info_t *sock_info = SBUF2SOCKINFO(val);

	(void)key;
	if (sock_info->listen)
		free(sock_info->listen);
	free(sock_info);
}
#endif

void socket_shutdown(void)
{
	htable_free(udp_binds);
	htable_free(tcp_binds);
	htable_free(tcp_conns);
#if 0 /* this code prevents from finding leaks */
	htable_for_each(fd_to_sock, socket_free_cb);
#endif
	htable_free(fd_to_sock);
}
