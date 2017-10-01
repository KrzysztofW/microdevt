#ifndef _NET_APPS_H_
#define _NET_APPS_H_
#include "net/socket.h"

int tcp_init(uint16_t port);
int tcp_server(uint16_t port);
void tcp_app(void);
int udp_server(uint16_t port);
#ifdef CONFIG_BSD_COMPAT
int udp_client(struct sockaddr_in *sockaddr, uint32_t addr, uint16_t port);
#endif
int udp_init(uint32_t dst_addr, uint16_t port);
void udp_app(void);
#endif
