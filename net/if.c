#include "config.h"
#include "pkt-mempool.h"

int if_init(iface_t *ifce, uint16_t (*send)(const buf_t *),
	    pkt_t *(*recv)(void))
{
	INIT_LIST_HEAD(&ifce->rx);
	INIT_LIST_HEAD(&ifce->tx);
	ifce->recv = recv;
	ifce->send = send;

	return 0;
}

void if_shutdown(iface_t *ifce)
{
	(void)ifce;
}
