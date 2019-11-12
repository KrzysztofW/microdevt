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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash-tables.h"

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

int htable_add(hash_table_t *htable, const sbuf_t *key, sbuf_t *val)
{
	node_t *new_node;
	sbuf_t *cur_val;
	void *data;
	uint32_t hashval = hash_function(htable, *key);

	assert(htable->len >= 0);

	/* if the key already exists do not insert it again */
	if (__htable_lookup(htable, hashval, key, &cur_val) == 0)
		return -1;

	if ((new_node = malloc(sizeof(node_t))) == NULL)
		return -1;

	if ((data = malloc(key->len)) == NULL) {
		free(new_node);
		return -1;
	}

	memcpy(data, key->data, key->len);
	new_node->key.data = data;
	new_node->key.len = key->len;

	if ((data = malloc(val->len)) == NULL) {
		free((void *)new_node->key.data);
		free(new_node);
		return -1;
	}

	memcpy(data, val->data, val->len);
	new_node->val.data = data;
	new_node->val.len = val->len;
	val->data = data;
	INIT_LIST_HEAD(&new_node->list);

	list_add_tail(&new_node->list, &htable->list_head[hashval]);
	htable->len++;

	return 0;
}

static void htable_free_node(node_t *node)
{
	list_del(&node->list);
	free((void *)node->key.data);
	free((void *)node->val.data);
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
	list_t *list, *tmp;

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

void htable_for_each(hash_table_t *htable,
		     int (*cb)(sbuf_t *key, sbuf_t *val, void **arg),
		     void **arg)
{
	int i;

	for (i = 0; i < htable->size; i++) {
		node_t *e;
		list_t *list, *tmp;

		list_for_each_safe(list, tmp, &htable->list_head[i]) {
			e = list_entry(list, node_t, list);
			if (cb(&e->key, &e->val, arg) < 0)
				return;
		}
	}
}

static int ht_free_node_cb(sbuf_t *key, sbuf_t *val, void **arg)
{
	node_t *node = container_of(val, node_t, val);

	(void)key;
	(void)arg;

	htable_free_node(node);
	return 0;
}

void htable_free(hash_table_t *htable)
{
	htable_for_each(htable, ht_free_node_cb, NULL);
}
