#ifndef _TUN_H_
#define _TUN_H_
#include <poll.h>

void tun_alloc(char *dev, struct pollfd *tun_fds);

#endif
