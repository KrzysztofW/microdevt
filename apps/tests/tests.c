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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/array.h>
#include <sys/ring.h>
#include <sys/list.h>
#include <sys/hash-tables.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <net/tests.h>
#include <drivers/rf.h>
#include <drivers/gsm-at.h>

#define CHK_RSIZE 64
static struct iface_queues {
	RING_DECL_IN_STRUCT(pkt_pool, CONFIG_PKT_DRIVER_NB_MAX);
	RING_DECL_IN_STRUCT(rx, CONFIG_PKT_NB_MAX);
	RING_DECL_IN_STRUCT(tx, CONFIG_PKT_NB_MAX);
} iface_queues = {
	.pkt_pool = RING_INIT(iface_queues.pkt_pool),
	.rx = RING_INIT(iface_queues.rx),
	.tx = RING_INIT(iface_queues.tx),
};
static rf_ctx_t rf_ctx;

static int fill_ring(ring_t *ring, unsigned char *bytes, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (ring_addc(ring, bytes[i]) < 0)
			return -1;
	}
	return 0;
}

static int ring_check_full(ring_t *ring)
{
	int i;
	unsigned char c;

	for (i = 0; i < CHK_RSIZE - 1; i++) {
		if (ring_addc(ring, 1) < 0)
			return -1;
	}
	if (ring_len(ring) != CHK_RSIZE - 1) {
		return -1;
	}
	ring_getc(ring, &c);
	if (ring_len(ring) != CHK_RSIZE - 2) {
		return -1;
	}
	if (ring_addc(ring, 1) < 0)
		return -1;
	if (ring_len(ring) != CHK_RSIZE - 1) {
		return -1;
	}

	return 0;
}

static int ring_check(void)
{
	RING_DECL(ring, CHK_RSIZE);
	unsigned int i;
	unsigned char bytes[] = { 0x3C, 0xFF, 0xA0, 0x00, 0x10, 0xB5, 0x99,
				  0x11, 0xF7 };
	unsigned char c;
	int j;

	if (ring_check_full(ring) < 0) {
		fprintf(stderr, "check full failed\n");
		return -1;
	}

	ring_reset(ring);
	fill_ring(ring, bytes, sizeof(bytes));

	/* printf("ring content:\n"); */
	/* ring_print(ring); */
	i = 0;
	while (ring_getc(ring, &c) == 0) {
		if (c != bytes[i]) {
			fprintf(stderr, "ring checks failed. Got: 0x%X should be: 0x%X\n",
				c, bytes[i]);
			return -1;
		}
		i++;
	}

	/* fill up the whole ring */
	ring_reset(ring);
	while (fill_ring(ring, bytes, sizeof(bytes)) == 0) {}

	if (ring_addc(ring, bytes[0]) == 0) {
		fprintf(stderr, "ring overloaded\n");
		return -1;
	}
	if (ring_getc(ring, &c) < 0) {
		fprintf(stderr, "cannot take elements\n");
		return -1;
	}
	if (c != bytes[0]) {
		fprintf(stderr, "elements mismatch\n");
		return -1;
	}
	j = 1;
	/* ring_print(ring); */
	/* ring_print_bits(ring); */
	for (i = 1; i < CHK_RSIZE - 1; i++) {
		if (ring_getc(ring, &c) < 0 || c != bytes[j]) {
			printf("ring content:\n");
			ring_print(ring);
			fprintf(stderr, "failed getting %dth element from ring (len:%d)\n", i,
				ring_len(ring));
			return -1;
		}
		j = (j + 1) % sizeof(bytes);
	}
	while (fill_ring(ring, bytes, sizeof(bytes)) == 0) {}
	ring_skip(ring, 2);
	if (ring_len(ring) != CHK_RSIZE - 3) {
		fprintf(stderr, "ring skip 1 failed (ring size:%d)\n",
			ring_len(ring));
		return -1;
	}
	ring_addc(ring, 0xAA);
	ring_addc(ring, 0xFF);
	if (ring_len(ring) != CHK_RSIZE - 1) {
		fprintf(stderr, "ring skip 2 failed (ring size:%d)\n",
			ring_len(ring));
		return -1;
	}
	ring_skip(ring, CHK_RSIZE);
	if (ring_len(ring) != 0) {
		fprintf(stderr, "ring skip 3 failed (ring size:%d)\n",
			ring_len(ring));
		return -1;
	}

	return 0;
}

typedef struct list_el {
	list_t list;
	slist_node_t slist;
	int a;
	int b;
} list_el_t;

#define LIST_ELEMS 10

static int list_check(void)
{
	int i;
	list_el_t *e;
	list_t list_head;
	list_t *node, *tmp;

	INIT_LIST_HEAD(&list_head);
	for (i = 0; i < LIST_ELEMS; i++) {
		list_el_t *el = malloc(sizeof(list_el_t));

		if (el == NULL) {
			fprintf(stderr, "malloc failed\n");
			exit(EXIT_FAILURE);
		}
		memset(el, 0, sizeof(list_el_t));
		el->a = el->b = i;
		list_add(&el->list, &list_head);
	}

	i = LIST_ELEMS - 1;
	list_for_each_entry(e, &list_head, list) {
		if (i != e->a && i != e->b) {
			fprintf(stderr, "%s:%d failed i:%d a:%d b:%d\n",
				__func__, __LINE__, i, e->a, e->b);
			return -1;
		}
		i--;
	}

	if (i != -1) {
		fprintf(stderr, "%s:%d failed i:%d\n", __func__, __LINE__, i);
		return -1;
	}
	list_for_each_safe(node, tmp, &list_head) {
		e = list_entry(node, list_el_t, list);
		list_del(node);
		free(e);
	}
	return 0;
}

static int slist_check(void)
{
	int i;
	list_el_t *e;
	slist_t list_head;
	slist_node_t *node;

	INIT_SLIST_HEAD(&list_head);

	/* slist add head: */
	for (i = 0; i < LIST_ELEMS; i++) {
		list_el_t *el = malloc(sizeof(list_el_t));

		if (el == NULL) {
			fprintf(stderr, "malloc failed\n");
			exit(EXIT_FAILURE);
		}
		memset(el, 0, sizeof(list_el_t));
		el->a = el->b = i;
		slist_add(&el->slist, &list_head);
	}

	e = slist_first_entry(&list_head, list_el_t, slist);
	i = LIST_ELEMS - 1;
	if (e->a != i) {
		fprintf(stderr, "%s:%d failed a != %d\n",
			__func__, __LINE__, i);
		return -1;
	}

	slist_for_each(node, &list_head) {
		e = list_entry(node, list_el_t, slist);
		if (i != e->a && i != e->b) {
			fprintf(stderr, "%s:%d failed i:%d a:%d b:%d\n",
				__func__, __LINE__, i, e->a, e->b);
			return -1;
		}
		i--;
	}

	if (i != -1) {
		fprintf(stderr, "%s:%d failed i:%d\n", __func__, __LINE__, i);
		return -1;
	}

	while ((node = slist_get_first(&list_head))) {
		e = list_entry(node, list_el_t, slist);
		free(e);
	}

	/* slist add tail: */
	for (i = 0; i < LIST_ELEMS; i++) {
		list_el_t *el = malloc(sizeof(list_el_t));

		if (el == NULL) {
			fprintf(stderr, "malloc failed\n");
			exit(EXIT_FAILURE);
		}
		memset(el, 0, sizeof(list_el_t));
		el->a = el->b = i;
		slist_add_tail(&el->slist, &list_head);
	}

	e = slist_first_entry(&list_head, list_el_t, slist);
	i = 0;
	if (e->a != i) {
		fprintf(stderr, "%s:%d failed a != %d\n",
			__func__, __LINE__, i);
		return -1;
	}
	slist_for_each(node, &list_head) {
		e = list_entry(node, list_el_t, slist);
		if (i != e->a && i != e->b) {
			fprintf(stderr, "%s:%d failed i:%d a:%d b:%d\n",
				__func__, __LINE__, i, e->a, e->b);
			return -1;
		}
		i++;
	}

	if (i != 10) {
		fprintf(stderr, "%s:%d failed i:%d\n", __func__, __LINE__, i);
		return -1;
	}

	while ((node = slist_get_first(&list_head))) {
		e = list_entry(node, list_el_t, slist);
		free(e);
	}

	return 0;
}

#ifdef CONFIG_HT_STORAGE
static int htable_check(int htable_size)
{
	HTABLE_DECL(htable, htable_size);
	unsigned i;
	const char *keys[] = {
		"key1", "key2", "fdsa", "fewsfesf", "feafse",
	};
	const char *vals[] = {
		"val1", "val2", "rerere", "eeeee", "wwww",
	};
	unsigned count = sizeof(keys) / sizeof(char *);

	htable_init(&htable);

	for (i = 0; i < count; i++) {
		sbuf_t key, val, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));
		sbuf_init(&val, (unsigned char *)vals[i], strlen(vals[i]));

		if (htable.len != (int)i) {
			fprintf(stderr, "wrong htable length %d. Should be %d (0)\n",
				htable.len, i);
			return -1;
		}

		if (htable_add(&htable, &key, &val) < 0) {
			fprintf(stderr, "htable: can't add entry\n");
			return -1;
		}
		if (htable_lookup(&htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
	}

	for (i = 0; i < count; i++) {
		sbuf_t key, val, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));
		sbuf_init(&val, (unsigned char *)vals[i], strlen(vals[i]));

		if (htable_lookup(&htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
		if (sbuf_cmp(&val, val2) < 0) {
			fprintf(stderr, "found val: `%.*s' (expected: `%.*s'\n",
				val.len, val.data, val2->len, val2->data);
			return -1;
		}
	}

	for (i = 0; i < count; i++) {
		sbuf_t key, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));

		if (htable.len != (int)(count - i)) {
			fprintf(stderr, "wrong htable length %d. Should be %d (1)\n",
				htable.len, i);
			return -1;
		}

		if (htable_del(&htable, &key) < 0) {
			fprintf(stderr, "can't delete key: %.*s\n",
				key.len, key.data);
		}

		if (htable_lookup(&htable, &key, &val2) >= 0) {
			fprintf(stderr, "key: %.*s should not be present\n",
				key.len, key.data);
			return -1;
		}
	}

	/* check htable_del_val() */
	for (i = 0; i < count; i++) {
		sbuf_t key, val, *val2 = NULL;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));
		sbuf_init(&val, (unsigned char *)vals[i], strlen(vals[i]));

		if (htable.len != (int)i) {
			fprintf(stderr, "wrong htable length %d. Should be %d (2)\n",
				htable.len, i);
			return -1;
		}

		if (htable_add(&htable, &key, &val) < 0) {
			fprintf(stderr, "htable: can't add entry\n");
			return -1;
		}
		if (htable_lookup(&htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
	}

	for (i = 0; i < count; i++) {
		sbuf_t key, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));

		if (htable.len != (int)(count - i)) {
			fprintf(stderr, "wrong htable length %d. Should be %d (2)\n",
				htable.len, i);
			return -1;
		}

		if (htable_lookup(&htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
		htable_del_val(&htable, val2);
		if (htable_lookup(&htable, &key, &val2) >= 0) {
			fprintf(stderr, "found key: `%.*s' but shouldn't\n",
				key.len, key.data);
			return -1;
		}
	}

	{
		int k = 12;
		uint16_t v = 3, *v2;
		sbuf_t key, val, *val2;

		sbuf_init(&key, &k, sizeof(k));
		sbuf_init(&val, &v, sizeof(v));

		if (htable_add(&htable, &key, &val) < 0) {
			fprintf(stderr, "htable: can't add entry\n");
			return -1;
		}
		if (htable_lookup(&htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
		v2 = (uint16_t *)val2->data;
		if (v != *v2) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
	}

	htable_free(&htable);

	return 0;
}
#endif

typedef struct timer_el {
	tim_t timer;
	int val;
} timer_el_t;

static int val_g;

static void timer_cb(void *arg)
{
	timer_el_t *el = arg;

	(void)el;
	assert(val_g == el->val);
	val_g++;
}

static int timer_check(void)
{
#define TIM_CNT 1024
#define TIMER_TABLE_SIZE 512
	timer_el_t timer_els[TIM_CNT];
	int i;

	memset(timer_els, 0, sizeof(timer_el_t) * TIM_CNT);
	timer_subsystem_init();

	for (i = 0; i < TIM_CNT; i++) {
		timer_el_t *tim_el = &timer_els[i];

		tim_el->val = i;
		timer_init(&tim_el->timer);
		timer_add(&tim_el->timer, i, timer_cb, tim_el);
	}

	for (i = 0; i < TIMER_TABLE_SIZE * 2; i++) {
		timer_process();
	}

	timer_subsystem_shutdown();
	return 0;
}

static int send(const struct iface *iface, pkt_t *pkt)
{
	return 0;
}

static void recv(const struct iface *iface) {}

static void net_swen_l3_flush_scheduler(void)
{
	int i;

	for (i = 0; i < 10; i++)
		scheduler_run_task();
}

static int driver_rf_checks(void)
{
	iface_t iface;
	int ret = -1;
	uint8_t hw_addr = 0x6F;

	pkt_mempool_init();
	memset(&iface, 0, sizeof(iface_t));
	iface.send = &send;
	iface.recv = &recv;
	iface.hw_addr = &hw_addr;
	if_init(&iface, IF_TYPE_RF, &iface_queues.pkt_pool, &iface_queues.rx,
		&iface_queues.tx, 1);

	rf_init(&iface, &rf_ctx, 2);
	ret = rf_checks(&iface);
	net_swen_l3_flush_scheduler();

	pkt_mempool_shutdown();
	return ret;
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (array_tests() < 0) {
		fprintf(stderr, "  ==> array checks failed\n");
		return -1;
	}
	printf("  ==> array checks succeeded\n");
	if (ring_check() < 0) {
		fprintf(stderr, "  ==> ring checks failed\n");
		return -1;
	}
	printf("  ==> ring checks succeeded\n");
	if (list_check() < 0) {
		fprintf(stderr, "  ==> double linked list checks failed\n");
		return -1;
	}
	printf("  ==> double linked list checks succeeded\n");

	if (slist_check() < 0) {
		fprintf(stderr, "  ==> singly linked list checks failed\n");
		return -1;
	}
	printf("  ==> singly linked list checks succeeded\n");

#ifdef CONFIG_HT_STORAGE
	if (htable_check(1024) < 0) {
		fprintf(stderr, "  ==> htable checks failed (htable size: 1024)\n");
		return -1;
	}
	if (htable_check(4) < 0) {
		fprintf(stderr, "  ==> htable checks failed (htable size: 4)\n");
		return -1;
	}

	printf("  ==> htable checks succeeded\n");
#endif
	if (timer_check() < 0) {
		fprintf(stderr, "  ==> timer checks failed\n");
		return -1;
	}
	printf("  ==> timer checks succeeded\n");

	if (driver_rf_checks() < 0) {
		fprintf(stderr, "  ==> driver RF tests failed\n");
		return -1;
	}
	printf("  ==> driver RF tests succeeded\n");

	if (gsm_tests() < 0) {
		fprintf(stderr, "  ==> driver gsm-at tests failed\n");
		return -1;
	}
	printf("  ==> driver gsm-at tests succeeded\n");

#ifdef CONFIG_SWEN_L3
	if (net_swen_l3_tests() < 0) {
		fprintf(stderr, "  ==> net swen-l3 tests failed\n");
		return -1;
	}
	printf("  ==> net swen-l3 tests succeeded\n");
#endif
	if (net_arp_tests() < 0) {
		fprintf(stderr, "  ==> net arp tests failed\n");
		return -1;
	}
	printf("  ==> net arp tests succeeded\n");

#ifdef CONFIG_ICMP
	if (net_icmp_tests() < 0) {
		fprintf(stderr, "  ==> net icmp tests failed\n");
		return -1;
	}
	printf("  ==> net icmp tests succeeded\n");
#endif
#ifdef CONFIG_UDP
	if (net_udp_tests() < 0) {
		fprintf(stderr, "  ==> net udp tests failed\n");
		return -1;
	}
	printf("  ==> net udp tests succeeded\n");
#endif
#ifdef CONFIG_TCP
	if (net_tcp_tests() < 0) {
		fprintf(stderr, "  ==> net tcp tests failed\n");
		return -1;
	}
	printf("  ==> net tcp tests succeeded\n");
#endif
	return 0;
}
