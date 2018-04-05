#include "pkt-mempool.h"

unsigned char buffer_data[CONFIG_PKT_SIZE * CONFIG_PKT_NB_MAX];
pkt_t buffer_pool[CONFIG_PKT_NB_MAX];
#ifndef RING_POOL
list_t __pkt_pool;
list_t *pkt_pool = &__pkt_pool;
#else
ring_t *pkt_pool;
#endif

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
static uint8_t emergency_pkt_used;

int pkt_is_emergency(pkt_t *pkt)
{
	return (pkt > buffer_pool + CONFIG_PKT_NB_MAX || pkt < buffer_pool);
}
#endif

#ifndef RING_POOL
pkt_t *pkt_get(list_t *head)
{
	pkt_t *pkt;

	if (list_empty(head))
		return NULL;

	pkt = list_first_entry(head, pkt_t, list);
	list_del(&pkt->list);
	return pkt;
}

int pkt_put(list_t *head, pkt_t *pkt)
{
	assert(pkt->list.next == LIST_POISON1 && pkt->list.prev == LIST_POISON2);
	list_add_tail(&pkt->list, head);
	return 0;
}

#else
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
	buf_init(&pkt->buf, ((uint8_t *)pkt) + sizeof(pkt_t), CONFIG_PKT_SIZE);
	pkt->buf.len = 0;
	pkt->refcnt = 0;
#ifndef RING_POOL
	INIT_LIST_HEAD(&pkt->list);
#ifdef DEBUG
	/* mark pkt list as poisoned */
	list_del(&pkt->list);
#endif
#endif
	emergency_pkt_used = 1;
	return pkt;
}

int pkt_mempool_init(void)
{
	uint8_t i;

#ifndef RING_POOL
	INIT_LIST_HEAD(pkt_pool);
#else
	pkt_pool = ring_create(CONFIG_PKT_NB_MAX + 1);
	if (pkt_pool == NULL)
		return -1;
#endif
	for (i = 0; i < CONFIG_PKT_NB_MAX; i++) {
		pkt_t *pkt = &buffer_pool[i];

		buf_init(&pkt->buf, &buffer_data[i * CONFIG_PKT_SIZE],
			 CONFIG_PKT_SIZE);
		pkt->buf.len = 0;
		pkt->refcnt = 0;
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
