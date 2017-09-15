#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>
#include "list.h"
#include "buf.h"

typedef struct node {
	sbuf_t key;
	sbuf_t val;
	struct list_head list;
} node_t;

typedef struct hash_table {
	int size;
	int len;
	struct list_head *list_head; /* TODO use hlist_head */
} hash_table_t;

/* htable size must by power of 2 */
hash_table_t *htable_create(int size);

int htable_lookup(const hash_table_t *htable, const sbuf_t *key, sbuf_t **val);
int htable_add(hash_table_t *htable, const sbuf_t *key, const sbuf_t *val);
int __htable_add(hash_table_t *htable, const void *key, int key_len,
		 const void *val, int val_len);
int htable_del(hash_table_t *htable, const sbuf_t *key);
void htable_del_val(hash_table_t *htable, sbuf_t *val);
void htable_free(hash_table_t *htable);

#endif
