#ifndef TUN
#include <avr/io.h>
#include <avr/pgmspace.h>
#else
#define PSTR(x) x
#endif

#include "net_apps.h"

#ifdef CONFIG_BSD_COMPAT
struct sockaddr_in addr_c;

#ifdef CONFIG_TCP
int tcp_server(uint16_t port)
{
	int fd = 0;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#ifdef DEBUG
		printf("can't create socket\n");
#endif
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
#ifdef DEBUG
		printf("can't bind\n");
#endif
		return -1;
	}
	if (listen(fd, 5) < 0) {
#ifdef DEBUG
		printf("can't listen\n");
#endif
		return -1;
	}
	return fd;
}

int tcp_fd;
int tcp_init(uint16_t port)
{
	tcp_fd = tcp_server(port);
	if (tcp_fd < 0) {
#ifdef DEBUG
		printf(PSTR("can't create TCP socket\n"));
#endif
		return -1;
	}
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
			printf("accepted connection from:0x%lX on port %u\n",
			       ntohl(addr.sin_addr.s_addr),
			       ntohs(addr.sin_port));
	} else
	if (socket_get_pkt(client_fd, &pkt, (struct sockaddr *)&addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		printf("got:%*s\n", sb.len, sb.data);
		if (socket_put_sbuf(client_fd, &sb, (struct sockaddr *)&addr) < 0)
			printf("can't put sbuf to socket\n");
		pkt_free(pkt);
	}
	/* close(client_fd); */
}
#endif	/* CONFIG_TCP */

#ifdef CONFIG_UDP
int udp_server(uint16_t port)
{
	int fd;
	struct sockaddr_in sockaddr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
#ifdef DEBUG
		printf("can't create socket\n");
#endif
		return -1;
	}
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&sockaddr,
		 sizeof(struct sockaddr_in)) < 0) {
#ifdef DEBUG
		printf("can't bind\n");
#endif
		return -1;
	}
	return fd;
}

int udp_client(struct sockaddr_in *sockaddr, uint32_t addr, uint16_t port)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
#ifdef DEBUG
		printf("can't create socket\n");
#endif
		return -1;
	}
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = addr;
	sockaddr->sin_port = port;

	return fd;
}

int udp_fd;
int udp_fd_client;
int udp_init(uint32_t dst_addr, uint16_t port)
{
	udp_fd = udp_server(port);
	udp_fd_client = udp_client(&addr_c, dst_addr, htons(port));

	if (udp_fd < 0 || udp_fd_client < 0) {
#ifdef DEBUG
		printf(PSTR("can't create UDP sockets\n"));
#endif
		return -1;
	}
	return 0;
}

void udp_app(void)
{
	pkt_t *pkt;
	struct sockaddr_in addr;
	static int a;

	if (socket_get_pkt(udp_fd, &pkt, (struct sockaddr *)&addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		if (socket_put_sbuf(udp_fd, &sb, (struct sockaddr *)&addr) < 0)
			printf("can't put sbuf to socket\n");
		pkt_free(pkt);
	}

	(void)a;
#if 0
	if (a == 100) {
		sbuf_t sb = SBUF_INITS("blabla\n");

		if (socket_put_sbuf(udp_fd_client, &sb, (struct sockaddr *)&addr_c) < 0)
			printf("can't put sbuf to socket\n");
		a = 0;
		if (socket_get_pkt(udp_fd_client, &pkt, (struct sockaddr *)&addr) >= 0) {
			sbuf_t sb = PKT2SBUF(pkt);
			char *s = (char *)sb.data;
			s[sb.len] = 0;
			printf("%s\n", s);
			pkt_free(pkt);
		}
	}
	a++;
#endif
}
#endif	/* CONFIG_UDP */

#else	/* CONFIG_BSD_COMPAT */

#ifdef CONFIG_TCP
sock_info_t sock_info_server;

int tcp_server(uint16_t port)
{
	if (sock_info_init(&sock_info_server, 0, SOCK_STREAM, htons(port)) < 0) {
		fprintf(stderr, "can't init tcp sock_info\n");
		return -1;
	}
	__sock_info_add(&sock_info_server);
	if (sock_info_listen(&sock_info_server, 5) < 0) {
		fprintf(stderr, "can't listen on tcp sock_info\n");
		return -1;
	}

	if (sock_info_bind(&sock_info_server) < 0) {
		fprintf(stderr, "can't start tcp server\n");
		return -1;
	}

	return 0;
}

int tcp_init(uint16_t port)
{
	if (tcp_server(port) < 0) {
#ifdef DEBUG
		printf(PSTR("can't create TCP socket\n"));
#endif
		return -1;
	}
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
				printf("accepted connection from:0x%lX on port %u\n",
				       ntohl(src_addr), ntohs(src_port));
			}
			continue;
		}
		if (sb[i].len == 0 &&
		    __socket_get_pkt(&sock_info_clients[i], &pkts[i], &src_addr,
				     &src_port) >= 0) {
			sb[i] = PKT2SBUF(pkts[i]);
			printf("[conn:%d]: got (len:%d):%.*s (pkt:%p)\n", i,
			       sb[i].len, sb[i].len, sb[i].data, pkts[i]);
		}
		if (__socket_put_sbuf(&sock_info_clients[i], &sb[i], src_addr,
				      src_port) < 0) {
			printf("can't put sbuf to socket (%d) (pkt:%p)\n", sb[i].len, pkts[i]);
			continue;
		}
		if (sb[i].len) {
			sbuf_reset(&sb[i]);
			pkt_free(pkts[i]);
		}
	}
}
#endif	/* CONFIG_TCP */
#ifdef CONFIG_UDP
sock_info_t sock_info_udp_server, sock_info_udp_client;
int udp_server(uint16_t port)
{
	if (sock_info_init(&sock_info_udp_server, 0, SOCK_DGRAM, htons(port)) < 0) {
		fprintf(stderr, "can't init udp sock_info\n");
		return -1;
	}
	__sock_info_add(&sock_info_udp_server);

	if (sock_info_bind(&sock_info_udp_server) < 0) {
		fprintf(stderr, "can't start udp server\n");
		return -1;
	}
	return 0;
}

uint32_t src_addr_c;
uint16_t src_port_c;

int udp_client(uint32_t dst_addr, uint16_t port)
{
	if (sock_info_init(&sock_info_udp_client, 0, SOCK_DGRAM, 0) < 0) {
		fprintf(stderr, "can't init udp sock_info\n");
		return -1;
	}
	__sock_info_add(&sock_info_udp_client);
	src_addr_c = dst_addr;
	src_port_c = htons(port);
	return 0;
}

int udp_init(uint32_t dst_addr, uint16_t port)
{
	if (udp_server(port) < 0 || udp_client(dst_addr, port) < 0)
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
			printf("can't put sbuf to udp socket\n");
		pkt_free(pkt);
	}
	(void)a;
#if 0
	if (a == 100) {
		sbuf_t sb = SBUF_INITS("blabla\n");

		if (__socket_put_sbuf(&sock_info_udp_client, &sb, src_addr_c,
				      src_port_c) < 0)
			printf("can't put sbuf to socket\n");
		a = 0;
		if (__socket_get_pkt(&sock_info_udp_client, &pkt, &src_addr_c,
				     &src_port_c) >= 0) {
			sbuf_t sb = PKT2SBUF(pkt);
			char *s = (char *)sb.data;
			s[sb.len] = 0;
			printf("%s\n", s);
			pkt_free(pkt);
		}
	}
	a++;
#endif
}

#endif	/* CONFIG_UDP */
#endif  /* CONFIG_BSD_COMPAT */
