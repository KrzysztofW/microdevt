#ifndef _BUF_MEMPOOL_H_
#define _BUF_MEMPOOL_H_
#include <stdint.h>
#include "../sys/buf.h"
#include "../sys/ring.h"
#include "../sys/list.h"

#define PKT_SIZE 128
#define NB_PKTS    3

struct pkt {
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
} __attribute__((__packed__));
typedef struct pkt pkt_t;

#define btod(pkt, type) (type)((pkt)->buf.data + (pkt)->buf.skip)
#define pkt_adj(pkt, len) buf_adj(&(pkt)->buf, len)

#define PKT2SBUF(pkt) (sbuf_t)			       \
	{					       \
		.data = pkt->buf.data + pkt->buf.skip, \
		.len  = pkt_len(pkt),                  \
	}

void pkt_mempool_shutdown(void);
int pkt_mempool_init(void);
pkt_t *pkt_get(struct list_head *head);
int pkt_put(struct list_head *head, pkt_t *pkt);

#ifdef PKT_DEBUG
pkt_t *__pkt_alloc(const char *func, int line);
int __pkt_free(pkt_t *pkt, const char *func, int line);
#define pkt_alloc() __pkt_alloc(__func__, __LINE__)
#else
pkt_t *pkt_alloc(void);
int pkt_free(pkt_t *pkt);
#endif

static inline int pkt_len(const pkt_t *pkt)
{
	return pkt->buf.len;
}

static inline void pkt_retain(pkt_t *pkt)
{
	pkt->refcnt++;
}

#endif
