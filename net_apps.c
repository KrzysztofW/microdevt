#ifdef CONFIG_AVR_MCU
#include <avr/io.h>
#include <avr/pgmspace.h>
#endif

#include "net_apps.h"
#include <log.h>
#include "timer.h"

uint16_t port = 777; /* little endian */
uint32_t client_addr = 0x01020101; /* big endian */

#ifdef CONFIG_BSD_COMPAT
#include "sys/errno.h"
struct sockaddr_in addr_c;

#ifdef CONFIG_TCP
int tcp_server(void)
{
	int fd = 0;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_LOG("can't create socket\n");
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
		DEBUG_LOG("can't bind\n");
		return -1;
	}
	if (listen(fd, 5) < 0) {
		DEBUG_LOG("can't listen\n");
		return -1;
	}
	return fd;
}

#ifdef CONFIG_TCP_CLIENT
static int tcp_client_fd = 1;
static tim_t tcp_client_timer;
static struct sockaddr_in tcp_client_sockaddr;
#define TCP_TIMER_RECONNECT 5	/* reconnect every 5 seconds */

static int tcp_app_client_init(void);
static void tcp_client_send_buf_cb(void *arg);

static void tcp_client_connect_cb(void *arg)
{
	(void)arg;

	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (connect(tcp_client_fd, (struct sockaddr *)&tcp_client_sockaddr,
		    sizeof(struct sockaddr_in)) >= 0) {
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
			  tcp_client_send_buf_cb, NULL);
		return;
	}
	DEBUG_LOG("can't connect errno:%d\n", errno);
	if (errno != EAGAIN) {
		close(tcp_client_fd);
		tcp_app_client_init();
		return;
	}

	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000UL);
}

static int tcp_app_client_init(void)
{

	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if ((tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG_LOG("TCP client: can't create socket\n");
		return -1;
	}
	tcp_client_sockaddr.sin_family = AF_INET;
	tcp_client_sockaddr.sin_addr.s_addr = 0x01020101;
	tcp_client_sockaddr.sin_port = htons(port + 1);

	timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
		  tcp_client_connect_cb, NULL);

	return 0;
}

static void tcp_client_send_buf_cb(void *arg)
{
	sbuf_t sb = SBUF_INITS("blabla\n");
	pkt_t *pkt;

	(void)arg;
	if (socket_put_sbuf(tcp_client_fd, &sb, &tcp_client_sockaddr) < 0) {
		if (errno != EAGAIN) {
			close(tcp_client_fd);
			tcp_app_client_init();
			return;
		}
		goto reschedule;
	}
	if (socket_get_pkt(tcp_client_fd, &pkt, &tcp_client_sockaddr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
		pkt_free(pkt);
	}
 reschedule:
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000);
}
#endif

int tcp_fd;
int tcp_init(void)
{
	tcp_fd = tcp_server();
	if (tcp_fd < 0) {
		DEBUG_LOG("can't create TCP socket\n");
		return -1;
	}
	tcp_app_client_init();
	return 0;
}

void tcp_app(void)
{
	pkt_t *pkt;
	struct sockaddr_in addr;
	socklen_t addr_len;
	static int client_fd = -1;

	if (client_fd < 0) {
		if ((client_fd = accept(tcp_fd, (struct sockaddr *)&addr, &addr_len)) >= 0)
			DEBUG_LOG("accepted connection from:0x%lX on port %u\n",
			       ntohl(addr.sin_addr.s_addr),
			       (uint16_t)ntohs(addr.sin_port));
	}
	if (socket_get_pkt(client_fd, &pkt, &addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		DEBUG_LOG("got:%.*s\n", sb.len, sb.data);
		if (socket_put_sbuf(client_fd, &sb, &addr) < 0)
			DEBUG_LOG("can't put sbuf to socket\n");
		pkt_free(pkt);
		return;
	}
	if (errno == EBADF) {
		close(client_fd);
		client_fd = -1;
	}
}
#endif

#ifdef CONFIG_UDP
int udp_server(void)
{
	int fd;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		DEBUG_LOG("can't create socket\n");
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
		DEBUG_LOG("can't bind\n");
		return -1;
	}
	return fd;
}

int udp_client(struct sockaddr_in *sockaddr)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		DEBUG_LOG("can't create socket\n");
		return -1;
	}
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = client_addr;
	sockaddr->sin_port = htons(port + 1);

	return fd;
}

int udp_fd;
int udp_fd_client;
int udp_init(void)
{
	udp_fd = udp_server();
	udp_fd_client = udp_client(&addr_c);

	if (udp_fd < 0 || udp_fd_client < 0) {
		DEBUG_LOG("can't create UDP sockets\n");
		return -1;
	}
	return 0;
}

void udp_app(void)
{
	pkt_t *pkt;
	struct sockaddr_in addr;
	static int a;

	if (socket_get_pkt(udp_fd, &pkt, &addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		if (socket_put_sbuf(udp_fd, &sb, &addr) < 0)
			DEBUG_LOG("can't put sbuf to socket\n");
		pkt_free(pkt);
	}

	(void)a;
#if 0
	if (a == 100) {
		sbuf_t sb = SBUF_INITS("blabla\n");

		if (socket_put_sbuf(udp_fd_client, &sb, &addr_c) < 0)
			DEBUG_LOG("can't put sbuf to socket\n");
		a = 0;
		if (socket_get_pkt(udp_fd_client, &pkt, &addr) >= 0) {
			sbuf_t sb = PKT2SBUF(pkt);
			char *s = (char *)sb.data;
			s[sb.len] = 0;
			DEBUG_LOG("%s\n", s);
			pkt_free(pkt);
		}
	}
	a++;
#endif
}
#endif	/* CONFIG_UDP */

#else	/* CONFIG_BSD_COMPAT */

#ifdef CONFIG_TCP

#ifdef CONFIG_TCP_CLIENT
static tim_t tcp_client_timer;
#define TCP_TIMER_RECONNECT 5	/* reconnect every 5 seconds */
sock_info_t sock_info_client;

static int tcp_app_client_init(void);
static void tcp_client_send_buf_cb(void *arg);

static void tcp_client_connect_cb(void *arg)
{
	(void)arg;

	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_connect(&sock_info_client, client_addr,
			      htons(port + 1)) >= 0) {
		timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
			  tcp_client_send_buf_cb, NULL);
		return;
	}
	DEBUG_LOG("can't connect\n");
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000UL);
}

static int tcp_app_client_init(void)
{
	DEBUG_LOG("%s:%d\n", __func__, __LINE__);
	if (sock_info_init(&sock_info_client, 0, SOCK_STREAM, htons(port)) < 0) {
		DEBUG_LOG("can't init tcp sock_info\n");
		return -1;
	}

	timer_add(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000,
		  tcp_client_connect_cb, NULL);

	return 0;
}

static void tcp_client_send_buf_cb(void *arg)
{
	sbuf_t sb = SBUF_INITS("blabla\n");
	pkt_t *pkt;

	(void)arg;
	if (__socket_put_sbuf(&sock_info_client, &sb, 0, 0) < 0) {
		if (sock_info_client.trq.tcp_conn == NULL) {
			tcp_app_client_init();
			return;
		}
		goto reschedule;
	}
	if (__socket_get_pkt(&sock_info_client, &pkt, NULL, NULL) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		DEBUG_LOG("tcp client got:%.*s\n", sb.len, sb.data);
		pkt_free(pkt);
	}
 reschedule:
	timer_reschedule(&tcp_client_timer, TCP_TIMER_RECONNECT * 1000000);
}
#endif

sock_info_t sock_info_server;

int tcp_server(void)
{
	if (sock_info_init(&sock_info_server, 0, SOCK_STREAM, htons(port)) < 0) {
		DEBUG_LOG("can't init tcp sock_info\n");
		return -1;
	}
	__sock_info_add(&sock_info_server);
	if (sock_info_listen(&sock_info_server, 5) < 0) {
		DEBUG_LOG("can't listen on tcp sock_info\n");
		return -1;
	}

	if (sock_info_bind(&sock_info_server) < 0) {
		DEBUG_LOG("can't start tcp server\n");
		return -1;
	}

	return 0;
}

int tcp_init(void)
{
	if (tcp_server() < 0) {
		DEBUG_LOG("can't create TCP socket\n");
		return -1;
	}
	tcp_app_client_init();
	return 0;
}

#define TCP_CLIENTS 5
sock_info_t sock_info_clients[TCP_CLIENTS];
sbuf_t sb[TCP_CLIENTS];
pkt_t *pkts[TCP_CLIENTS];
void tcp_app(void)
{
	uint32_t src_addr;
	uint16_t src_port;
	int i;

	for (i = 0; i < TCP_CLIENTS; i++) {
		if (sock_info_clients[i].trq.tcp_conn == NULL) {
			if (sock_info_accept(&sock_info_server, &sock_info_clients[i],
					     &src_addr, &src_port) >= 0) {
				DEBUG_LOG("accepted connection from:0x%lX on port %u\n",
				       ntohl(src_addr), (uint16_t)ntohs(src_port));
			}
			continue;
		}
		if (sb[i].len == 0 &&
		    __socket_get_pkt(&sock_info_clients[i], &pkts[i], &src_addr,
				     &src_port) >= 0) {
			sb[i] = PKT2SBUF(pkts[i]);
			DEBUG_LOG("[conn:%d]: got (len:%d):%.*s (pkt:%p)\n", i,
				  sb[i].len, sb[i].len, sb[i].data, pkts[i]);
		}
		if (__socket_put_sbuf(&sock_info_clients[i], &sb[i], 0,
				      0) < 0) {
			DEBUG_LOG("can't put sbuf to socket (len:%d) (from pkt:%p)\n",
				  sb[i].len, pkts[i]);
			if (socket_info_state(&sock_info_clients[i]) != SOCK_CONNECTED) {
				sock_info_unbind(&sock_info_clients[i]);
				goto reset_sb;
			}
			continue;
		}
	reset_sb:
		if (sb[i].len) {
			sbuf_reset(&sb[i]);
			pkt_free(pkts[i]);
		}
	}
}
#endif	/* CONFIG_TCP */

#ifdef CONFIG_UDP
sock_info_t sock_info_udp_server, sock_info_udp_client;
int udp_server(void)
{
	if (sock_info_init(&sock_info_udp_server, 0, SOCK_DGRAM, htons(port)) < 0) {
		DEBUG_LOG("can't init udp sock_info\n");
		return -1;
	}
	__sock_info_add(&sock_info_udp_server);

	if (sock_info_bind(&sock_info_udp_server) < 0) {
		DEBUG_LOG("can't start udp server\n");
		return -1;
	}
	return 0;
}

uint32_t src_addr_c;
uint16_t src_port_c;

int udp_client(void)
{
	if (sock_info_init(&sock_info_udp_client, 0, SOCK_DGRAM, 0) < 0) {
		DEBUG_LOG("can't init udp sock_info\n");
		return -1;
	}
	__sock_info_add(&sock_info_udp_client);
	src_addr_c = client_addr;
	src_port_c = htons(port);
	return 0;
}

int udp_init(void)
{
	if (udp_server() < 0 || udp_client() < 0)
		return -1;
	return 0;
}

void udp_app(void)
{
	uint32_t src_addr;
	uint16_t src_port;
	pkt_t *pkt;
	static int a;

	if (__socket_get_pkt(&sock_info_udp_server, &pkt, &src_addr,
			     &src_port) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		if (__socket_put_sbuf(&sock_info_udp_server, &sb, src_addr,
				      src_port) < 0)
			DEBUG_LOG("can't put sbuf to udp socket\n");
		pkt_free(pkt);
	}
	(void)a;
#if 0
	if (a == 100) {
		sbuf_t sb = SBUF_INITS("blabla\n");

		if (__socket_put_sbuf(&sock_info_udp_client, &sb, src_addr_c,
				      src_port_c) < 0)
			DEBUG_LOG("can't put sbuf to socket\n");
		a = 0;
		if (__socket_get_pkt(&sock_info_udp_client, &pkt, &src_addr_c,
				     &src_port_c) >= 0) {
			sbuf_t sb = PKT2SBUF(pkt);
			char *s = (char *)sb.data;
			s[sb.len] = 0;
			DEBUG_LOG("%s\n", s);
			pkt_free(pkt);
		}
	}
	a++;
#endif
}

#endif	/* CONFIG_UDP */
#endif  /* CONFIG_BSD_COMPAT */
