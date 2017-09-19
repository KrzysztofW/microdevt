#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash-tables.h"

hash_table_t *htable_create(int size)
{
	hash_table_t *htable;
	int i;

	if (size < 1 || !POWEROF2(size))
		return NULL;

	if ((htable = malloc(sizeof(hash_table_t))) == NULL) {
		return NULL;
	}

	htable->list_head = malloc(sizeof(struct list_head) * size);
	if (htable->list_head == NULL)
		return NULL;

	for (i = 0; i < size; i++)
		INIT_LIST_HEAD(&htable->list_head[i]);

	htable->size = size;
	htable->len = 0;

	return htable;
}

static inline uint32_t hash_function(const hash_table_t *htable, sbuf_t key)
{
	uint32_t hashval = 0;

	while (key.len--) {
		hashval = *key.data + (hashval << 5) - hashval;
		key.data++;
	}

	return hashval & (htable->size - 1);
}

static int __htable_lookup(const hash_table_t *htable, uint32_t hashval,
			   const sbuf_t *key, sbuf_t **val)
{
	node_t *e;

	list_for_each_entry(e, &htable->list_head[hashval], list) {
		if (sbuf_cmp(key, &e->key) == 0) {
			*val = &e->val;
			return hashval;
		}
	}
	return -1;
}

int htable_lookup(const hash_table_t *htable, const sbuf_t *key,
		  sbuf_t **val)
{
	uint32_t hashval = hash_function(htable, *key);

	return __htable_lookup(htable, hashval, key, val);
}

int htable_add(hash_table_t *htable, const sbuf_t *key,
	       const sbuf_t *val)
{
	node_t *new_node;
	sbuf_t *cur_val;
	unsigned char *data;
	uint32_t hashval = hash_function(htable, *key);

	assert(htable->len >= 0);

	/* if the key already exists do not insert it again */
	if (__htable_lookup(htable, hashval, key, &cur_val) == 0)
		return -1;

	if ((new_node = malloc(sizeof(node_t))) == NULL)
		return -1;

	if ((data = malloc(key->len)) == NULL)
		return -1;

	memcpy(data, key->data, key->len);
	new_node->key.data = data;
	new_node->key.len = key->len;

	if ((data = malloc(val->len)) == NULL)
		return -1;

	memcpy(data, val->data, val->len);
	new_node->val.data = data;
	new_node->val.len = val->len;
	INIT_LIST_HEAD(&new_node->list);

	list_add_tail(&new_node->list, &htable->list_head[hashval]);
	htable->len++;

	return 0;
}

int __htable_add(hash_table_t *htable, const void *key, int key_len,
		 const void *val, int val_len)
{
	sbuf_t buf_key, buf_val;

	sbuf_init(&buf_key, key, key_len);
	sbuf_init(&buf_val, val, val_len);

	return htable_add(htable, &buf_key, &buf_val);
}

static void htable_free_node(node_t *node)
{
	unsigned char *data;

	list_del(&node->list);
	data = (unsigned char *)node->key.data;
	free(data);
	data = (unsigned char *)node->val.data;
	free(data);
	free(node);
}

void htable_del_val(hash_table_t *htable, sbuf_t *val)
{
	node_t *node = container_of(val, node_t, val);

	htable_free_node(node);
	htable->len--;
}

int htable_del(hash_table_t *htable, const sbuf_t *key)
{
	uint32_t hashval = hash_function(htable, *key);
	node_t *e;
	struct list_head *list, *tmp;

	list_for_each_safe(list, tmp, &htable->list_head[hashval]) {
		e = list_entry(list, node_t, list);

		if (sbuf_cmp(key, &e->key) < 0)
			continue;

		htable_free_node(e);
		htable->len--;
		return 0;
	}
	assert(htable->len >= 0);
	return -1;
}

void
htable_for_each(hash_table_t *htable, void (*cb)(sbuf_t *key, sbuf_t *val))
{
	int i;

	for (i = 0; i < htable->size; i++) {
		node_t *e;
		struct list_head *list, *tmp;

		list_for_each_safe(list, tmp, &htable->list_head[i]) {
			e = list_entry(list, node_t, list);
			cb(&e->key, &e->val);
		}
	}
}

static void ht_free_node_cb(sbuf_t *key, sbuf_t *val)
{
	node_t *node = container_of(val, node_t, val);

	(void)key;
	htable_free_node(node);
}

void htable_free(hash_table_t *htable)
{
	htable_for_each(htable, ht_free_node_cb);
	free(htable->list_head);
	free(htable);
}
