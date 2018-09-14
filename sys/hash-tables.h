#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>
#include "list.h"
#include "buf.h"

typedef struct node {
	sbuf_t key;
	sbuf_t val;
	list_t list;
} node_t;

typedef struct hash_table {
	int size;
	int len;
	list_t *list_head;
} hash_table_t;

static inline void htable_init(hash_table_t *htable)
{
	int i;

	if (htable->size < 1 || !POWEROF2(htable->size))
		__abort();
	for (i = 0; i < htable->size; i++)
		INIT_LIST_HEAD(&htable->list_head[i]);
}

/* htable size must be a power of 2 */
#define HTABLE_DECL(name, htable_size)			\
	list_t name##__htable_list[htable_size];	\
	hash_table_t name = {				\
		.size = htable_size,			\
		.len = 0,				\
		.list_head = name##__htable_list,	\
	}

int
htable_lookup(const hash_table_t *htable, const sbuf_t *key, sbuf_t **val);
int htable_add(hash_table_t *htable, const sbuf_t *key, sbuf_t *val);
int __htable_add(hash_table_t *htable, const void *key, int key_len,
		 const void *val, int val_len);
int htable_del(hash_table_t *htable, const sbuf_t *key);
void htable_del_val(hash_table_t *htable, sbuf_t *val);
void htable_free(hash_table_t *htable);
void htable_for_each(hash_table_t *htable,
		     int (*cb)(sbuf_t *key, sbuf_t *val, void **arg),
		     void **arg);
#endif
