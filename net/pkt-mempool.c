#include "pkt-mempool.h"

unsigned char buffer_data[PKT_SIZE * NB_PKTS];
pkt_t buffer_pool[NB_PKTS];
#ifndef RING_POOL
struct list_head __pkt_pool;
struct list_head *pkt_pool = &__pkt_pool;
#else
ring_t *pkt_pool;
#endif

#ifndef RING_POOL
pkt_t *pkt_get(struct list_head *head)
{
	pkt_t *pkt;

	if (list_empty(head))
		return NULL;

	pkt = list_first_entry(head, pkt_t, list);
	list_del(&pkt->list);
	return pkt;
}

int pkt_put(struct list_head *head, pkt_t *pkt)
{
	assert(pkt->list.next == LIST_POISON1 && pkt->list.prev == LIST_POISON2);
	list_add_tail(&pkt->list, head);
	return 0;
}

#else
pkt_t *pkt_get(ring_t *ring)
{
	uint8_t offset;
	int ret = ring_getc_finish(ring, &offset);

	if (ret < 0)
		return NULL;
	return &buffer_pool[offset];
}

int pkt_put(ring_t *ring, pkt_t *pkt)
{
	return ring_addc(ring, pkt->offset);
}
#endif

#ifdef PKT_DEBUG
pkt_t *__pkt_alloc(const char *func, int line)
{
	pkt_t *pkt;

	pkt = pkt_get(pkt_pool);
	printf("%s() in %s:%d (pkt:%p)\n", __func__, func, line, pkt);
	return pkt;
}


int __pkt_free(pkt_t *pkt, const char *func, int line)
{
	buf_reset(&pkt->buf);
	return pkt_put(pkt_pool, pkt);
}
#else
pkt_t *pkt_alloc(void)
{
	return pkt_get(pkt_pool);
}

int pkt_free(pkt_t *pkt)
{
	buf_reset(&pkt->buf);
	return pkt_put(pkt_pool, pkt);
}

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
#ifdef DEBUG
		/* mark pkt list as poisoned */
		list_del(&pkt->list);
#endif
		pkt_put(pkt_pool, pkt);
#else
		pkt->offset = i;
		ring_addc(pkt_pool, i);
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
