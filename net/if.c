#include "config.h"
#include "pkt-mempool.h"

int if_init(iface_t *ifce, uint16_t (*send)(const buf_t *),
	    pkt_t *(*recv)(void))
{
	ifce->rx = ring_create(CONFIG_PKT_NB_MAX + 1);
	ifce->tx = ring_create(CONFIG_PKT_NB_MAX + 1);
	if (ifce->tx == NULL|| ifce->rx == NULL)
		return -1;

	ifce->recv = recv;
	ifce->send = send;

	return 0;
}

void if_shutdown(iface_t *ifce)
{
	(void)ifce;
}
