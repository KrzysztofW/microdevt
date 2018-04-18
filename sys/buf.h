#ifndef _BUF_H_
#define _BUF_H_
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include "log.h"
#include "utils.h"

struct static_buf {
	int len;
	const uint8_t *data;
} __attribute__((__packed__));
typedef struct static_buf sbuf_t;

struct buf {
	int len;
	int size;
	int skip;
	uint8_t *data;
} __attribute__((__packed__));
typedef struct buf buf_t;

#define buf2sbuf(buf) (sbuf_t)					\
	{							\
		.len = (buf)->len,				\
		.data = (buf)->data + (buf)->skip		\
	}
#define sbuf2buf(sbuf) (buf_t) { .len = (sbuf)->len,	\
			.data = (void *)(sbuf)->data,	\
			.size = (sbuf)->len }

#define SBUF_INITS(str) (sbuf_t)					\
	{								\
		.data = (uint8_t *)str, .len = sizeof(str) - 1		\
	}
#define BUF_INIT(__data, __len) (buf_t)					\
	{								\
		.data = (uint8_t *)__data,				\
		.size = __len						\
	}
#define SBUF_INIT_NULL (sbuf_t)						\
	{								\
		.data = NULL, .len = 0					\
	}
#define BUF(sz)						\
	(buf_t){					\
		.data = alloca(sz),			\
		.size = sz				\
	}

static inline void sbuf_init(sbuf_t *sbuf, const void *data,
			     unsigned len)
{
	sbuf->data = data;
	sbuf->len = len;
}

static inline void buf_init(buf_t *buf, void *data, unsigned len)
{
	buf->size = buf->len = len;
	buf->skip = 0;
	buf->data = data;
}

static inline int buf_len(const buf_t *buf)
{
	return buf->len;
}

static inline int buf_get_free_space(const buf_t *buf)
{
	return buf->size - buf->skip - buf->len;
}

static inline int sbuf_cmp(const sbuf_t *buf1, const sbuf_t *buf2)
{
	int i;

	if (buf1->len != buf2->len)
		return -1;
	for (i = 0; i < buf1->len; i++) {
		if (buf1->data[i] != buf2->data[i])
			return -1;
	}
	return 0;
}

static inline int buf_cmp(const buf_t *buf1, const buf_t *buf2)
{
	sbuf_t sbuf1 = buf2sbuf(buf1);
	sbuf_t sbuf2 = buf2sbuf(buf2);

	return sbuf_cmp(&sbuf1, &sbuf2);
}

#ifdef TEST
static inline int buf_alloc(buf_t *buf, int len)
{
	buf->data = malloc(len);
	if (buf->data == NULL)
		return -1;
	buf->size = len;
	buf->len = buf->skip = 0;
	return 0;
}
#endif

static inline void buf_free(buf_t *buf)
{
	if (buf->data && buf->size)
		free(buf->data);
}

static inline void buf_reset(buf_t *buf)
{
	buf->len = 0;
	buf->skip = 0;
}

static inline void __buf_reset_keep(buf_t *buf)
{
	buf->len = buf->skip;
	buf->skip = 0;
}

static inline void sbuf_reset(sbuf_t *sbuf)
{
	sbuf->len = 0;
}

static inline int buf_has_room(buf_t *buf, int len)
{
	if (buf->len + buf->skip + len > buf->size)
		return -1;
	return 0;
}

static inline void buf_adj(buf_t *buf, int len)
{
#ifdef DEBUG
	if (len < 0)
		assert(buf->skip >= -len);
	else
		assert(buf->skip + len <= buf->size);
#endif
	buf->len -= len;
	if (buf->len < 0)
		buf->len = 0;
	buf->skip += len;
}

static inline unsigned char *buf_data(const buf_t *buf)
{
	return buf->data + buf->skip;
}

static inline void __buf_add(buf_t *buf, const void *data, int len)
{
	memcpy(buf->data + buf->len + buf->skip, data, len);
	buf->len += len;
}

static inline void __buf_adds(buf_t *buf, const char *data, int len)
{
	uint8_t c = '\0';

	__buf_add(buf, (uint8_t *)data, len);
	__buf_add(buf, &c, 1);
}

static inline int buf_adds(buf_t *buf, const char *data)
{
	int len = strlen(data);

	if (buf_has_room(buf, len + 1) < 0)
		return -1;
	__buf_adds(buf, data, len);
	return 0;
}

static inline int buf_add(buf_t *buf, const void *data, int len)
{
	if (buf_has_room(buf, len) < 0)
		return -1;
	__buf_add(buf, data, len);
	return 0;
}

static inline int buf_addc(buf_t *buf, uint8_t c)
{
	return buf_add(buf, &c, 1);
}

static inline void __buf_addc(buf_t *buf, uint8_t c)
{
	__buf_add(buf, &c, 1);
}

static inline int buf_get_lastc(buf_t *buf, uint8_t *c)
{
	if (buf_len(buf) < 1)
		return -1;
	*c = buf->data[buf->skip + buf->len - 1];
	buf->len--;
	return 0;
}

static inline void __buf_getc(buf_t *buf, uint8_t *c)
{
	uint8_t *data = buf_data(buf);

	*c = data[0];
	buf->len--;
	buf->skip++;
}

static inline int buf_getc(buf_t *buf, uint8_t *c)
{
	if (buf_len(buf) < 1)
		return -1;
	__buf_getc(buf, c);
	return 0;
}

static inline void *__memmem(const void *haystack, unsigned haystacklen,
			     const void *needle, unsigned needlelen)
{
	int i, j;
	const uint8_t *h = haystack;
	const uint8_t *n = needle;

	for (i = 0; i < haystacklen; i++) {
		for (j = 0; j < needlelen; j++) {
			if (h[i + j] != n[j])
				break;
		}
		if (j == needlelen)
			return (void *)(h + i);
	}
	return NULL;
}

static inline int
__buf_get_sbuf_upto_sbuf(buf_t *buf, sbuf_t *sbuf, const sbuf_t *s, int skip)
{
	uint8_t *data = buf_data(buf);
	uint8_t *d;
	unsigned skipped_len;

	d = __memmem(data, buf->len, s->data, s->len);
	if (d == NULL)
		return -1;
	skipped_len = d - data;
	sbuf_init(sbuf, data, d - data);
	buf_adj(buf, skipped_len + skip);
	return 0;
}

static inline int
buf_get_sbuf_upto_sbuf_and_skip(buf_t *buf, sbuf_t *sbuf, const sbuf_t *s)
{
	return __buf_get_sbuf_upto_sbuf(buf, sbuf, s, s->len);
}

static inline int
buf_get_sbuf_upto_sbuf(buf_t *buf, sbuf_t *sbuf, const sbuf_t *s)
{
	return __buf_get_sbuf_upto_sbuf(buf, sbuf, s, 0);
}

static inline int
buf_get_sbuf_upto(buf_t *buf, sbuf_t *sbuf, const char *s)
{
	sbuf_t sbuf2;

	sbuf_init(&sbuf2, s, strlen(s));
	return buf_get_sbuf_upto_sbuf(buf, sbuf, &sbuf2);
}


static inline int
buf_get_sbuf_upto_and_skip(buf_t *buf, sbuf_t *sbuf, const char *s)
{
	sbuf_t sbuf2;

	sbuf_init(&sbuf2, s, strlen(s));
	return buf_get_sbuf_upto_sbuf_and_skip(buf, sbuf, &sbuf2);
}

static inline int buf_addbuf(buf_t *dst, const buf_t *src)
{
	return buf_add(dst, src->data + src->skip, src->len - src->skip);
}

static inline int buf_addsbuf(buf_t *dst, const sbuf_t *src)
{
	return buf_add(dst, src->data, src->len);
}

static inline int buf_pad(buf_t *buf, uint8_t order)
{
	uint8_t pad = 1 << order;
	uint8_t mask = pad - 1;

	while (buf_len(buf) & mask) {
		if (buf_addc(buf, 0) < 0)
			return -1;
	}
	return 0;
}

#ifdef TEST
static inline int buf_read_file(buf_t *buf, const char *filename)
{
	FILE *fp = fopen(filename,"rb");
	unsigned size;

	if (fp == NULL) {
		fprintf(stderr, "cannot open file %s (%m)", filename);
		return -1;
	}
	if (fseek(fp, 0, SEEK_END) < 0)
		return -1;
	size = ftell(fp);
	rewind(fp);
	if (buf_alloc(buf, size) < 0)
		return -1;
	if (fread(buf->data, buf->size, 1, fp) == 0)
		return -1;
	buf->len = size;
	fclose(fp);
	return 0;
}
#endif

#if defined(TEST) || defined(DEBUG)
static inline void __sbuf_print(const sbuf_t *buf, uint8_t hex)
{
	int i;
	char *fmt = hex ? "0x%02X " : "%c";

	for (i = 0; i < buf->len; i++)
		printf(fmt, buf->data[i]);
	if (hex)
		puts("");
}

static inline void sbuf_print(const sbuf_t *buf)
{
	__sbuf_print(buf, 0);
}

static inline void sbuf_print_hex(const sbuf_t *buf)
{
	__sbuf_print(buf, 1);
}

static inline void buf_print(const buf_t *buf)
{
	sbuf_t sb;

	sbuf_init(&sb, buf->data + buf->skip, buf->len);
	sbuf_print(&sb);
}

static inline void buf_print_hex(const buf_t *buf)
{
	sbuf_t sb;

	sbuf_init(&sb, buf->data + buf->skip, buf->len);
	sbuf_print_hex(&sb);
}
#else
#define sbuf_print(x)
#define buf_print(x)
#endif

#endif
