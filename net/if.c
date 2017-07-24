#include "config.h"

int if_init(iface_t *ifce, int rx_size, int tx_size,
	    uint16_t (*send)(const buf_t *),
	    uint16_t (*recv)(buf_t *))
{
	ifce = calloc(1, sizeof(iface_t));
	if (ifce == NULL)
		return -1;

	if (buf_alloc(&ifce->rx_buf, rx_size) < 0) {
		return -1;
	}
	if (buf_alloc(&ifce->tx_buf, tx_size) < 0) {
		return -1;
	}
	ifce->recv = recv;
	ifce->send = send;
	return 0;
}

void if_shutdown(iface_t *ifce)
{
	buf_free(&ifce->rx_buf);
	buf_free(&ifce->tx_buf);
	free(ifce);
}
