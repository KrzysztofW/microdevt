#ifndef _BYTE_H_
#define _BYTE_H_

struct byte {
	uint8_t c;
	uint8_t pos;
} __attribute__((__packed__));
typedef struct byte byte_t;

static inline void byte_reset(byte_t *byte)
{
	byte->pos = 0;
}

static inline void byte_init(byte_t *byte, uint8_t c)
{
	byte->c = c;
	byte->pos = 8;
}

static inline int byte_is_empty(const byte_t *byte)
{
	return byte->pos == 0;
}

static inline int byte_add_bit(byte_t *byte, uint8_t bit)
{
	byte->c = (byte->c << 1) | bit;
	byte->pos++;

	if (byte->pos >= 8) {
		byte_reset(byte);
		return byte->c;
	}
	return -1;
}

static inline int byte_get_bit(byte_t *byte)
{
	uint8_t bit;

	if (byte->pos == 0)
		return -1;
	bit = byte->c >> 7;
	byte->c = byte->c << 1;
	byte->pos--;
	return bit;
}

#ifdef DEBUG
static inline void print_byte(const byte_t *byte)
{
	int j;

	for (j = 0; j < byte->pos; j++) {
		printf("%s", ((byte->c  >> (7 - j)) & 1) ? "X" : " ");
	}
}

#else
static inline void print_byte(const byte_t *byte)
{
	(void)byte;
}
#endif

#endif
