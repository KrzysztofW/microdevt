#ifndef _RING_H_
#define _RING_H_

#include <stdlib.h>
#include "utils.h"

struct byte {
	unsigned char c;
	int pos;
} __attribute__((__packed__));
typedef struct byte byte_t;

struct ring {
	int head;
	int tail;
	byte_t byte;
	int mask;
	unsigned char data[];
} __attribute__((__packed__));
typedef struct ring ring_t;

/*
      0 1 2 3
head    |
tail  |
size = 3

*/

static inline void reset_byte(byte_t *byte)
{
	byte->pos = 0;
}

static inline void ring_reset_byte(ring_t *ring)
{
	reset_byte(&ring->byte);
}

static inline void ring_reset(ring_t *ring)
{
	reset_byte(&ring->byte);
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

	if (!POWEROF2(size))
		return NULL;

	if ((ring = calloc(1, sizeof(ring_t) + size)) == NULL)
		return NULL;

	ring_init(ring, size);
	return ring;
}

static inline void ring_free(ring_t *ring)
{
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

static inline int ring_is_empty2(const ring_t *ring)
{
	return ring->tail == ring->head && ring->byte.pos == 0;
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

static inline void __ring_addc(ring_t *ring, unsigned char c)
{
	ring->data[ring->head] = c;
	ring->head = (ring->head + 1) & ring->mask;
}

static inline int ring_addc(ring_t *ring, unsigned char c)
{
	if (ring_is_full(ring))
		return -1;
	__ring_addc(ring, c);
	return 0;
}

static inline int ring_add(ring_t *ring, void *data, int len)
{
	int i;
	unsigned char *d = data;

	if (ring->mask - ring_len(ring) <= 0)
		return -1;
	for (i = 0; i < len; i++) {
		__ring_addc(ring, d[0]);
		d++;
	}
	return 0;
}

static inline void __ring_getc(ring_t *ring, unsigned char *c)
{
	*c = (unsigned char)ring->data[ring->tail];
	ring->tail = (ring->tail + 1) & ring->mask;
}

static inline int ring_getc(ring_t *ring, unsigned char *c)
{
	if (ring_is_empty(ring)) {
		return -1;
	}
	__ring_getc(ring, c);
	return 0;
}

static inline int ring_get(ring_t *ring, void *data, int len)
{
	unsigned char *c = data;

	if (ring_len(ring) < len)
		return -1;
	while (len--)
		__ring_getc(ring, c++);
	return 0;
}

static inline void ring_skip(ring_t *ring, int len)
{
	int rlen = ring_len(ring);

	if (rlen == 0)
		return;

	if (rlen < len)
		len = rlen;
	ring->tail = (ring->tail + len) & ring->mask;
}

#ifdef DEBUG
static inline void print_byte(const byte_t *byte)
{
	int j;

	for (j = 0; j < byte->pos; j++) {
		printf("%s", ((byte->c  >> (7 - j)) & 1) ? "X" : " ");
	}
}

static inline void ring_print_limit(const ring_t *ring, int limit)
{
	int i;
	int len;

	if (limit)
		len = MIN(limit, ring_len(ring));
	else
		len = ring_len(ring);

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < len; i++) {
		int pos = (ring->tail + i) & ring->mask;

		printf("0x%X ", ring->data[pos]);
	}
	puts("");
}
static inline void ring_print(const ring_t *ring)
{
	ring_print_limit(ring, 0);
}

static inline void ring_print_bits(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->tail + i) & ring->mask;
		unsigned char byte = ring->data[pos];
		int j;

		for (j = 0; j < 8; j++) {
			printf("%s", ((byte >> (7 - j)) & 1) ? "X" : " ");
		}
	}
	if (ring->byte.pos)
		printf("byte:");
	print_byte(&ring->byte);
	puts("");
}
#else
static inline void print_byte(const byte_t *byte)
{
	(void)byte;
}
static inline void ring_print(const ring_t *ring)
{
	(void)ring;
}
static inline void ring_print_bits(const ring_t *ring)
{
	(void)ring;
}
#endif

static inline int ring_add_bit(ring_t *ring, int bit)
{
	ring->byte.c = (ring->byte.c << 1) | bit;
	ring->byte.pos++;

	if (ring->byte.pos >= 8) {
		ring->byte.pos = 0;
		if (ring_is_full(ring))
			return -1;

		ring_addc(ring, ring->byte.c);
		ring->byte.c = 0;
	}
	return 0;
}

static inline int
ring_cmp(const ring_t *ring, const unsigned char *str, int len)
{
	int i;

	if (len == 0 || ring_len(ring) < len)
		return -1;

	for (i = 0; i < len; i++) {
		int pos = (ring->tail + i) & ring->mask;

		if (ring->data[pos] != str[i])
			return -1;
	}
	return 0;
}
#endif
