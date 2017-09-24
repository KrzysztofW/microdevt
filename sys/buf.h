#ifndef _BUF_H_
#define _BUF_H_
#include <assert.h>
#include <string.h>
#include "ring.h"
#include "utils.h"

struct static_buf {
	int len;
	const unsigned char *data;
} __attribute__((__packed__));
typedef struct static_buf sbuf_t;

struct buf {
	int len;
	int size;
	int skip;
	unsigned char *data;
} __attribute__((__packed__));
typedef struct buf buf_t;

#define buf2staticbuf(buf) (sbuf_t) { .len = buf->len, .data = buf->data }

#define SBUF_INITS(str) (sbuf_t)					\
	{								\
		.data = (unsigned char *)str, .len = strlen(str)	\
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
	sbuf_t sbuf1 = buf2staticbuf(buf1);
	sbuf_t sbuf2 = buf2staticbuf(buf2);

	return sbuf_cmp(&sbuf1, &sbuf2);
}

static inline int buf_alloc(buf_t *buf, int len)
{
	buf->data = malloc(len);
	if (buf->data == NULL)
		return -1;
	buf->size = len;
	buf->len = buf->skip = 0;
	return 0;
}

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

static inline int buf_has_room(buf_t *buf, int len)
{
	if (buf->len + buf->skip + len >= buf->size)
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

static inline int buf_add(buf_t *buf, const uint8_t *data, int len)
{
	if (buf_has_room(buf, len) < 0)
		return -1;

	memcpy(buf->data + buf->len + buf->skip, data, len);
	buf->len += len;
	return 0;
}

static inline int buf_addc(buf_t *buf, uint8_t c)
{
	return buf_add(buf, &c, 1);
}

static inline int buf_get_lastc(buf_t *buf, uint8_t *c)
{
	if (buf_len(buf) < 1)
		return -1;
	*c = buf->data[buf->skip + buf->len - 1];
	buf->len--;
	return 0;
}

static inline int buf_addbuf(buf_t *dst, const buf_t *src)
{
	return buf_add(dst, src->data + src->skip, src->len - src->skip);
}

static inline int buf_addsbuf(buf_t *dst, const sbuf_t *src)
{
	return buf_add(dst, src->data, src->len);
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

static inline void sbuf_print(const sbuf_t *buf)
{
#if defined(TEST) || defined(DEBUG)
	int i;

	for (i = 0; i < buf->len; i++) {
		printf(" 0x%02X", buf->data[i]);
	}
	puts("");
#endif
}

static inline void buf_print(const buf_t *buf)
{
	sbuf_t sb;
	sbuf_init(&sb, buf->data + buf->skip, buf->len);
	sbuf_print(&sb);
}

#endif
