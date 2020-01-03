/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2017, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
*/

#include "pkt-mempool.h"
#include "event.h"

STATIC_RING_DECL(pkt_pool, CONFIG_PKT_NB_MAX);
static uint8_t buffer_data[CONFIG_PKT_NB_MAX * CONFIG_PKT_SIZE];
static pkt_t buffer_pool[CONFIG_PKT_NB_MAX];

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
static pkt_t emergency_pkt;
uint8_t emergency_pkt_data[CONFIG_PKT_SIZE];

int pkt_is_emergency(pkt_t *pkt)
{
	return pkt == &emergency_pkt;
}
#endif

unsigned int pkt_pool_get_nb_free(void)
{
	return ring_len(pkt_pool);
}

#if defined(PKT_TRACE) || defined(PKT_DEBUG)
void pkt_get_traced_pkts(void)
{
	int i;

	DEBUG_LOG("\nLast used functions:\n");
	for (i = 0; i < CONFIG_PKT_NB_MAX - 1; i++) {
		pkt_t *pkt = &buffer_pool[i];

		DEBUG_LOG("[%d] pkt:%p get:%s put:%s\n",
			  i, pkt, pkt->last_get_func,
			  pkt->last_put_func);
	}
	DEBUG_LOG("\n");
}

pkt_t *__pkt_get(ring_t *ring, const char *func, int line)
{
	uint8_t offset;
	int ret = ring_getc(ring, &offset);
	pkt_t *pkt;

	if (ret < 0) {
#ifdef PKT_DEBUG
		DEBUG_LOG("%s() in %s:%d failed\n", __func__, func, line);
#endif
		return NULL;
	}
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (offset == (typeof(offset))-1) {
		DEBUG_LOG("%s() in %s:%d (pkt:%p emergency pkt) failed\n",
			  __func__, func, line, &emergency_pkt);
		return &emergency_pkt;
	}
#endif
	pkt = &buffer_pool[offset];
#ifdef PKT_DEBUG
	DEBUG_LOG("%s() in %s:%d (pkt:%p)\n", __func__, func, line, pkt);
#endif
	pkt->last_get_func = func;
	return pkt;
}

int __pkt_put(ring_t *ring, pkt_t *pkt, const char *func, int line)
{
	int ret = ring_addc(ring, pkt->offset);
#ifdef PKT_DEBUG
	DEBUG_LOG("%s() in %s:%d (pkt:%p) %s\n", __func__, func, line, pkt,
		  ret < 0 ? "failed" : "");
#endif
	pkt->last_put_func = func;
	return ret;
}

pkt_t *__pkt_alloc(const char *func, int line)
{
	pkt_t *pkt = pkt_get(pkt_pool);

	if (pkt == NULL)
		return NULL;
	/* detect double free */
	assert(pkt->refcnt == 0);

	pkt->refcnt++;
#ifdef PKT_DEBUG
	DEBUG_LOG("%s() in %s:%d (pkt:%p)\n", __func__, func, line, pkt);
#endif
	return pkt;
}


void __pkt_free(pkt_t *pkt, const char *func, int line)
{
	/* detect double free */
	assert(pkt->refcnt);

	pkt->refcnt--;
	if (pkt->refcnt)
		return;

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (pkt_is_emergency(pkt))
		return;
#endif
#ifdef PKT_DEBUG
	DEBUG_LOG("%s() in %s:%d (pkt:%p)\n", __func__, func, line, pkt);
#endif
	buf_reset(&pkt->buf);
	if (pkt_put(pkt_pool, pkt) < 0)
		__abort();
#ifdef CONFIG_EVENT
	event_resume_write_events();
#endif
}
#else
pkt_t *pkt_get(ring_t *ring)
{
	uint8_t offset;
	int ret = ring_getc(ring, &offset);

	if (ret < 0)
		return NULL;
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (offset == (typeof(offset))-1)
		return &emergency_pkt;
#endif
	return &buffer_pool[offset];
}

int pkt_put(ring_t *ring, pkt_t *pkt)
{
	return ring_addc(ring, pkt->offset);
}

pkt_t *pkt_alloc(void)
{
#ifdef DEBUG
	pkt_t *pkt = pkt_get(pkt_pool);

	if (pkt == NULL)
		return NULL;
	/* detect double free */
	assert(pkt->refcnt == 0);

	pkt->refcnt++;
	return pkt;
#else
	return pkt_get(pkt_pool);
#endif
}

void pkt_free(pkt_t *pkt)
{
#ifdef DEBUG
	/* detect double free */
	assert(pkt->refcnt);

	pkt->refcnt--;
	if (pkt->refcnt)
		return;
#else
	if (pkt->refcnt) {
		pkt->refcnt--;
		return;
	}
#endif

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	if (pkt_is_emergency(pkt))
		return;
#endif
	buf_reset(&pkt->buf);
	if (pkt_put(pkt_pool, pkt) < 0)
		__abort();
#ifdef CONFIG_EVENT
	event_resume_write_events();
#endif
}

#endif

static void pkt_init_pkt(pkt_t *pkt, uint8_t *data)
{
	pkt->buf = BUF_INIT(data, CONFIG_PKT_SIZE);
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
	assert(emergency_pkt.refcnt == 0);
	emergency_pkt.refcnt++;
	return &emergency_pkt;
}
#endif

void pkt_mempool_init(void)
{
	unsigned i;

	for (i = 0; i < CONFIG_PKT_NB_MAX - 1; i++) {
		pkt_t *pkt = &buffer_pool[i];

		assert(i < (typeof(pkt->offset))(-1));
		pkt_init_pkt(pkt, &buffer_data[i * CONFIG_PKT_SIZE]);
		pkt->offset = i;
		ring_addc(pkt_pool, i);
	}
#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
	pkt_init_pkt(&emergency_pkt, (uint8_t *)&emergency_pkt + sizeof(pkt_t));
	emergency_pkt.offset = -1;
#endif
}

#ifdef TEST
void pkt_mempool_shutdown(void)
{
	ring_reset(pkt_pool);
}
#endif
