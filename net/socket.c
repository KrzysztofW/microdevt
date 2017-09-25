#include "socket.h"
#include "eth.h"
#include "ip.h"
#ifdef CONFIG_UDP
#include "udp.h"
#endif
#include "../sys/list.h"
#include "../sys/hash-tables.h"

#ifdef CONFIG_HT_STORAGE
hash_table_t *fd_to_sock;
#else
struct list_head sock_list;
#endif

uint8_t cur_fd = 3;
uint8_t max_fds = 100;

#ifdef CONFIG_HT_STORAGE
#ifdef CONFIG_UDP
hash_table_t *udp_binds;
#endif
#ifdef CONFIG_TCP
hash_table_t *tcp_binds;
extern hash_table_t *tcp_conns;
#endif
#else
extern struct list_head tcp_conns;
#endif

#ifdef CONFIG_HT_STORAGE
static sock_info_t *fd2sockinfo(int fd)
{
	sbuf_t key = FD2SBUF(fd);
	sbuf_t *val;

	if (htable_lookup(fd_to_sock, &key, &val) < 0)
		return NULL;
	return *(sock_info_t **)val->data;
}

static sock_info_t *port2sockinfo(hash_table_t *ht, uint16_t port)
{
	sbuf_t key, *val;

	sbuf_init(&key, &port, sizeof(port));
	if (htable_lookup(ht, &key, &val) < 0)
		return NULL;
	return *(sock_info_t **)val->data;
}

#ifdef CONFIG_TCP
sock_info_t *tcpport2sockinfo(uint16_t port)
{
	return port2sockinfo(tcp_binds, port);
}
#endif

sock_info_t *udpport2sockinfo(uint16_t port)
{
	return port2sockinfo(udp_binds, port);
}

static int sock_info_add(int fd, sock_info_t *sock_info)
{
	sbuf_t key = FD2SBUF(fd);
	sbuf_t val = SOCKINFO2SBUF(sock_info);

	sock_info->fd = fd;
	sock_info->port = 0;
	sock_info->listen = NULL;
	if (htable_add(fd_to_sock, &key, &val) < 0)
		return -1;
	return 0;
}

static hash_table_t *get_hash_table(const sock_info_t *sock_info)
{
	switch (sock_info->type) {
#ifdef CONFIG_UDP
	case SOCK_DGRAM:
		return udp_binds;
#endif
#ifdef CONFIG_TCP
	case SOCK_STREAM:
		return tcp_binds;
#endif
	default:
		return NULL;
	}
}

static int bind_on_port(uint16_t port, sock_info_t *sock_info)
{
	hash_table_t *ht = get_hash_table(sock_info);
	sbuf_t key, val;

	if (ht == NULL)
		return -1;
	sbuf_init(&key, &port, sizeof(port));
	val = SOCKINFO2SBUF(sock_info);

	if (htable_add(ht, &key, &val) < 0)
		return -1;
	sock_info->port = port;
	return 0;
}

static int unbind_port(sock_info_t *sock_info)
{
	sbuf_t key;
	hash_table_t *ht = get_hash_table(sock_info);

	if (ht == NULL)
		return -1;
	if (sock_info->type == SOCK_STREAM && sock_info->trq.tcp_conn) {
		tcp_conn_delete(sock_info->trq.tcp_conn);
		sock_info->trq.tcp_conn = NULL;
	}

	sbuf_init(&key, &sock_info->port, sizeof(sock_info->port));
	htable_del(ht, &key);
	sbuf_init(&key, &sock_info->fd, sizeof(sock_info->fd));
	if (htable_del(fd_to_sock, &key) < 0)
		return -1;
	return 0;
}

#else
static sock_info_t *fd2sockinfo(int fd)
{
	sock_info_t *sock_info;

	list_for_each_entry(sock_info, &sock_list, list) {
		if (sock_info->fd == fd)
			return sock_info;
	}
	return NULL;
}

static sock_info_t *port2sockinfo(int type, uint16_t port)
{
	sock_info_t *sock_info;

	list_for_each_entry(sock_info, &sock_list, list) {
		if (sock_info->port == port && sock_info->type == type)
			return sock_info;
	}
	return NULL;
}

#ifdef CONFIG_TCP
sock_info_t *tcpport2sockinfo(uint16_t port)
{
	return port2sockinfo(SOCK_STREAM, port);
}
#endif

sock_info_t *udpport2sockinfo(uint16_t port)
{
	return port2sockinfo(SOCK_DGRAM, port);
}

static int sock_info_add(int fd, sock_info_t *sock_info)
{
	if (fd2sockinfo(fd))
		return -1;
	INIT_LIST_HEAD(&sock_info->list);
	sock_info->fd = fd;
	sock_info->port = 0;
#ifdef CONFIG_TCP
	sock_info->listen = NULL;
#endif
	list_add_tail(&sock_info->list, &sock_list);
	return 0;
}

static int bind_on_port(uint16_t port, sock_info_t *sock_info)
{
	if (port2sockinfo(port, sock_info->type))
		return -1;
	sock_info->port = port;
	return 0;
}

static int unbind_port(sock_info_t *sock_info)
{
	if (port2sockinfo(sock_info->port, sock_info->type) == NULL) {
		if (fd2sockinfo(sock_info->fd) == NULL)
			return -1;
	}
	sock_info->port = 0;
#ifdef CONFIG_TCP
	if (sock_info->type == SOCK_STREAM && sock_info->trq.tcp_conn) {
		tcp_conn_delete(sock_info->trq.tcp_conn);
		sock_info->trq.tcp_conn = NULL;
	}
#endif
	list_del(&sock_info->list);
	return 0;
}

#ifdef CONFIG_TCP
tcp_conn_t *socket_tcp_conn_lookup(const tcp_uid_t *uid)
{
	sock_info_t *sock_info;

	list_for_each_entry(sock_info, &sock_list, list) {
		listen_t *listen = sock_info->listen;
		tcp_conn_t *tcp_conn;

		if (listen == NULL)
			continue;
		list_for_each_entry(tcp_conn, &listen->tcp_conn_list_head,
				    list) {
			if (memcmp(uid, &tcp_conn->uid, sizeof(tcp_uid_t)) == 0)
				return tcp_conn;
		}
	}
	return NULL;
}
#endif
#endif

int socket(int family, int type, int protocol)
{
	sock_info_t *sock_info;
	uint8_t fd;
	int retries = 0;

	(void)protocol;
	if (family != AF_INET)
		return -1;

	if (family >= SOCK_LAST)
		return -1;

	if ((sock_info = malloc(sizeof(sock_info_t))) == NULL)
		return -1;

 again:
	fd = cur_fd;
	if (sock_info_add(fd, sock_info) < 0) {
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
	switch (type) {
#ifdef CONFIG_UDP
	case SOCK_DGRAM:
		INIT_LIST_HEAD(&sock_info->trq.pkt_list);
		break;
#endif
#ifdef CONFIG_TCP
	case SOCK_STREAM:
		sock_info->trq.tcp_conn = NULL;
		break;
#endif
	default:
		free(sock_info);
		return -1;
	}
	sock_info->type = type;
	sock_info->family = family;

	cur_fd++;
	return fd;
}

#ifdef CONFIG_TCP
int listen(int fd, int backlog)
{
	sock_info_t *sock_info = fd2sockinfo(fd);
	listen_t *listen;

	if (sock_info == NULL)
		return -1;
	if ((listen = malloc(sizeof(listen_t))) == NULL)
		return -1;

	INIT_LIST_HEAD(&listen->tcp_conn_list_head);
	sock_info->listen = listen;
	listen->backlog = 0;
	listen->backlog_max = backlog;
	return 0;
}

static void socket_listen_free(listen_t *listen)
{
	tcp_conn_t *tcp_conn, *tcp_conn_tmp;

	if (listen == NULL)
		return;
	if (list_empty(&listen->tcp_conn_list_head)) {
		free(listen);
		return;
	}
	list_for_each_entry_safe(tcp_conn, tcp_conn_tmp,
				 &listen->tcp_conn_list_head, list) {
		list_del(&tcp_conn->list);
		free(tcp_conn);
	}
	free(listen);
}
#endif

int max_retries = CONFIG_EPHEMERAL_PORT_END - CONFIG_EPHEMERAL_PORT_START;

static int sockinfo_bind(sock_info_t *sock_info)
{
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


		ret = bind_on_port(p, sock_info);
		if (ret < 0)
			retries++;
		else {
			sock_info->port = p;
			break;
		}
	} while (client && retries < max_retries);

	return ret;
}

static int sockinfo_unbind(sock_info_t *sock_info)
{
	return unbind_port(sock_info);
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

	/* TODO check sin_addr for ip addresses on available interfaces */
	return bind_on_port(sockaddr->sin_port, sock_info);
}

#ifdef CONFIG_TCP
int socket_add_backlog(listen_t *listen, tcp_conn_t *tcp_conn)
{
	if (listen->backlog >= listen->backlog_max)
		return -1;
	list_add_tail(&tcp_conn->list, &listen->tcp_conn_list_head);
	listen->backlog++;
	return 0;
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

	if (sock_info->listen == NULL)
		return -1;

	if (list_empty(&sock_info->listen->tcp_conn_list_head)) {
		/* EAGAIN */
		return -1;
	}
	tcp_conn = list_first_entry(&sock_info->listen->tcp_conn_list_head,
				    tcp_conn_t, list);
	list_del(&tcp_conn->list);
	sock_info->listen->backlog--;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		tcp_conn_delete(tcp_conn);
		return -1;
	}
#ifndef CONFIG_HT_STORAGE
	list_add_tail(&tcp_conn->list, &tcp_conns);
#endif
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = tcp_conn->uid.src_addr;
	sockaddr->sin_port = tcp_conn->uid.src_port;
	*addrlen = sizeof(struct sockaddr_in);
	sock_info = fd2sockinfo(fd);
	sock_info->trq.tcp_conn = tcp_conn;
	tcp_conn->sock_info = sock_info;

	return fd;
}
#endif

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

	if (sockinfo->port == 0 && sockinfo_bind(sockinfo) < 0)
		goto error;

	switch (sockinfo->type) {
#ifdef CONFIG_UDP
	case SOCK_TYPE_UDP:
		pkt_adj(pkt, (int)sizeof(udp_hdr_t));
		if (buf_addsbuf(&pkt->buf, sbuf) < 0)
			goto error;

		pkt_adj(pkt, -(int)sizeof(udp_hdr_t));
		udp_output(pkt, addr_in->sin_addr.s_addr, sockinfo->port,
			   addr_in->sin_port);
		return 0;
#endif
#ifdef CONFIG_TCP
	case SOCK_TYPE_TCP:
		pkt_adj(pkt, (int)sizeof(tcp_hdr_t));
		if (buf_addsbuf(&pkt->buf, sbuf) < 0)
			goto error;

		pkt_adj(pkt, -(int)sizeof(tcp_hdr_t));
		tcp_output(pkt, addr_in->sin_addr.s_addr, TH_PUSH | TH_ACK, sockinfo->port,
			   addr_in->sin_port, 0, 0, IP_DF);
		tcp_conn_inc_seqid(sockinfo->trq.tcp_conn, sbuf->len);
		return 0;
#endif
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
#ifdef CONFIG_UDP
	udp_hdr_t *udp_hdr;
#endif
	ip_hdr_t *ip_hdr;
	pkt_t *pkt;
	int transport_hdr_len;

	if (sockinfo == NULL) {
		/* errno = CLOSED */
		return -1;
	}

#ifdef CONFIG_UDP
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
	} else
#endif
#ifdef CONFIG_TCP
	if (sockinfo->type == SOCK_STREAM) {
		tcp_conn_t *tcp_conn;
		tcp_hdr_t *tcp_hdr;

		if (sockinfo->trq.tcp_conn == NULL)
			return -1;
		tcp_conn = tcp_conn_lookup(&sockinfo->trq.tcp_conn->uid);
		if (tcp_conn == NULL)
			return -1;
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
#endif
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
	sock_info_t *sock_info;

	if ((sock_info = fd2sockinfo(fd)) == NULL)
		return -1;

	if (sockinfo_unbind(sock_info) >= 0 && fd == cur_fd - 1)
		cur_fd--;
#ifdef CONFIG_TCP
	socket_listen_free(sock_info->listen);
#endif
	free(sock_info);
	return 0;
}

int close(int fd)
{
	return socket_close(fd);
}

void socket_append_pkt(struct list_head *list_head, pkt_t *pkt)
{
	list_add_tail(&pkt->list, list_head);
}

int socket_init(void)
{
#ifdef CONFIG_HT_STORAGE
	if ((fd_to_sock = htable_create(CONFIG_MAX_SOCK_HT_SIZE)) == NULL)
		return -1;
#ifdef CONFIG_UDP
	if ((udp_binds = htable_create(CONFIG_MAX_SOCK_HT_SIZE)) == NULL)
		return -1;
#endif
#ifdef CONFIG_TCP
	if ((tcp_binds = htable_create(CONFIG_MAX_SOCK_HT_SIZE)) == NULL)
		return -1;
	if ((tcp_conns = htable_create(CONFIG_MAX_SOCK_HT_SIZE)) == NULL)
		return -1;
#endif
#else
	INIT_LIST_HEAD(&sock_list);
#ifdef CONFIG_TCP
	INIT_LIST_HEAD(&tcp_conns);
#endif
#endif
	return 0;
}

#if 0 /* this code prevents from finding leaks */
static void socket_free_cb(sbuf_t *key, sbuf_t *val)
{
	sock_info_t *sock_info = SBUF2SOCKINFO(val);

	(void)key;
	socket_listen_free(sock_info->listen);
	free(sock_info);
}
#endif

void socket_shutdown(void)
{
#ifdef CONFIG_HT_STORAGE
#ifdef CONFIG_UDP
	htable_free(udp_binds);
#endif
#ifdef CONFIG_TCP
	htable_free(tcp_binds);
	htable_free(tcp_conns);
#endif
#if 0 /* this code prevents from finding leaks */
	htable_for_each(fd_to_sock, socket_free_cb);
#endif
	htable_free(fd_to_sock);
#else
	sock_info_t *sock_info, *si_tmp;

	list_for_each_entry_safe(sock_info, si_tmp, &sock_list, list) {
		list_del(&sock_info->list);
#ifdef CONFIG_TCP
		socket_listen_free(sock_info->listen);
#endif
		free(sock_info);
	}
#endif
}
