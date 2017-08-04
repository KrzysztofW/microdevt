#ifndef _BUF_H_
#define _BUF_H_
#include <assert.h>
#include "../ring.h"

typedef struct buf {
	int len;
	int size;
	int skip;
	unsigned char *data;
} buf_t;

#define btod(buf, type) (type)((buf)->data + (buf)->skip)

static inline void buf_init(buf_t *buf, unsigned char *data, int len)
{
	buf->size = buf->len = len;
	buf->skip = 0;
	buf->data = data;
}

static inline int buf_len(const buf_t *buf)
{
	return buf->len;
}

static inline int buf_cmp(const buf_t *buf1, const buf_t *buf2)
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
	buf->data -= buf->skip;
}

static inline int buf_has_room(buf_t *buf, int len)
{
	if (buf->len + buf->skip + len >= buf->size) {
		return -1;
	}
	return 0;
}

static inline void buf_adj(buf_t *buf, int len)
{
#ifdef DEBUG
	if (len < 0) {
		assert(buf->skip >= -len);
	} else {
		assert(buf->skip + len <= buf->size);
	}
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
	if (buf_has_room(buf, len) < 0) {
		return -1;
	}
	memcpy(buf->data + buf->len + buf->skip, data, len);
	buf->len += len;
	return 0;
}

static inline int buf_addc(buf_t *buf, uint8_t c)
{
	return buf_add(buf, &c, 1);
}

static inline int buf_addbuf(buf_t *dst, const buf_t *src)
{
	return buf_add(dst, src->data + src->skip, src->len - src->skip);
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

static inline void buf_print(buf_t buf)
{
	int i;

	for (i = 0; i < buf.len; i++) {
		printf(" 0x%02X", buf.data[i]);
	}
	puts("");
}

#endif
