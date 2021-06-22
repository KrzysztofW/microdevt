/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "config.h"
#ifdef CONFIG_TCP
#include "tcp.h"
#endif

#ifdef CONFIG_EVENT
#include "event.h"
#endif

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
#ifndef SOCKLEN_DEFINED
typedef uint8_t socklen_t;
#endif

#ifndef X86
typedef int ssize_t;
#endif

#define INADDR_ANY ((uint32_t)0)

struct sockaddr {
	sa_family_t sa_family;  /* address family, AF_xxx       */
	char sa_data[14];       /* 14 bytes of protocol address */
};

/* Socket address, internet style. */
struct sockaddr_in {
	uint8_t        sin_len;
	sa_family_t    sin_family;
	in_port_t      sin_port;
	struct in_addr sin_addr;
	char           sin_zero[8];
};

/* internal data structures */

typedef enum sock_type {
	SOCK_TYPE_NONE,
	SOCK_TYPE_TCP,
	SOCK_TYPE_UDP,
} sock_type_t;

#ifdef CONFIG_TCP
typedef enum sock_status {
	SOCK_CLOSED,
	SOCK_TCP_SYN_SENT,
	SOCK_TCP_SYN_ACK_SENT,
	SOCK_TCP_FIN_SENT,
	SOCK_CONNECTED,
} sock_status_t;
#endif

typedef union uaddr {
	uint32_t ip4_addr;
#ifdef CONFIG_IPV6
	uint32_t ip6_addr[4];
#endif
} uaddr_t;

#ifdef CONFIG_TCP
struct tcp_conn;
typedef struct tcp_conn tcp_conn_t;
struct tcp_uid;
typedef struct tcp_uid tcp_uid_t;
#endif

typedef union transport_queue {
#ifdef CONFIG_UDP
	/* connectionless socket packet queue */
	struct list_head pkt_list;
#endif
#ifdef CONFIG_TCP
	tcp_conn_t *tcp_conn;
#endif
} transport_queue_t;

#ifdef CONFIG_TCP
typedef struct listen {
	uint8_t backlog;
	uint8_t backlog_max;
	struct list_head tcp_conn_list_head;
} listen_t;
#endif

typedef struct sock_info {
#ifndef CONFIG_HT_STORAGE
	struct list_head list;
#endif
#if 0 /* only one ip address is allowed on an interface */
	uaddr_t  addr;
#endif
	uint16_t port;

	uint8_t type   : 4; /* upto 15 types */
#ifdef CONFIG_BSD_COMPAT
	uint8_t family : 4; /* upto 15 families */
	uint8_t fd;
#endif
#ifdef CONFIG_TCP
	listen_t *listen;
#endif
	transport_queue_t trq;
	/* TODO tx_pkt_list */
#ifdef CONFIG_EVENT
	event_t event;
#endif
} sock_info_t;

#define FD2SBUF(fd) (sbuf_t)			\
	{					\
		.data = (void *)&fd,		\
		.len = sizeof(uint8_t),         \
	}

#define SOCKINFO2SBUF(sockinfo) (sbuf_t)	\
	{					\
		.data = (void *)&sockinfo,	\
		.len = sizeof(void *),          \
	}

#define SBUF2SOCKINFO(sb) *(sock_info_t **)(sb)->data

#ifdef CONFIG_EVENT
/** Register an event on a socket
 *
 * @param[in]  sock_info    network socket
 * @param[in]  events       events to wake up on
 * @param[in]  ev_cb        function to be called on wake up
 */
void socket_event_register(sock_info_t *sock_info, uint8_t events,
			   void (*ev_cb)(event_t *ev, uint8_t events));

#ifdef CONFIG_BSD_COMPAT

/** Register an event on a file descriptor
 *
 * @param[in]  fd      file descriptor
 * @param[in]  events  events to wake up on
 * @param[in]  ev_cb   function to be called on wake up
 */
void socket_event_register_fd(int fd, uint8_t events,
			      void (*ev_cb)(event_t *ev, uint8_t events));
#endif

/** Unregister events on a socket
 *
 * @param[in] sock_info  network socket
 */
static inline void socket_event_unregister(sock_info_t *sock_info)
{
	event_unregister(&sock_info->event);
}

/** Set event mask on a socket
 *
 * @param[in] sock_info  network socket
 * @param[in] events     event mask to wake up on
 */
static inline void
socket_event_set_mask(sock_info_t *sock_info, uint8_t events)
{
	event_set_mask(&sock_info->event, events);
}

/** Clear event mask on a socket
 *
 * @param[in] sock_info  network socket
 * @param[in] events     events to be cleared
 */
static inline void
socket_event_clear_mask(sock_info_t *sock_info, uint8_t events)
{
	event_clear_mask(&sock_info->event, events);
}

/** Get socket from event
 *
 * @param[in] ev     event
 * @return network socket
 */
static inline sock_info_t *socket_event_get_sock_info(event_t *ev)
{
	return container_of(ev, sock_info_t, event);
}
#endif

void socket_append_pkt(struct list_head *list_head, pkt_t *pkt);
#ifdef CONFIG_HT_STORAGE
void socket_init(void);
#else
#define socket_init()
#endif
void socket_shutdown(void);
sock_info_t *udpport2sockinfo(uint16_t port);
sock_info_t *tcpport2sockinfo(uint16_t port);
#ifdef CONFIG_TCP
tcp_conn_t *socket_tcp_conn_lookup(const tcp_uid_t *uid);
#endif

/* user app functions */
#ifdef CONFIG_BSD_COMPAT
/** Initialize a BSD compatible network socket
 *
 * @param[in] family   family of the socket
 * @param[in] type     type of the socket
 * @param[in] protocol used protocol
 * @return a file descriptor on success, -1 on failure
 */
int socket(int family, int type, int protocol);

/** Close a BSD compatible network socket
 *
 * @param[in] fd  file descriptor
 * @return 0 on success, -1 on failure
 */
int close(int fd);

/** Bind a BSD compatible network socket
 *
 * @param[in] fd    file descriptor
 * @param[in] sockaddr  address
 * @param[in] addrlen   address length
 * @return 0 on success, -1 on failure
 */
int bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

/** Listen on a BSD compatible network socket
 *
 * @param[in] fd        file descriptor
 * @param[in] backlog   maximum number of unhandled client connections
 * @return 0 on success, -1 on failure
 */
int listen(int fd, int backlog);

/** Accept an incoming connection on a BSD compatible network socket
 *
 * @param[in] fd       file descriptor
 * @param[in] addr     address
 * @param[in] addrlen  address length
 * @return 0 on success, -1 on failure
 */
int accept(int fd, struct sockaddr *addr, socklen_t *addrlen);

/** Connect a BSD compatible network socket
 *
 * @param[in] fd       file descriptor
 * @param[in] addr     address
 * @param[in] addrlen  address length
 * @return 0 on success, -1 on failure
 */
int connect(int fd, const struct sockaddr *addr, socklen_t addrlen);

/** Send data on a BSD compatible network socket
 *
 * @param[in] fd       file descriptor
 * @param[in] buf      data buffer
 * @param[in] len      data length
 * @param[in] flags    BSD flags
 * @param[in] addr     dest address
 * @param[in] addrlen  address length
 * @return 0 on success, -1 on failure
 */
ssize_t sendto(int fd, const void *buf, size_t len, int flags,
	       const struct sockaddr *dest_addr, socklen_t addrlen);

/** Receive a buffer from a BSD compatible network socket
 *
 * @param[in]  fd       file descriptor
 * @param[out] buf      data buffer
 * @param[in]  len      data length
 * @param[in]  flags    BSD flags
 * @param[out] addr     source address
 * @param[out] addrlen  address length
 * @return 0 on success, -1 on failure
 */
ssize_t recvfrom(int fd, void *buf, size_t len, int flags,
		 struct sockaddr *src_addr, socklen_t *addrlen);

/** Get packet from a BSD compatible network socket
 *
 * @param[in]  fd      file descriptor
 * @param[out] pkt     packet
 * @param[out] addr    source address
 * @return 0 on success, -1 on failure
 */
int socket_get_pkt(int fd, pkt_t **pkt, struct sockaddr_in *addr);

/** Send data on a BSD compatible network socket
 *
 * @param[in]  fd      file descriptor
 * @param[in]  sbuf    static buffer
 * @param[out] addr    dest sockaddr
 * @return 0 on success, -1 on failure
 */
int
socket_put_sbuf(int fd, const sbuf_t *sbuf, const struct sockaddr_in *addr);
#endif

/** Get packet from a network socket
 *
 * @param[in]  sock_info  network socket
 * @param[out] pkt        packet
 * @param[out] src_addr   source address
 * @param[out] src_port   source port
 * @return 0 on success, -1 on failure
 */
int __socket_get_pkt(sock_info_t *sock_info, pkt_t **pkt,
		     uint32_t *src_addr, uint16_t *src_port);

/** Send data on a network socket
 *
 * @param[in]  sock_info  network socket
 * @param[in]  sbuf       static buffer
 * @param[in]  dst_addr   dest address
 * @param[in]  dst_port   dest port
 * @return 0 on success, -1 on failure
 */
int __socket_put_sbuf(sock_info_t *sock_info, const sbuf_t *sbuf,
		      uint32_t dst_addr, uint16_t dst_port);

/** Initialize a network socket
 *
 * @param[in] sock_info  network socket
 * @param[in] type       type of the socket
 * @return 0 on success, -1 on failure
 */
int sock_info_init(sock_info_t *sock_info, int sock_type);

/** Bind a network socket
 *
 * @param[in] sock_info  network socket
 * @param[in] port       local port
 * @return 0 on success, -1 on failure
 */
int sock_info_bind(sock_info_t *sock_info, uint16_t port);

/** Close a network socket
 *
 * @param[in] sock_info  network socket
 * @return 0 on success, -1 on failure
 */
int sock_info_close(sock_info_t *sock_info);

#ifdef CONFIG_TCP
/** Listen on a network socket
 *
 * @param[in] sock_info  network socket
 * @param[in] backlog    maximum number of unhandled client connections
 * @return 0 on success, -1 on failure
 */
int sock_info_listen(sock_info_t *sock_info, int backlog);

/** Accept a connection on a network socket
 *
 * @param[in]  sock_info_server  local network socket
 * @param[out] sock_info_client  client network socket
 * @param[out] src_addr          client address
 * @param[out] src_port          client port
 * @return 0 on success, -1 on failure
 */
int sock_info_accept(sock_info_t *sock_info_server,
		     sock_info_t *sock_info_client,
		     uint32_t *src_addr, uint16_t *src_port);
/** Connect a network socket
 *
 * @param[in]  sock_info  network socket
 * @param[in]  src_addr   address
 * @param[in]  src_port   port
 * @return 0 on success, -1 on failure
 */
int sock_info_connect(sock_info_t *sock_info, uint32_t addr, uint16_t port);

/** Get state of a network socket
 *
 * @param[in]  sock_info  network socket
 * @return socket status
 */
sock_status_t sock_info_state(const sock_info_t *sock_info);

void socket_add_backlog(listen_t *listen, tcp_conn_t *tcp_conn);
#endif
#endif
