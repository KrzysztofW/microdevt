#include "pkt-mempool.h"

static uint8_t *buffer_data;
static pkt_t *buffer_pool;
static ring_t *pkt_pool;
static unsigned pkt_total_size;

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
static pkt_t *emergency_pkt;

int pkt_is_emergency(pkt_t *pkt)
{
	return (pkt > buffer_pool + pkt_pool->mask + 1 || pkt < buffer_pool);
}
#endif

pkt_t *pkt_get(ring_t *ring)
{
	uint8_t offset;
	int ret = ring_getc(ring, &offset);

	if (ret < 0)
		return NULL;
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (offset == (typeof(offset))-1)
		return emergency_pkt;
#endif
	return &buffer_pool[offset];
}

int pkt_put(ring_t *ring, pkt_t *pkt)
{
	return ring_addc(ring, pkt->offset);
}

#ifdef PKT_DEBUG
pkt_t *__pkt_alloc(const char *func, int line)
{
	pkt_t *pkt;

	pkt = pkt_get(pkt_pool);
	DEBUG_LOG("%s() in %s:%d (pkt:%p)\n", __func__, func, line, pkt);
	return pkt;
}


int __pkt_free(pkt_t *pkt, const char *func, int line)
{
	if (pkt->refcnt) {
		pkt->refcnt--;
		return 0;
	}
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (pkt_is_emergency(pkt)) {
		assert(emergency_pkt);
		free(pkt);
		emergency_pkt = NULL;
		return 0;
	}
#endif
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
	if (pkt->refcnt) {
		pkt->refcnt--;
		return 0;
	}

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (pkt_is_emergency(pkt)) {
		assert(emergency_pkt);
		free(pkt);
		emergency_pkt = NULL;
		return 0;
	}
#endif
	buf_reset(&pkt->buf);
	return pkt_put(pkt_pool, pkt);
}

#endif

static void pkt_init_pkt(pkt_t *pkt, uint8_t *data)
{
	pkt->buf = BUF_INIT(data, pkt_total_size);
	pkt->refcnt = 0;

	INIT_LIST_HEAD(&pkt->list);
#ifdef DEBUG
	/* mark pkt list as poisoned */
	list_del(&pkt->list);
#endif
}

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
pkt_t *pkt_alloc_emergency(void)
{
	pkt_t *pkt;

	if (emergency_pkt)
		return NULL;

	pkt = malloc(sizeof(pkt_t) + pkt_total_size);
	if (pkt == NULL) /* unrecoverable error => reset */
		__abort();
	pkt_init_pkt(pkt, (uint8_t *)pkt + sizeof(pkt_t));
	pkt->offset = -1;
	emergency_pkt = pkt;
	return pkt;
}
#endif

void pkt_mempool_init(unsigned nb_pkts, unsigned pkt_size)
{
	unsigned i;

	pkt_pool = ring_create(nb_pkts);
	buffer_pool = malloc(nb_pkts * sizeof(pkt_t));
	buffer_data = malloc(nb_pkts * pkt_size);
	if (buffer_pool == NULL || buffer_data == NULL)
		__abort();
	pkt_total_size = pkt_size;
	for (i = 0; i < nb_pkts - 1; i++) {
		pkt_t *pkt = &buffer_pool[i];

		assert(i < (typeof(pkt->offset))(-1));
		pkt_init_pkt(pkt, &buffer_data[i * pkt_size]);
		pkt->offset = i;
		ring_addc(pkt_pool, i);
	}
}

#ifdef TEST
void pkt_mempool_shutdown(void)
{
	ring_free(pkt_pool);
	free(buffer_data);
	free(buffer_pool);
}
#endif
