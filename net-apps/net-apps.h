#ifndef _NET_APPS_H_
#define _NET_APPS_H_
#include "../net/socket.h"

#ifdef CONFIG_AVR_MCU
#include <avr/io.h>
#include <avr/pgmspace.h>
#endif

int tcp_app_init(void);
int tcp_server(void);
void tcp_app(void);
int udp_server(void);
#ifdef CONFIG_BSD_COMPAT
int udp_client(struct sockaddr_in *sockaddr);
#endif
int udp_app_init(void);
void udp_app(void);

int dns_resolver_init(void);

#endif
