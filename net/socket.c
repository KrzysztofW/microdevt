#include "socket.h"
#include "eth.h"
#include "ip.h"
#include "udp.h"
#include "../sys/list.h"
#include "../sys/hash-tables.h"

typedef enum sock_type {
	SOCK_TYPE_NONE,
	SOCK_TYPE_TCP,
	SOCK_TYPE_UDP,
} sock_type_t;

typedef enum sock_status {
	CLOSED,
	CONNECTING,
	OPEN,
} sock_status_t;

typedef union uaddr {
	uint32_t ip4_addr;
#ifdef CONFIG_IPV6
	uint32_t ip6_addr[4];
#endif
} uaddr_t;

struct sock_info {
	uaddr_t  addr;
	uint16_t port;

	uint8_t family : 4; /* upto 15 families */
	uint8_t type   : 2; /* upto 3 types */
	uint8_t status : 2; /* upto 3 statuses */

	struct list_head pkt_list;
	/* TODO tx_pkt_list */
}  __attribute__((__packed__));

typedef struct sock_info sock_info_t;

#define MAX_SOCK_HT_SIZE 4
#define MAX_NB_SOCKETS   4

hash_table_t *fd_to_sock;
uint8_t cur_fd = 3;

#define FD2SBUF(fd) (sbuf_t)			\
	{					\
		.data = (void *)&fd,		\
		.len = sizeof(uint8_t),	        \
	}

#define SOCKINFO2SBUF(sockinfo) (sbuf_t)	\
	{					\
		.data = (void *)sockinfo,	\
		.len = sizeof(sock_info_t),     \
	}

static sock_info_t *fd2sockinfo(int fd)
{
	sbuf_t key = FD2SBUF(fd);
	sbuf_t *val;

	if (htable_lookup(fd_to_sock, &key, &val) < 0)
		return NULL;
	return (sock_info_t *)val->data;
}

int socket(int family, int type, int protocol)
{
	sock_info_t sock_info, *sinfo;
	uint8_t fd = cur_fd;
	sbuf_t key, val;

	(void)protocol;
	if (family != AF_INET)
		return -1;

	if (family >= SOCK_LAST)
		return -1;

	key = FD2SBUF(fd);
	val = SOCKINFO2SBUF(&sock_info);
	if (htable_add(fd_to_sock, &key, &val) < 0)
		return -1;

	sinfo = fd2sockinfo(fd);
	assert (sinfo != NULL);

	memset(sinfo, 0, sizeof(sock_info_t));
	sinfo->type = type;
	sinfo->family = family;
	INIT_LIST_HEAD(&sinfo->pkt_list);

	cur_fd++;
	return fd;
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

	/* TODO check sin_addr for ip addresses on available interfaces */

	if (sock_info->type == SOCK_DGRAM)
		return udp_bind(sockfd, sockaddr->sin_port);

	return -1;
}

int socket_put_sbuf(int fd, const sbuf_t *sbuf, struct sockaddr *addr)
{
	sock_info_t *sockinfo = fd2sockinfo(fd);
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	pkt_t *pkt;
	ip_hdr_t *ip_hdr;

	if (sockinfo == NULL)
		return -1;

	if ((pkt = pkt_alloc()) == NULL)
		return -1;

	pkt_adj(pkt, (int)sizeof(eth_hdr_t));
	ip_hdr = btod(pkt, ip_hdr_t *);
	pkt_adj(pkt, (int)sizeof(ip_hdr_t));

	switch (sockinfo->type) {
	case SOCK_TYPE_UDP:
		ip_hdr->p = IPPROTO_UDP;
		pkt_adj(pkt, (int)sizeof(udp_hdr_t));

		if (buf_addsbuf(&pkt->buf, sbuf) < 0) {
			goto error;
		}
		pkt_adj(pkt, -(int)sizeof(udp_hdr_t));
		udp_output(pkt, addr_in->sin_addr.s_addr, sockinfo->port,
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
		if (udp_unbind(sockinfo->port) < 0)
			return -1;
	} else
		return -1;

	if (fd == cur_fd - 1)
		cur_fd--;

	return 0;
}

int socket_append_pkt(const sbuf_t *fd, pkt_t *pkt)
{
	sock_info_t *sockinfo;
	sbuf_t *val;

	if (htable_lookup(fd_to_sock, fd, &val) < 0)
		return -1;

	sockinfo = (sock_info_t *)val->data;
	list_add_tail(&pkt->list, &sockinfo->pkt_list);

	return 0;
}

void socket_init(void)
{
	fd_to_sock = htable_create(MAX_SOCK_HT_SIZE);
}

void socket_shutdown(void)
{
	htable_free(fd_to_sock);
}
