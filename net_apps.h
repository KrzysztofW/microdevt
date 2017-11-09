#ifndef _NET_APPS_H_
#define _NET_APPS_H_
#include "net/socket.h"

int tcp_init(void);
int tcp_server(void);
void tcp_app(void);
int udp_server(void);
#ifdef CONFIG_BSD_COMPAT
int udp_client(struct sockaddr_in *sockaddr);
#endif
int udp_init(void);
void udp_app(void);

int dns_resolver_init(void);

#endif
