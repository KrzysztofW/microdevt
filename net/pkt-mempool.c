#include "pkt-mempool.h"

unsigned char buffer_data[PKT_SIZE * NB_PKTS];
pkt_t buffer_pool[NB_PKTS];
#ifndef RING_POOL
struct list_head __pkt_pool;
struct list_head *pkt_pool = &__pkt_pool;
#else
ring_t *pkt_pool;
#endif

int pkt_mempool_init(void)
{
	uint8_t i;

#ifndef RING_POOL
	INIT_LIST_HEAD(pkt_pool);
#else
	pkt_pool = ring_create(NB_PKTS + 1);
	if (pkt_pool == NULL)
		return -1;
#endif
	STATIC_ASSERT(NB_PKTS < 256);
	for (i = 0; i < NB_PKTS; i++) {
		pkt_t *pkt = &buffer_pool[i];

		buf_init(&pkt->buf, &buffer_data[i * PKT_SIZE], PKT_SIZE);
		pkt->buf.len = 0;
#ifdef DEBUG
		pkt->pkt_nb = i;
#endif
#ifndef RING_POOL
		INIT_LIST_HEAD(&pkt->list);
		pkt_put(pkt_pool, pkt);
#else
		pkt->offset = i;
		ring_addc_finish(pkt_pool, i);
#endif
	}
	return 0;
}

void pkt_mempool_shutdown(void)
{
#ifdef RING_POOL
	ring_free(pkt_pool);
#endif
}
