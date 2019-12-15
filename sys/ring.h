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

#ifndef _RING_H_
#define _RING_H_

#include "chksum.h"
#include "buf.h"
#include "utils.h"

/** Single producer - single consumer circular buffer ring implementation.
 * Note: multi prod/cons rings on AVR architecture are not possible as
 * atomic 'compare & set' is not provided.
 */

#ifdef CONFIG_AVR_MCU
/* only arithmetics on uint8_t are atomic */
#define TYPE uint8_t
#else
#define TYPE unsigned int
#endif

struct ring {
	volatile TYPE head;
	volatile TYPE tail;
	TYPE mask;
	uint8_t data[];
} __PACKED__;
typedef struct ring ring_t;
#undef TYPE

/** Ring declaration
 * The following ring declaration can be used for declaring a ring
 * as a global variable in a C file.
 * The size of a ring MUST be a power of 2.
 */
#define RING_DECL(name, size)			\
	struct  {				\
		ring_t ring;			\
		uint8_t ring_data[size];	\
	} ring_##name = {			\
		.ring.mask = (size) - 1,	\
	};					\
	ring_t *name = &ring_##name.ring

/** Static ring declaration
 * The following static ring declaration can be used for declaring a ring
 * as a global variable in a C file.
 * The size of a ring MUST be a power of 2.
 */
#define STATIC_RING_DECL(name, size)		\
	static struct  {			\
		ring_t ring;			\
		uint8_t ring_data[size];	\
	} ring_##name = {			\
		.ring.mask = (size) - 1,	\
	};					\
	static ring_t *name = &ring_##name.ring

/** Ring declaration in C structures
 *
 * Use this macro to declare rings in C structures
 *
 * Example of usage:
 *  struct {
 *	RING_DECL_IN_STRUCT(my_ring, 4);
 *   } a = {
 *	.my_ring = RING_INIT(a.my_ring),
 * };
 */
#define RING_DECL_IN_STRUCT(name, size)		\
	ring_t name;				\
	uint8_t name##_data[size];

/** Initialize ring at compile time
 *
 * @param[in] name ring name
 */
#define RING_INIT(name) {				\
		.mask = sizeof(name##_data) - 1,	\
	}

/** Reset ring
 *
 * @param[in] ring ring
 */
static inline void ring_reset(ring_t *ring)
{
	ring->tail = ring->head;
}

/** Initialize ring
 *
 * @param[in] ring ring
 * @param[in] size ring size
 */
static inline void ring_init(ring_t *ring, int size)
{
#ifdef CONFIG_AVR_MCU
	if (size > MAX_VALUE(ring->mask))
		__abort();
#endif
	if (!POWEROF2(size))
		__abort();
	ring->head = ring->tail = 0;
	ring->mask = size - 1;
}

/** Check if ring is full
 *
 * @param[in] ring ring
 * @return 1 if full, 0 otherwise
 */
static inline uint8_t ring_is_full(const ring_t *ring)
{
	if (((ring->head + 1) & ring->mask) == ring->tail)
		return 1;
	return 0;
}

/** Check if ring is empty
 *
 * @param[in] ring ring
 * @return 1 if empty, 0 otherwise
 */
static inline uint8_t ring_is_empty(const ring_t *ring)
{
	return ring->tail == ring->head;
}

/** Get ring length
 *
 * @param[in] ring ring
 * @return ring length
 */
static inline int ring_len(const ring_t *ring)
{
	return ring->mask - ((ring->mask + ring->tail
			      - ring->head) & ring->mask);
}

/** Get ring available entriees
 *
 * @param[in] ring ring
 * @return number of free entriess in ring
 */
static inline int ring_free_entries(const ring_t *ring)
{
	return ring->mask - ring_len(ring);
}

/** Add byte to ring without checking
 *
 * @param[in] ring  ring
 * @param[in] c     byte to add
 */
static inline void __ring_addc(ring_t *ring, uint8_t c)
{
	ring->data[ring->head] = c;
	ring->head = (ring->head + 1) & ring->mask;
}

/** Add byte to ring
 *
 * @param[in] ring ring
 * @param[in] c    byte to add
 * @return 0 on success, -1 on failure
 */
static inline int ring_addc(ring_t *ring, uint8_t c)
{
	if (ring_is_full(ring))
		return -1;
	__ring_addc(ring, c);
	return 0;
}

/** Add buffer to ring without checking
 *
 * @param[in] ring ring
 * @param[in] buf  buffer
 */
static inline void __ring_addbuf(ring_t *ring, const buf_t *buf)
{
	int i;
	uint8_t *d = buf->data;

	for (i = 0; i < buf->len; i++)
		__ring_addc(ring, d[i]);
}

/** Add buffer to ring
 *
 * @param[in] ring ring
 * @param[in] buf  buffer
 * @return 0 on success, -1 on failure
 */
static inline int ring_addbuf(ring_t *ring, const buf_t *buf)
{
	if (buf->len > ring_free_entries(ring))
		return -1;
	__ring_addbuf(ring, buf);
	return 0;
}

/** Add data to ring
 *
 * @param[in] ring ring
 * @param[in] data pointer to data to add
 * @param[in] len  data length
 * @return 0 on success, -1 on failure
 */
static inline int ring_add(ring_t *ring, const void *data, int len)
{
	int i;
	const uint8_t *d = data;

	if (len > ring_free_entries(ring))
		return -1;
	for (i = 0; i < len; i++)
		__ring_addc(ring, d[i]);
	return 0;
}

/** Get byte at position in ring
 *
 * @param[in] ring ring
 * @param[out] c   byte
 * @param[in] pos  position in ring
 */
static inline void __ring_getc_at(ring_t *ring, uint8_t *c, int pos)
{
	assert(pos < ring->mask);
	pos = (ring->tail + pos ) & ring->mask;
	*c = ring->data[pos];
}

/** Get byte from ring without checking
 *
 * @param[in] ring ring
 * @param[out] c   byte
 */
static inline void __ring_getc(ring_t *ring, uint8_t *c)
{
	*c = ring->data[ring->tail];
	ring->tail = (ring->tail + 1) & ring->mask;
}

/** Get buffer from ring without skipping
 *
 * @param[in] ring ring
 * @param[out] buf buffer
 * @param[in] len data length to get
 * @return 0 on success, -1 on failure
 */
static inline int
__ring_get_dont_skip(const ring_t *ring, buf_t *buf, int len)
{
	int i;
	int blen = buf_get_free_space(buf);
	int l = MIN(blen, len);

	for (i = 0; i < l; i++) {
		int pos = (ring->tail + i) & ring->mask;

		__buf_addc(buf, ring->data[pos]);
	}
	return l;
}

/** Get byte from ring
 *
 * @param[in] ring ring
 * @param[out] c   bytes
 * @return 0 on success, -1 on failure
 */
static inline int ring_getc(ring_t *ring, uint8_t *c)
{
	if (ring_is_empty(ring))
		return -1;
	__ring_getc(ring, c);
	return 0;
}

/** Get last byte from ring
 *
 * @param[in] ring ring
 * @param[out] c   byte
 * @return 0 on sucess, -1 on failure
 */
static inline int ring_get_last_byte(ring_t *ring, uint8_t *c)
{
	int pos;

	if (ring_is_empty(ring))
		return -1;

	pos = (ring->head - 1) & ring->mask;
	*c = ring->data[pos];
	return 0;
}

/** Skip data in ring without checking
 *
 * @param[in] ring ring
 * @param[in] len  length to skip
 */
static inline void __ring_skip(ring_t *ring, int len)
{
	ring->tail = (ring->tail + len) & ring->mask;
}

/** Skip data up to given byte
 *
 * @param[in] ring ring
 * @param[in] c    byte
 * @return 0 on success, -1 on failure
 */
static inline int ring_skip_upto(ring_t *ring, uint8_t c)
{
	int rlen = ring_len(ring);
	int i;

	for (i = 0; i < rlen; i++) {
		int pos = (ring->tail + i) & ring->mask;

		if (ring->data[pos] == c) {
			__ring_skip(ring, i + 1);
			return 0;
		}
	}
	return -1;
}

/** Get buffer from ring
 *
 * @param[in] ring  ring
 * @param[out] buf  buffer
 * @param[in]  max_len data length to get
 */
static inline int __ring_get(ring_t *ring, buf_t *buf, int max_len)
{
	int l = __ring_get_dont_skip(ring, buf, max_len);

	__ring_skip(ring, l);
	return l;
}

/** Get buffer from ring without checking
 *
 * @param[in] ring ring
 * @param[out] buf buffer
 */
static inline void __ring_get_buf(ring_t *ring, buf_t *buf)
{
	int i;

	for (i = 0; i < buf->size; i++) {
		int pos = (ring->tail + i) & ring->mask;

		__buf_addc(buf, ring->data[pos]);
	}
	__ring_skip(ring, buf->len);
}

/** Get buffer from ring
 *
 * @param[in] ring ring
 * @param[out] buf buffer
 * @return 0 on success, -1 on failure
 */
static inline int ring_get(ring_t *ring, buf_t *buf)
{
	int rlen = ring_len(ring);

	return __ring_get(ring, buf, rlen);
}

/** Skip data in ring without checking
 *
 * @param[in] ring ring
 * @param[in] len  data length to skip
 */
static inline void ring_skip(ring_t *ring, int len)
{
	int rlen = ring_len(ring);

	if (rlen == 0)
		return;

	if (rlen < len)
		len = rlen;
	__ring_skip(ring, len);
}

/** Print data limited by size
 *
 * @param[in] ring ring
 * @param[in] limit size to print
 * @param[in] hex   print in hexadecimal
 */
static inline void
ring_print_limit(const ring_t *ring, int limit, uint8_t hex)
{
	int i;
	int len;
	char *fmt = hex ? "0x%02X " : "%c";

	if (limit)
		len = MIN(limit, ring_len(ring));
	else
		len = ring_len(ring);

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < len; i++) {
		int pos = (ring->tail + i) & ring->mask;

		printf(fmt, ring->data[pos]);
	}
	if (hex)
		puts("");
}

/** Print ring content
 *
 * @param[in] ring ring
 */
static inline void ring_print(const ring_t *ring)
{
	ring_print_limit(ring, 0, 0);
}

/** Print ring content in hexadecimal
 *
 * @param[in] ring ring
 */
static inline void ring_print_hex(const ring_t *ring)
{
	ring_print_limit(ring, 0, 1);
}

/** Print bits from ring
 *
 * @param[in] ring ring
 */
static inline void ring_print_bits(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->tail + i) & ring->mask;
		uint8_t byte = ring->data[pos];
		int j;

		for (j = 0; j < 8; j++) {
			printf("%s", ((byte >> (7 - j)) & 1) ? "X" : " ");
		}
	}
	puts("\n");
}

/** Compare ring content with data in linear buffer
 *
 * @param[in] ring ring
 * @param[in] data linear buffer
 * @param[in] len  length to compare
 * @return 0 if data are identical, -1 otherwise
 */
static inline int
ring_cmp(const ring_t *ring, const uint8_t *data, int len)
{
	int i;

	if (len == 0 || ring_len(ring) < len)
		return -1;

	for (i = 0; i < len; i++) {
		int pos = (ring->tail + i) & ring->mask;

		if (ring->data[pos] != data[i])
			return -1;
	}
	return 0;
}

/** Compare ring content with data in static linear buffer
 *
 * @param[in] ring ring
 * @param[in] sbuf static buffer
 * @return 0 if data are identical, -1 otherwise
 */
static inline int ring_sbuf_cmp(const ring_t *ring, const sbuf_t *sbuf)
{
	return ring_cmp(ring, sbuf->data, sbuf->len);
}

/** Get ring content checksum
 *
 * @param[in] ring ring
 * @param[in] len  length
 * @return checksum
 */
static inline int
__ring_cksum(const ring_t *ring, int len)
{
	int i;
	uint32_t csum = 0;

	for (i = 0; i < len; i++) {
		int pos = (ring->tail + i) & ring->mask;
		uint16_t w = ring->data[pos];

		if (i + 1 < len) {
			pos = (ring->tail + i + 1) & ring->mask;
			w |= (ring->data[pos] << 8);
			i++;
		}
		csum += w;
	}
	return cksum_finish(csum);
}
#endif
