#include "pkt-mempool.h"

unsigned char buffer_data[CONFIG_PKT_SIZE * CONFIG_PKT_NB_MAX];
pkt_t buffer_pool[CONFIG_PKT_NB_MAX];
ring_t *pkt_pool;

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
static uint8_t emergency_pkt_used;

int pkt_is_emergency(pkt_t *pkt)
{
	return (pkt > buffer_pool + CONFIG_PKT_NB_MAX || pkt < buffer_pool);
}
#endif

pkt_t *pkt_get(ring_t *ring)
{
	uint8_t offset;
	int ret = ring_getc(ring, &offset);

	if (ret < 0)
		return NULL;
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
		assert(emergency_pkt_used);
		free(pkt);
		emergency_pkt_used = 0;
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
		assert(emergency_pkt_used);
		free(pkt);
		emergency_pkt_used = 0;
		return 0;
	}
#endif
	buf_reset(&pkt->buf);
	return pkt_put(pkt_pool, pkt);
}

#endif

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
pkt_t *pkt_alloc_emergency(void)
{
	pkt_t *pkt;

	if (emergency_pkt_used)
		return NULL;

	pkt = malloc(sizeof(pkt_t) + CONFIG_PKT_SIZE);
	if (pkt == NULL) /* unrecoverable error => reset */
		return NULL;
	pkt->buf = BUF_INIT(((uint8_t *)pkt) + sizeof(pkt_t), CONFIG_PKT_SIZE);
	pkt->refcnt = 0;

	INIT_LIST_HEAD(&pkt->list);
#ifdef DEBUG
	/* mark pkt list as poisoned */
	list_del(&pkt->list);
#endif
	emergency_pkt_used = 1;
	return pkt;
}
#endif

void pkt_mempool_init(void)
{
	uint8_t i;

	pkt_pool = ring_create(CONFIG_PKT_NB_MAX);
	for (i = 0; i < CONFIG_PKT_NB_MAX - 1; i++) {
		pkt_t *pkt = &buffer_pool[i];

		pkt->buf = BUF_INIT(&buffer_data[i * CONFIG_PKT_SIZE],
				    CONFIG_PKT_SIZE);
		pkt->refcnt = 0;
		INIT_LIST_HEAD(&pkt->list);
#ifdef DEBUG
		/* mark pkt list as poisoned */
		list_del(&pkt->list);
#endif
		pkt->offset = i;
		ring_addc(pkt_pool, i);
	}
}

#ifdef TEST
void pkt_mempool_shutdown(void)
{
	ring_free(pkt_pool);
}
#endif
