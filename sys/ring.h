#ifndef _RING_H_
#define _RING_H_

#include <sys/chksum.h>
#include "buf.h"
#include "utils.h"

/* Single producer - single consumer circular buffer ring implementation.
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
} __attribute__((__packed__));
typedef struct ring ring_t;
#undef TYPE

static inline void ring_reset(ring_t *ring)
{
	ring->tail = ring->head;
}

static inline void ring_init(ring_t *ring, int size)
{
	ring->mask = size - 1;
	ring_reset(ring);
}

static inline ring_t *ring_create(int size)
{
	ring_t *ring;

#ifdef CONFIG_AVR_MCU
	if (size > 256)
		__abort();
#endif
	if (!POWEROF2(size))
		__abort();

	if ((ring = malloc(sizeof(ring_t) + size)) == NULL)
		__abort();

	ring->head = 0;
	ring_init(ring, size);
	return ring;
}

static inline void ring_free(ring_t *ring)
{
	if (ring)
		free(ring);
}

static inline int ring_is_full(const ring_t *ring)
{
	if (((ring->head + 1) & ring->mask) == ring->tail)
		return 1;
	return 0;
}

static inline int ring_is_empty(const ring_t *ring)
{
	return ring->tail == ring->head;
}

static inline int ring_len(const ring_t *ring)
{
	return ring->mask - ((ring->mask + ring->tail
			      - ring->head) & ring->mask);
}

static inline int ring_free_entries(const ring_t *ring)
{
	return ring->mask - ring_len(ring);
}

static inline void __ring_addc(ring_t *ring, uint8_t c)
{
	ring->data[ring->head] = c;
	ring->head = (ring->head + 1) & ring->mask;
}

static inline int ring_addc(ring_t *ring, uint8_t c)
{
	if (ring_is_full(ring))
		return -1;
	__ring_addc(ring, c);
	return 0;
}

static inline void __ring_addbuf(ring_t *ring, const buf_t *buf)
{
	int i;
	uint8_t *d = buf->data;

	for (i = 0; i < buf->len; i++)
		__ring_addc(ring, d[i]);
}

static inline int ring_addbuf(ring_t *ring, const buf_t *buf)
{
	if (buf->len > ring_free_entries(ring))
		return -1;
	__ring_addbuf(ring, buf);
	return 0;
}

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

static inline void __ring_getc_at(ring_t *ring, uint8_t *c, int pos)
{
	assert(pos < ring->mask);
	pos = (ring->tail + pos ) & ring->mask;
	*c = ring->data[pos];
}

static inline void __ring_getc(ring_t *ring, uint8_t *c)
{
	*c = ring->data[ring->tail];
	ring->tail = (ring->tail + 1) & ring->mask;
}

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

static inline int ring_getc(ring_t *ring, uint8_t *c)
{
	if (ring_is_empty(ring))
		return -1;
	__ring_getc(ring, c);
	return 0;
}

static inline int ring_get_last_byte(ring_t *ring, uint8_t *c)
{
	int pos;

	if (ring_is_empty(ring))
		return -1;

	pos = (ring->head - 1) & ring->mask;
	*c = ring->data[pos];
	return 0;
}

static inline void __ring_skip(ring_t *ring, int len)
{
	ring->tail = (ring->tail + len) & ring->mask;
}

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

static inline int __ring_get(ring_t *ring, buf_t *buf, int max_len)
{
	int l = __ring_get_dont_skip(ring, buf, max_len);

	__ring_skip(ring, l);
	return l;
}

static inline void __ring_get_buf(ring_t *ring, buf_t *buf)
{
	int i;

	for (i = 0; i < buf->size; i++) {
		int pos = (ring->tail + i) & ring->mask;

		__buf_addc(buf, ring->data[pos]);
	}
	__ring_skip(ring, buf->len);
}

static inline int ring_get(ring_t *ring, buf_t *buf)
{
	int rlen = ring_len(ring);

	return __ring_get(ring, buf, rlen);
}

static inline void ring_skip(ring_t *ring, int len)
{
	int rlen = ring_len(ring);

	if (rlen == 0)
		return;

	if (rlen < len)
		len = rlen;
	__ring_skip(ring, len);
}

#ifdef DEBUG
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

static inline void ring_print(const ring_t *ring)
{
	ring_print_limit(ring, 0, 0);
}

static inline void ring_print_hex(const ring_t *ring)
{
	ring_print_limit(ring, 0, 1);
}

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
#else
#define ring_print(x)
#define ring_print_bits(x)
#endif

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

static inline int ring_sbuf_cmp(const ring_t *ring, const sbuf_t *sbuf)
{
	return ring_cmp(ring, sbuf->data, sbuf->len);
}

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
