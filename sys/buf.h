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

#ifndef _BUF_H_
#define _BUF_H_
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <alloca.h>
#include <stdarg.h>
#include <ctype.h>
#include "log.h"
#include "utils.h"

/** Static buffer
 */
struct static_buf {
	int len;
	const uint8_t *data;
} __PACKED__;
typedef struct static_buf sbuf_t;

/** Buffer
 */
struct buf {
	int len;
	int size;
	int skip;
	uint8_t *data;
} __PACKED__;
typedef struct buf buf_t;

#define buf2sbuf(buf) (sbuf_t)					\
	{							\
		.len = (buf)->len,				\
		.data = (buf)->data				\
	}
#define sbuf2buf(sbuf) (buf_t) { .len = (sbuf)->len,	\
			.data = (void *)(sbuf)->data,	\
			.size = (sbuf)->len }

#define SBUF_INITS(str) (sbuf_t)					\
	{								\
		.data = (uint8_t *)str, .len = sizeof(str) - 1		\
	}
#define SBUF_INIT_BIN(__data) (sbuf_t)					\
	{								\
		.data = (uint8_t *)__data, .len = sizeof(__data)	\
	}
#define SBUF_INIT(__data, __len) (sbuf_t)				\
	{								\
		.data = (uint8_t *)__data, .len = __len			\
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

static inline void buf_free(buf_t *buf)
{
	if (buf->data && buf->size)
		free(buf->data);
}
#endif

static inline void buf_reset(buf_t *buf)
{
	buf->len = 0;
	buf->data -= buf->skip;
	buf->skip = 0;
}

static inline void __buf_reset_keep(buf_t *buf)
{
	buf->len += buf->skip;
	buf->data -= buf->skip;
	buf->skip = 0;
}

static inline void __buf_shrink(buf_t *buf, int len)
{
	buf->len -= len;
}

static inline void buf_shrink(buf_t *buf, int len)
{
	if (buf->len < len)
		return;
	__buf_shrink(buf, len);
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
	if (buf->len < len)
		buf->len = 0;
	else
		buf->len -= len;
	buf->skip += len;
	buf->data += len;
}

static inline void __buf_add(buf_t *buf, const void *data, int len)
{
	memcpy(buf->data + buf->len, data, len);
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

static inline int buf_addf(buf_t *buf, const char *fmt, ...)
{
	va_list ap;
	int len;
	int buf_room = buf->size - buf->skip - buf->len;

	va_start(ap, fmt);
	len = vsnprintf((char *)buf->data, buf_room, fmt, ap);
	va_end(ap);
	if (len < 0 || len >= buf_room)
		return -1;
	buf->len += len + 1;
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

static inline void __buf_addsbuf(buf_t *buf, const sbuf_t *sbuf)
{
	__buf_add(buf, sbuf->data, sbuf->len);
}

static inline int buf_get_lastc(buf_t *buf, uint8_t *c)
{
	if (buf->len < 1)
		return -1;
	*c = buf->data[buf->len - 1];
	buf->len--;
	return 0;
}

static inline void __buf_getc(buf_t *buf, uint8_t *c)
{
	*c = buf->data[0];
	buf->len--;
	buf->skip++;
	buf->data++;
}

static inline int buf_getc(buf_t *buf, uint8_t *c)
{
	if (buf->len < 1)
		return -1;
	__buf_getc(buf, c);
	return 0;
}

static inline void __buf_skip(buf_t *buf, int len)
{
	buf->skip += len;
	buf->data += len;
	buf->len -= len;
}

static inline int buf_skip(buf_t *buf, int len)
{
	if (buf->len < len)
		return -1;
	__buf_skip(buf, len);
	return 0;
}

static inline void __buf_get_u16(buf_t *buf, uint16_t *val)
{
	*val = *(uint16_t *)buf->data;
	__buf_skip(buf, sizeof(uint16_t));
}

static inline int buf_get_u16(buf_t *buf, uint16_t *val)
{
	if (buf->len < (int)sizeof(uint16_t))
		return -1;
	__buf_get_u16(buf, val);
	return 0;
}

static inline void __buf_get(buf_t *buf, void *data, int len)
{
	memcpy(data, buf->data, len);
	__buf_skip(buf, len);
}

static inline int buf_get(buf_t *buf, void *data, int len)
{
	if (buf->len < len)
		return -1;
	__buf_get(buf, data, len);
	return 0;
}

static inline void buf_skip_spaces(buf_t *buf)
{
	while (buf->len && isspace(buf->data[0]))
	       __buf_skip(buf, 1);
}

static inline void *__memmem(const void *haystack, unsigned haystacklen,
			     const void *needle, unsigned needlelen)
{
	unsigned i, j;
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

static inline void buf_print(const buf_t *buf);
static inline void sbuf_print(const sbuf_t *buf);

static inline int
__buf_get_sbuf_upto_sbuf(buf_t *buf, sbuf_t *sbuf, const sbuf_t *s,
			 uint8_t skip)
{
	uint8_t *d;
	unsigned skipped_len;

	d = __memmem(buf->data, buf->len, s->data, s->len);
	if (d == NULL)
		return -1;
	skipped_len = d - buf->data;
	sbuf_init(sbuf, buf->data, skipped_len);
	if (skip)
		buf_adj(buf, skipped_len + s->len);

	return 0;
}

static inline int
buf_get_sbuf_upto_sbuf_and_skip(buf_t *buf, sbuf_t *sbuf, const sbuf_t *s)
{
	return __buf_get_sbuf_upto_sbuf(buf, sbuf, s, 1);
}

static inline int
buf_get_sbuf_upto_sbuf(buf_t *buf, sbuf_t *sbuf, const sbuf_t *s)
{
	return __buf_get_sbuf_upto_sbuf(buf, sbuf, s, 0);
}

static inline int buf_get_sbuf_upto(buf_t *buf, sbuf_t *sbuf, const char *s)
{
	sbuf_t sbuf2;

	sbuf_init(&sbuf2, s, strlen(s));
	return buf_get_sbuf_upto_sbuf(buf, sbuf, &sbuf2);
}

/* This function has un undefined behavior if buf->data is not null
 * terminated.
 */
static inline int buf_get_long(buf_t *buf, long *i)
{
	char *endptr;
	int len;

	if (buf->len == 0)
		return -1;
	errno = 0;
	*i = strtol((char *)buf->data, &endptr, 10);
	if (errno != 0 || endptr == (char *)buf->data)
		return -1;
	len = endptr - (char *)buf->data;
	if (len > buf->len)
		return -1;
	__buf_skip(buf, len);
	return 0;
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
	return buf_add(dst, src->data, src->len);
}

static inline void __buf_addbuf(buf_t *dst, const buf_t *src)
{
	__buf_add(dst, src->data, src->len);
}

static inline int buf_addsbuf(buf_t *dst, const sbuf_t *src)
{
	return buf_add(dst, src->data, src->len);
}

static inline int buf_pad(buf_t *buf, uint8_t order)
{
	uint8_t pad = 1 << order;
	uint8_t mask = pad - 1;

	while (buf->len & mask) {
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

static inline void __sbuf_print(const sbuf_t *buf, uint8_t hex)
{
	int i;
	const char *fmt = hex ? "0x%02X " : "%c";

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

	sbuf_init(&sb, buf->data, buf->len);
	sbuf_print(&sb);
}

static inline void buf_print_hex(const buf_t *buf)
{
	sbuf_t sb;

	sbuf_init(&sb, buf->data, buf->len);
	sbuf_print_hex(&sb);
}

#endif
