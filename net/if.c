#include "config.h"
#include "pkt-mempool.h"
#ifdef CONFIG_ETHERNET
#include "eth.h"
#endif
#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
#include "swen.h"
#endif
#include <scheduler.h>

static void if_refill_driver_pkt_pool(const iface_t *iface)
{
	pkt_t *pkt;

	while (!ring_is_full(iface->pkt_pool) && (pkt = pkt_alloc()))
		pkt_put(iface->pkt_pool, pkt);
}

void if_init(iface_t *ifce, uint8_t type,
	     unsigned nb_pkt_rx, unsigned nb_pkt_tx, unsigned nb_if_pkt_pool)
{
	switch (type) {
#ifdef CONFIG_ETHERNET
	case IF_TYPE_ETHERNET:
		assert(ifce->recv);
		assert(ifce->send);
		ifce->if_output = &eth_output;
		ifce->if_input = &eth_input;
		ifce->rx = ring_create(nb_pkt_rx);
		ifce->tx = ring_create(nb_pkt_tx);
		ifce->pkt_pool = ring_create(nb_if_pkt_pool);
		break;
#endif
#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
	case IF_TYPE_RF:
#ifdef CONFIG_RF_SENDER
		assert(ifce->send);
		ifce->if_output = &swen_output;
		ifce->tx = ring_create(nb_pkt_tx);
#endif
#ifdef CONFIG_RF_RECEIVER
		assert(ifce->recv);
		ifce->if_input = &swen_input;
		ifce->rx = ring_create(nb_pkt_rx);
		ifce->pkt_pool = ring_create(nb_if_pkt_pool);
#endif
		break;
#endif
	default:
		__abort();
	}
	if_refill_driver_pkt_pool(ifce);
}

static void if_schedule_receive_cb(void *arg)
{
	iface_t *iface = arg;
	if_refill_driver_pkt_pool(iface);
	iface->if_input(iface);
}

void if_schedule_receive(const iface_t *iface, pkt_t *pkt)
{
	pkt_put(iface->rx, pkt);
	schedule_task(if_schedule_receive_cb, (iface_t *)iface);
}

static void rf_pkt_free_cb(void *arg)
{
	pkt_free(arg);
}

void if_schedule_tx_pkt_free(pkt_t *pkt)
{
	schedule_task(rf_pkt_free_cb, pkt);
}


#ifdef TEST
void if_shutdown(iface_t *ifce)
{
	ring_free(ifce->rx);
	ring_free(ifce->tx);
	ring_free(ifce->pkt_pool);
}
#endif
