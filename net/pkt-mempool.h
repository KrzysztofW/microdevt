#ifndef _BUF_MEMPOOL_H_
#define _BUF_MEMPOOL_H_
#include <stdint.h>
#include "../sys/buf.h"
#include "../sys/ring.h"
#include "../sys/list.h"

#define PKT_SIZE 256
#define NB_PKTS    3

typedef struct pkt {
	buf_t buf;
	uint8_t refcnt;
#ifndef RING_POOL
	struct list_head list;
#else
	uint8_t offset;
#endif
#ifdef DEBUG
	uint8_t pkt_nb;
#endif
} pkt_t;

#define btod(pkt, type) (type)((pkt)->buf.data + (pkt)->buf.skip)
#define pkt_adj(pkt, len) buf_adj(&(pkt)->buf, len)

extern unsigned char buffer_data[];
extern pkt_t buffer_pool[];
#ifndef RING_POOL
extern struct list_head *pkt_pool;
#else
extern ring_t *pkt_pool;
#endif

#define PKT2SBUF(pkt) (sbuf_t)			       \
	{					       \
		.data = pkt->buf.data + pkt->buf.skip, \
		.len  = pkt_len(pkt),                  \
	}

static inline int pkt_len(const pkt_t *pkt)
{
	return pkt->buf.len;
}

#ifndef RING_POOL
static inline pkt_t *pkt_get(struct list_head *head)
{
	pkt_t *pkt;

	if (list_empty(head))
		return NULL;

	pkt = list_first_entry(head, pkt_t, list);
	list_del(&pkt->list);
	return pkt;
}

static inline int pkt_put(struct list_head *head, pkt_t *pkt)
{
	list_add_tail(&pkt->list, head);
	return 0;
}

#else
static inline pkt_t *pkt_get(ring_t *ring)
{
	uint8_t offset;
	int ret = ring_getc_finish(ring, &offset);

	if (ret < 0)
		return NULL;
	return &buffer_pool[offset];
}

static inline int pkt_put(ring_t *ring, pkt_t *pkt)
{
	return ring_addc_finish(ring, pkt->offset);
}
#endif

#ifdef PKT_DEBUG
static inline pkt_t *__pkt_alloc(const char *func, int line)
{
	pkt_t *pkt;

	pkt = pkt_get(pkt_pool);
	if (pkt) {
		pkt->refcnt = 0;
	}
	printf("%s() in %s:%d\n", __func__, func, line);
	return pkt;
}

#define pkt_alloc() __pkt_alloc(__func__, __LINE__)
static inline int __pkt_free(pkt_t *pkt, const char *func, int line)
{
	if (pkt->refcnt == 0) {
		buf_reset(&pkt->buf);
		printf("%s() in %s:%d\n", __func__, func, line);
		return pkt_put(pkt_pool, pkt);
	}
	pkt->refcnt--;
	return 0;
}
#define pkt_free(pkt) __pkt_free(pkt, __func__, __LINE__)
#else
static inline pkt_t *pkt_alloc(void)
{
	pkt_t *pkt = pkt_get(pkt_pool);

	if (pkt)
		pkt->refcnt = 0;
	return pkt;
}
static inline int pkt_free(pkt_t *pkt)
{
	if (pkt->refcnt == 0) {
		buf_reset(&pkt->buf);
		return pkt_put(pkt_pool, pkt);
	}
	pkt->refcnt--;
	return 0;
}

#endif

static inline void pkt_retain(pkt_t *pkt)
{
	pkt->refcnt++;
}

void pkt_mempool_shutdown(void);
int pkt_mempool_init(void);

#endif
