#include <avr/io.h>
#include <avr/pgmspace.h>

#include "net_apps.h"

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
int tcp_init(void)
{
	tcp_fd = tcp_server(777);
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

	if (client_fd == -1) {
		if ((client_fd = accept(tcp_fd, (struct sockaddr *)&addr, &addr_len)) >= 0)
			printf("accepted connection from:0x%lX on port %u\n",
			       ntohl(addr.sin_addr.s_addr),
			       ntohs(addr.sin_port));
	}
	if (client_fd
	    && socket_get_pkt(client_fd, &pkt, (struct sockaddr *)&addr) >= 0) {
		sbuf_t sb = PKT2SBUF(pkt);

		printf("got:%*s\n", sb.len, sb.data);
		if (socket_put_sbuf(client_fd, &sb, (struct sockaddr *)&addr) < 0)
			printf("can't put sbuf to socket\n");
		pkt_free(pkt);
	}
	/* close(client_fd); */
}
#else
int tcp_init(void)
{
	return 0;
}

int tcp_server(uint16_t port)
{
	return 0;
}

void tcp_app(void) {}
#endif

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
	sockaddr->sin_port = htons(port);

	return fd;
}

int udp_fd;
int udp_fd_client;
int udp_init(void)
{
	udp_fd = udp_server(777);
	udp_fd_client = udp_client(&addr_c, 0x0b00a8c0, 777);

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


	if (a == 100) {
#if 0
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
#endif
	}
	a++;
}
#else
int udp_server(uint16_t port)
{
	return 0;
}

int udp_client(struct sockaddr_in *sockaddr, uint32_t addr, uint16_t port) {
	(void)sockaddr;
	(void)addr;
	(void)port;
	return 0;
}

int udp_init(void)
{
	return 0;
}

void udp_app(void) {}

#endif
