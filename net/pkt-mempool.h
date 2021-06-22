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

#ifndef _BUF_MEMPOOL_H_
#define _BUF_MEMPOOL_H_
#include <stdint.h>
#include <stdio.h>
#include "../sys/buf.h"
#include "../sys/ring.h"
#include "../sys/list.h"

/* #define PKT_TRACE */

typedef struct pkt {
	buf_t buf;
	list_t list;
	uint8_t offset;
	uint8_t refcnt;
#if defined(PKT_TRACE) || defined(PKT_DEBUG)
	const char *last_get_func;
	const char *last_put_func;
#endif
} pkt_t;

#define btod(pkt) (void *)((pkt)->buf.data)
#define pkt_adj(pkt, len) buf_adj(&(pkt)->buf, len)

#define PKT2SBUF(pkt) (sbuf_t)			       \
	{					       \
		.data = pkt->buf.data,                 \
		.len  = pkt_len(pkt),                  \
	}

#ifndef CONFIG_PKT_NB_MAX
#define CONFIG_PKT_NB_MAX 3
#endif

#ifndef CONFIG_PKT_SIZE
#define CONFIG_PKT_SIZE 128
#endif

/** Initialize packet memory pool
 */
void pkt_mempool_init(void);

/** Destroy packet memory pool
 */
void pkt_mempool_shutdown(void);

#if defined(PKT_TRACE) || defined(PKT_DEBUG)
#ifndef DEBUG
#error "DEBUG must be defined"
#endif
pkt_t *__pkt_get(ring_t *ring, const char *func, int line);
int __pkt_put(ring_t *ring, pkt_t *pkt, const char *func, int line);
#define pkt_get(ring) __pkt_get(ring, __func__, __LINE__)
#define pkt_put(ring, pkt) __pkt_put(ring, pkt, __func__, __LINE__)

pkt_t *__pkt_alloc(const char *func, int line);
void __pkt_free(pkt_t *pkt, const char *func, int line);
#define pkt_alloc() __pkt_alloc(__func__, __LINE__)
#define pkt_free(pkt) __pkt_free(pkt, __func__, __LINE__)
#else

/** Get a packet from pool
 *
 * @param[in] ring packet pool
 * @return packet or NULL if no packets in pool
 */
pkt_t *pkt_get(ring_t *ring);

/** Put a packet to pool
 *
 * @param[in] ring  packet pool
 * @param[in] pkt   packet
 * @return 0 on success, -1 if no more room in pool
 */
int pkt_put(ring_t *ring, pkt_t *pkt);

/** Allocate a packet
 *
 * Note: Allocs and frees can only be called from a task scheduler
 * @return new packet or NULL if no more packets
 */
pkt_t *pkt_alloc(void);

/** Free a packet
 *
 * Note: Allocs and frees can only be called from a task scheduler
 * @param[in] pkt  packet to free
 */
void pkt_free(pkt_t *pkt);

#endif

#ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
/** Allocate an emergency packet
 *
 * The emergency packet should only be used for sending to avoid a race
 * on packet allocation if all packets are used.
 * @return packet
 */
pkt_t *pkt_alloc_emergency(void);

/** Check if a packet is an emergency packet
 *
 * @param[in] pkt  packet
 * @return 1 is packet is an emergency packet, 0 otherwise
 */
int pkt_is_emergency(pkt_t *pkt);

#endif

/** Get packet length
 *
 * @param[in] pkt  packet
 * @return packet length
 */
static inline int pkt_len(const pkt_t *pkt)
{
	return pkt->buf.len;
}

/** Retain packet
 *
 * Increment packet reference counter
 * @param[in] pkt  packet
 */
static inline void pkt_retain(pkt_t *pkt)
{
	pkt->refcnt++;
}

/** Get number of available packets
 *
 * @return number of available packets
 */
unsigned int pkt_pool_get_nb_free(void);

/** Get last used functions of packets in pool (for debugging)
 */
void pkt_get_traced_pkts(void);

#endif
