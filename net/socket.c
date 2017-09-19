#include "socket.h"
#include "eth.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"
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

static inline sock_info_t *fd2sockinfo(int fd)
{
	sbuf_t key = FD2SBUF(fd);
	sbuf_t *val;

	if (htable_lookup(fd_to_sock, &key, &val) < 0)
		return NULL;
	return *(sock_info_t **)val->data;
}

static inline void sock_info_init(sock_info_t *sock_info)
{
	memset(sock_info, 0, sizeof(sock_info_t));
	INIT_LIST_HEAD(&sock_info->pkt_list);
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
	sock_info_init(sock_info);
	sock_info->type = type;
	sock_info->family = family;

	cur_fd++;
	return fd;
}

int max_retries = EPHEMERAL_PORT_END - EPHEMERAL_PORT_START;

/* port - network endian format */
static int
transport_bind(hash_table_t *ht_binds, sock_info_t *sock_info)
{
	sbuf_t key, val;
	static unsigned ephemeral_port = EPHEMERAL_PORT_START;
	int ret = -1, retries = 0;
	uint16_t p = sock_info->port;
	int client = p ? 0 : 1;

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

int socket_put_sbuf(int fd, const sbuf_t *sbuf, struct sockaddr *addr)
{
	sock_info_t *sockinfo = fd2sockinfo(fd);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
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
			   addr_in->sin_port);
		return 0;
	default:
		goto error;
	}

 error:
	pkt_free(pkt);
	return -1;
}

int socket_get_pkt(int fd, pkt_t **pktp, struct sockaddr *addr)
{
	sock_info_t *sockinfo = fd2sockinfo(fd);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	udp_hdr_t *udp_hdr;
	ip_hdr_t *ip_hdr;
	pkt_t *pkt;

	if (sockinfo == NULL)
		return -1;

	// XXX if udp:
	if (list_empty(&sockinfo->pkt_list))
		return -1;

	pkt = list_first_entry(&sockinfo->pkt_list, pkt_t, list);
	list_del(&pkt->list);

	*pktp = pkt;
	pkt_adj(pkt, -(int)sizeof(udp_hdr_t));
	udp_hdr = btod(pkt, udp_hdr_t *);
	pkt_adj(pkt, -(int)sizeof(ip_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);
	pkt_adj(pkt, sizeof(ip_hdr_t));
	pkt_adj(pkt, (int)sizeof(udp_hdr_t));

	addr_in->sin_family = AF_INET;
	addr_in->sin_port = udp_hdr->src_port;
	addr_in->sin_addr.s_addr = ip_hdr->src;

	return 0;
}

int socket_close(int fd)
{
	sock_info_t *sockinfo;

	if ((sockinfo = fd2sockinfo(fd)) == NULL)
		return -1;

	if (sockinfo->type == SOCK_DGRAM) {
		if (udp_unbind(sockinfo) < 0)
			return -1;
	} else if (sockinfo->type == SOCK_STREAM) {
		if (tcp_unbind(sockinfo) < 0)
			return -1;
	} else
		return -1;

	if (fd == cur_fd - 1)
		cur_fd--;

	return 0;
}

void socket_append_pkt(sock_info_t *sock_info, pkt_t *pkt)
{
	list_add_tail(&pkt->list, &sock_info->pkt_list);
}

int socket_init(void)
{
	if ((fd_to_sock = htable_create(MAX_SOCK_HT_SIZE)) == NULL)
		return -1;
	if ((udp_binds = htable_create(TRANSPORT_MAX_HT)) == NULL)
		return -1;
	if ((tcp_binds = htable_create(TRANSPORT_MAX_HT)) == NULL)
		return -1;
	return 0;
}

static void socket_free_cb(sbuf_t *key, sbuf_t *val)
{
	(void)key;
	free(SBUF2SOCKINFO(val));
}

void socket_shutdown(void)
{
	htable_free(udp_binds);
	htable_free(tcp_binds);

	htable_for_each(fd_to_sock, socket_free_cb);
	htable_free(fd_to_sock);
}
