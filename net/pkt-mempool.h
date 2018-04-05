#ifndef _BUF_MEMPOOL_H_
#define _BUF_MEMPOOL_H_
#include <stdint.h>
#include <stdio.h>
#include "../sys/buf.h"
#include "../sys/ring.h"
#include "../sys/list.h"

/* List based PKT pools are not interrupt safe. */

/* #define RING_POOL */

struct pkt {
	buf_t buf;
#ifndef RING_POOL
	list_t list;
#else
	uint8_t offset;
#endif
#ifdef DEBUG
	uint8_t pkt_nb;
#endif
	uint8_t refcnt;
} __attribute__((__packed__));
typedef struct pkt pkt_t;

#define btod(pkt, type) (type)((pkt)->buf.data + (pkt)->buf.skip)
#define pkt_adj(pkt, len) buf_adj(&(pkt)->buf, len)

#define PKT2SBUF(pkt) (sbuf_t)			       \
	{					       \
		.data = pkt->buf.data + pkt->buf.skip, \
		.len  = pkt_len(pkt),                  \
	}

#ifndef CONFIG_PKT_NB_MAX
#define CONFIG_PKT_NB_MAX 3
#endif

#ifndef CONFIG_PKT_SIZE
#define CONFIG_PKT_SIZE 128
#endif

void pkt_mempool_shutdown(void);
int pkt_mempool_init(void);
pkt_t *pkt_get(list_t *head);
int pkt_put(list_t *head, pkt_t *pkt);

/* #define PKT_DEBUG */
#ifdef PKT_DEBUG
pkt_t *__pkt_alloc(const char *func, int line);
int __pkt_free(pkt_t *pkt, const char *func, int line);
#define pkt_alloc() __pkt_alloc(__func__, __LINE__)
#define pkt_free(pkt) __pkt_free(pkt, __func__, __LINE__)
#else
pkt_t *pkt_alloc(void);
int pkt_free(pkt_t *pkt);
#endif

/* the emergency pkt should only be used for sending to avoid a race
 * on packet allocation if all packets are used.
 */
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
pkt_t *pkt_alloc_emergency(void);
int pkt_is_emergency(pkt_t *pkt);
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
