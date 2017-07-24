#ifndef _BUF_H_
#define _BUF_H_

#include "../ring.h"

typedef struct buf {
	unsigned int len;
	unsigned int size;
	unsigned int skip;
	unsigned char *data;
} buf_t;

#define btod(buf, type) (type)(buf)->data

static inline int buf_alloc(buf_t *buf, int len)
{
	memset(buf, 0, len);
	buf->data = malloc(len);
	if (buf->data == NULL)
		return -1;
	buf->size = len;
	return 0;
}

static inline void buf_free(buf_t *buf)
{
	if (buf->data && buf->size)
		free(buf->data);
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
	if (buf->len + len >= buf->size) {
		buf->len = 0;
		return;
	}
	buf->data += len;
	buf->len -= len;
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
	fread(buf->data, buf->size, 1, fp);
	buf->len = size;
	fclose(fp);
	return 0;
}
#endif

static inline void buf_print(buf_t buf)
{
	unsigned i;
	for (i = 0; i < buf.len; i++) {
		printf(" 0x%X", buf.data[i]);
	}
	puts("");
}

#endif
