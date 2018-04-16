#include "config.h"
#include "pkt-mempool.h"
#ifdef CONFIG_ETHERNET
#include "eth.h"
#endif
#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
#include "swen.h"
#endif
#include <scheduler.h>

#define IF_PKT_POOL_SIZE 4

void if_init(iface_t *ifce, uint8_t type)
{
	ifce->rx = ring_create(CONFIG_PKT_NB_MAX);
	ifce->tx = ring_create(CONFIG_PKT_NB_MAX);
	ifce->pkt_pool = ring_create(IF_PKT_POOL_SIZE);

	switch (type) {
#ifdef CONFIG_ETHERNET
	case IF_TYPE_ETHERNET:
		assert(ifce->recv);
		assert(ifce->send);
		ifce->if_output = &eth_output;
		ifce->if_input = &eth_input;
		break;
#endif
#if defined CONFIG_RF_SENDER || defined CONFIG_RF_RECEIVER
	case IF_TYPE_RF:
#ifdef CONFIG_RF_SENDER
		assert(ifce->send);
		ifce->if_output = &swen_output;
#endif
#ifdef CONFIG_RF_RECEIVER
		assert(ifce->recv);
		ifce->if_input = &swen_input;
#endif
		break;
#endif
	default:
		__abort();
	}
}

static void if_schedule_receive_cb(void *arg)
{
	iface_t *iface = arg;
	pkt_t *pkt;

	/* refill driver's pkt pool */
	while (!ring_is_full(iface->pkt_pool) && (pkt = pkt_alloc()))
		pkt_put(iface->pkt_pool, pkt);
	iface->if_input(iface);
}

void if_schedule_receive(const iface_t *iface, pkt_t **pktp)
{
	if (pktp) {
		pkt_put(iface->rx, *pktp);
		*pktp = NULL;
	}
	if (ring_len(iface->rx))
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
}
#endif
