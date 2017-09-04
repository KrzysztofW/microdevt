#ifndef _RING_H_
#define _RING_H_

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

typedef struct byte {
	unsigned char c;
	int pos;
} byte_t;

typedef struct ring {
	int prod_head;
	int cons_head;
	int prod_tail;
	int cons_tail;
	byte_t byte;
	int mask;
	unsigned char data[];
} ring_t;

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
	ring->prod_tail = ring->cons_tail = ring->prod_head = ring->cons_head;
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
	if (((ring->prod_head + 1) & ring->mask) == ring->prod_tail)
		return 1;
	return 0;
}

static inline int ring_is_empty(const ring_t *ring)
{
	return ring->cons_tail == ring->cons_head;
}

static inline int ring_is_empty2(const ring_t *ring)
{
	return ring->cons_tail == ring->cons_head && ring->byte.pos == 0;
}

static inline int ring_len(const ring_t *ring)
{
	return ring->mask - ((ring->mask + ring->cons_tail
			      - ring->cons_head) & ring->mask);
}

static inline int ring_free_entries(const ring_t *ring)
{
	return ring->mask - ring_len(ring);
}

static inline void __ring_addc(ring_t *ring, unsigned char c)
{
	ring->data[ring->prod_head] = c;
	ring->prod_head = (ring->prod_head + 1) & ring->mask;
}

static inline int ring_addc(ring_t *ring, unsigned char c)
{
	if (ring_is_full(ring))
		return -1;
	__ring_addc(ring, c);
	return 0;
}

static inline int ring_add(ring_t *ring, unsigned char *data, int len)
{
	int i;

	if (ring->mask - ring_len(ring) <= 0) {
		return -1;
	}
	for (i = 0; i < len; i++) {
		__ring_addc(ring, data[0]);
		data++;
	}
	return 0;
}

static inline void ring_prod_finish(ring_t *ring)
{
	ring->cons_head = ring->prod_head;
}

static inline int ring_addc_finish(ring_t *ring, unsigned char data)
{
	int ret;

	if (ring_is_full(ring))
		return -1;

	ret = ring_addc(ring, data);
	if (ret == 0)
		ring_prod_finish(ring);
	return ret;
}

static inline int ring_getc(ring_t *ring, unsigned char *c)
{
	if (ring_is_empty(ring)) {
		return -1;
	}
	*c = (unsigned char)ring->data[ring->cons_tail];
	ring->cons_tail = (ring->cons_tail + 1) & ring->mask;
	return 0;
}

static inline void ring_cons_finish(ring_t *ring)
{
	ring->prod_tail = ring->cons_tail;
}

static inline int ring_getc_finish(ring_t *ring, unsigned char *data)
{
	int ret = ring_getc(ring, data);

	if (ret == 0)
		ring_cons_finish(ring);
	return ret;
}

static inline void ring_skip(ring_t *ring, int len)
{
	int rlen = ring_len(ring);

	if (rlen == 0)
		return;

	if (rlen < len)
		len = rlen;
	ring->cons_tail = (ring->cons_tail + len) & ring->mask;
}

static inline int ring_prod_len(const ring_t *ring)
{
	return ring->mask - ((ring->mask + ring->cons_head
			      - ring->prod_head) & ring->mask);
}

static inline void ring_prod_reset(ring_t *ring)
{
	ring->prod_head = ring->cons_head;
}

static inline void ring_cons_reset(ring_t *ring)
{
	ring->cons_tail = ring->prod_tail;
}

#ifdef DEBUG
static inline void print_byte(const byte_t *byte)
{
	int j;

	for (j = 0; j < byte->pos; j++) {
		printf("%s", ((byte->c  >> (7 - j)) & 1) ? "X" : " ");
	}
}

static inline void ring_print(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->cons_tail + i) & ring->mask;

		printf("0x%X ", ring->data[pos]);
	}
	puts("");
}

static inline void ring_print_bits(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->cons_tail + i) & ring->mask;
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
		int pos = (ring->cons_tail + i) & ring->mask;

		if (ring->data[pos] != str[i])
			return -1;
	}
	return 0;
}
#endif
