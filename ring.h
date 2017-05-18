#ifndef _RING_H_
#define _RING_H_

#define MAX_SIZE 64
#define MASK (MAX_SIZE - 1)

typedef struct ring {
	int head;
	int tail;
	unsigned char data[MAX_SIZE];
} ring_t;

typedef struct byte {
	unsigned char c;
	int pos;
} byte_t;

/*
      0 1 2 3
head    |
tail  |
size = 3

*/

static inline int ring_is_full(const ring_t *ring)
{
	if (((MASK + ring->tail - ring->head) & MASK) == 0)
		return 1;
	return 0;
}

static inline int ring_is_empty(const ring_t *ring)
{
	return ring->tail == ring->head;
}

static inline int ring_len(const ring_t *ring)
{
	return MASK - ((MASK + ring->tail - ring->head) & MASK);
}

static inline void ring_reset(ring_t *ring)
{
	ring->tail = ring->head;
}

static inline int ring_addc(ring_t *ring, unsigned char c)
{
	if (ring_is_full(ring))
		return -1;
	ring->data[ring->head] = c;
	ring->head = (ring->head + 1) & MASK;
	return 0;
}

static inline int ring_getc(ring_t *ring, unsigned char *c)
{
	if (ring_is_empty(ring)) {
		return -1;
	}
	*c = (unsigned char)ring->data[ring->tail];
	ring->tail = (ring->tail + 1) & MASK;
	return 0;
}

/* static inline void ring_skip(ring_t *ring, int skip) */
/* { */

/* } */

static inline void ring_print(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;
	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->tail + i) & MASK;
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
		int pos = (ring->tail + i) & MASK;
		unsigned char byte = ring->data[pos];
		int j;

		for (j = 0; j < 8; j++) {
			printf("%s", ((byte >> (7 - j)) & 1) ? "X" : " ");
		}
	}
	puts("");
}

static inline void ring_add_bit(ring_t *ring, byte_t *byte, int bit)
{
	byte->c = (byte->c << 1) | bit;
	byte->pos++;

	if (byte->pos >= 8) {
		byte->pos = 0;
		ring_addc(ring, byte->c);
		byte->c = 0;
		byte->pos = 0;
	}
}

static inline void reset_byte(byte_t *byte)
{
	byte->c = 0;
	byte->pos = 0;
}

#endif
