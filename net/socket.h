#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "config.h"

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

int socket(int family, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void socket_put_pkt(pkt_t *pkt);
int socket_get_pkt(int fd, pkt_t **pkt, struct sockaddr *addr);
int socket_put_sbuf(int fd, const sbuf_t *sbuf, struct sockaddr *addr);
int socket_close(int fd);
int socket_append_pkt(const sbuf_t *fd, pkt_t *pkt);
void socket_init(void);
void socket_shutdown(void);

#endif
