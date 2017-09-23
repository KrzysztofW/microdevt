#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "config.h"
#include "tcp.h"

#define EPHEMERAL_PORT_START 49152
#define EPHEMERAL_PORT_END   65535

/*
 * Socket types
 */

#define SOCK_STREAM     1  /* stream socket */
#define SOCK_DGRAM      2  /* datagram socket */
#define SOCK_RAW        3  /* raw-protocol interface */
#define SOCK_LAST       4  /* must be at the end and can't exceed 16 */

/*
 * Address families
 */
#define AF_UNSPEC       0  /* unspecified */
#define AF_UNIX         1  /* standardized name for AF_LOCAL */
#define AF_INET         2  /* internetwork: UDP, TCP, etc. */
#define AF_INET6        10 /* IP version 6 */
#define AF_MAX          16 /* can't hold more than 15 families */

typedef uint32_t in_addr_t;
struct in_addr {
	in_addr_t s_addr;
};

typedef uint16_t in_port_t;
typedef uint8_t sa_family_t;
typedef uint8_t socklen_t;

#define INADDR_ANY ((uint32_t)0)

struct sockaddr {
	sa_family_t sa_family;  /* address family, AF_xxx       */
	char sa_data[14];       /* 14 bytes of protocol address */
}  __attribute__((__packed__));

/* Socket address, internet style. */
struct sockaddr_in {
	uint8_t        sin_len;
	sa_family_t    sin_family;
	in_port_t      sin_port;
	struct in_addr sin_addr;
	char           sin_zero[8];
} __attribute__((__packed__));

/* internal data structures */

typedef enum sock_type {
	SOCK_TYPE_NONE,
	SOCK_TYPE_TCP,
	SOCK_TYPE_UDP,
} sock_type_t;

typedef enum sock_status {
	SOCK_CLOSED,
	SOCK_TCP_SYN_SENT,
	SOCK_TCP_SYN_ACK_SENT,
	SOCK_TCP_FIN_SENT,
	SOCK_CONNECTED,
} sock_status_t;

typedef union uaddr {
	uint32_t ip4_addr;
#ifdef CONFIG_IPV6
	uint32_t ip6_addr[4];
#endif
} uaddr_t;

typedef union transport_queue {
	/* connectionless socket packet queue */
	struct list_head pkt_list;
	tcp_conn_t *tcp_conn;
} transport_queue_t;

struct listen {
	struct list_head tcp_conn_list_head;
	uint8_t backlog;
	uint8_t backlog_max;
} __attribute__((__packed__));
typedef struct listen listen_t;

struct sock_info {
	uaddr_t  addr;
	uint16_t port;

	uint8_t family : 4; /* upto 15 families */
	uint8_t type   : 4; /* upto 15 types */
	uint8_t status;
	uint8_t fd;

	listen_t *listen;
	transport_queue_t trq;
	/* TODO tx_pkt_list */
} __attribute__((__packed__));
typedef struct sock_info sock_info_t;

#define FD2SBUF(fd) (sbuf_t)			\
	{					\
		.data = (void *)&fd,		\
		.len = sizeof(uint8_t),	        \
	}

#define SOCKINFO2SBUF(sockinfo) (sbuf_t)	\
	{					\
		.data = (void *)&sockinfo,	\
		.len = sizeof(void *),          \
	}

#define SBUF2SOCKINFO(sb) *(sock_info_t **)(sb)->data

void socket_append_pkt(struct list_head *list_head, pkt_t *pkt);
int socket_init(void);
void socket_shutdown(void);

/* userland functions */
int socket(int family, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int fd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	       const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
		 struct sockaddr *src_addr, socklen_t *addrlen);
int socket_get_pkt(int fd, pkt_t **pkt, struct sockaddr *addr);
int socket_put_sbuf(int fd, const sbuf_t *sbuf, const struct sockaddr *addr);
int socket_close(int fd);

#endif
