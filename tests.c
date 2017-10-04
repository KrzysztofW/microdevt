#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

#include "sys/ring.h"
#include "sys/list.h"
#include "sys/hash-tables.h"
#include "timer.h"
#include "net/tests.h"

static int fill_ring(ring_t *ring, unsigned char *bytes, int len)
{
	int i, j;

	for (i = 0; i < len; i++) {
		unsigned char b = bytes[i];

		if (ring_is_full(ring))
			return -1;
		for (j = 0; j < 8; j++) {
			unsigned char bit = b >> 7;
			ring_add_bit(ring, bit);
			b = b << 1;
		}
	}
	return 0;
}

#define ZCHK_RSIZE 64

static int ring_check_full(ring_t *ring)
{
	int i;
	unsigned char c;

	for (i = 0; i < ZCHK_RSIZE - 1; i++) {
		ring_addc(ring, 1);
	}
	if (ring_len(ring) != ZCHK_RSIZE - 1) {
		return -1;
	}
	ring_getc(ring, &c);
	if (ring_len(ring) != ZCHK_RSIZE - 2) {
		return -1;
	}
	if (ring_addc(ring, 1) < 0)
		return -1;
	if (ring_len(ring) != ZCHK_RSIZE - 1) {
		return -1;
	}

	return 0;
}

static int ring_check(void)
{
	ring_t *ring;
	unsigned int i;
	unsigned char bytes[] = { 0x3C, 0xFF, 0xA0, 0x00, 0x10, 0xB5, 0x99,
				  0x11, 0xF7 };
	unsigned char c;
	int j;

	if ((ring = ring_create(ZCHK_RSIZE)) == NULL) {
		fprintf(stderr, "can't create ring\n");
		free(ring);
		return -1;
	}

	if (ring_check_full(ring) < 0) {
		fprintf(stderr, "check full failed\n");
		free(ring);
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
			free(ring);
			return -1;
		}
		i++;
	}

	/* fill up the whole ring */
	ring_reset(ring);
	while (fill_ring(ring, bytes, sizeof(bytes)) == 0) {}

	if (ring_addc(ring, bytes[0]) == 0) {
		fprintf(stderr, "ring overloaded\n");
		free(ring);
		return -1;
	}
	if (ring_getc(ring, &c) < 0) {
		fprintf(stderr, "cannot take elements\n");
		free(ring);
		return -1;
	}
	if (c != bytes[0]) {
		fprintf(stderr, "elements mismatch\n");
		free(ring);
		return -1;
	}
	j = 1;
	/* ring_print(ring); */
	/* ring_print_bits(ring); */
	for (i = 1; i < ZCHK_RSIZE - 1; i++) {
		if (ring_getc(ring, &c) < 0 || c != bytes[j]) {
			printf("ring content:\n");
			ring_print(ring);
			fprintf(stderr, "failed getting %dth element from ring (len:%d)\n", i,
				ring_len(ring));
			free(ring);
			return -1;
		}
		j = (j + 1) % sizeof(bytes);
	}
	while (fill_ring(ring, bytes, sizeof(bytes)) == 0) {}
	ring_skip(ring, 2);
	if (ring_len(ring) != ZCHK_RSIZE - 3) {
		fprintf(stderr, "ring skip 1 failed (ring size:%d)\n",
			ring_len(ring));
		free(ring);
		return -1;
	}
	ring_addc(ring, 0xAA);
	ring_addc(ring, 0xFF);
	if (ring_len(ring) != ZCHK_RSIZE - 1) {
		fprintf(stderr, "ring skip 2 failed (ring size:%d)\n",
			ring_len(ring));
		free(ring);
		return -1;
	}
	ring_skip(ring, ZCHK_RSIZE);
	if (ring_len(ring) != 0) {
		fprintf(stderr, "ring skip 3 failed (ring size:%d)\n",
			ring_len(ring));
		free(ring);
		return -1;
	}

	free(ring);
	return 0;
}

typedef struct list_el {
	struct list_head list;
	int a;
	int b;
} list_el_t;

#define LIST_ELEMS 10

static int list_check(void)
{
	int i;
	list_el_t *e;
	struct list_head list_head;
	struct list_head *node, *tmp;

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

static int htable_check(int htable_size)
{
	hash_table_t *htable = htable_create(htable_size);
	unsigned i;
	const char *keys[] = {
		"key1", "key2", "fdsa", "fewsfesf", "feafse",
	};
	const char *vals[] = {
		"val1", "val2", "rerere", "eeeee", "wwww",
	};
	unsigned count = sizeof(keys) / sizeof(char *);

	if (htable == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		sbuf_t key, val, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));
		sbuf_init(&val, (unsigned char *)vals[i], strlen(vals[i]));

		if (htable->len != (int)i) {
			fprintf(stderr, "wrong htable length %d. Should be %d (0)\n",
				htable->len, i);
			return -1;
		}

		if (htable_add(htable, &key, &val) < 0) {
			fprintf(stderr, "htable: can't add entry\n");
			return -1;
		}
		if (htable_lookup(htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
	}

	for (i = 0; i < count; i++) {
		sbuf_t key, val, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));
		sbuf_init(&val, (unsigned char *)vals[i], strlen(vals[i]));

		if (htable_lookup(htable, &key, &val2) < 0) {
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

		if (htable->len != (int)(count - i)) {
			fprintf(stderr, "wrong htable length %d. Should be %d (1)\n",
				htable->len, i);
			return -1;
		}

		if (htable_del(htable, &key) < 0) {
			fprintf(stderr, "can't delete key: %.*s\n",
				key.len, key.data);
		}

		if (htable_lookup(htable, &key, &val2) >= 0) {
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

		if (htable->len != (int)i) {
			fprintf(stderr, "wrong htable length %d. Should be %d (2)\n",
				htable->len, i);
			return -1;
		}

		if (htable_add(htable, &key, &val) < 0) {
			fprintf(stderr, "htable: can't add entry\n");
			return -1;
		}
		if (htable_lookup(htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
	}

	for (i = 0; i < count; i++) {
		sbuf_t key, *val2;

		sbuf_init(&key, (unsigned char *)keys[i], strlen(keys[i]));

		if (htable->len != (int)(count - i)) {
			fprintf(stderr, "wrong htable length %d. Should be %d (2)\n",
				htable->len, i);
			return -1;
		}

		if (htable_lookup(htable, &key, &val2) < 0) {
			fprintf(stderr, "can't find key: %.*s\n",
				key.len, key.data);
			return -1;
		}
		htable_del_val(htable, val2);
		if (htable_lookup(htable, &key, &val2) >= 0) {
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

		if (htable_add(htable, &key, &val) < 0) {
			fprintf(stderr, "htable: can't add entry\n");
			return -1;
		}
		if (htable_lookup(htable, &key, &val2) < 0) {
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

	htable_free(htable);

	return 0;
}

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
	timer_subsystem_init(1);
	for (i = 0; i < TIM_CNT; i++) {
		timer_el_t *tim_el = &timer_els[i];

		tim_el->val = i;
		timer_add(&tim_el->timer, i, timer_cb, tim_el);
	}

	for (i = 0; i < TIMER_TABLE_SIZE * 2; i++) {
		__zchk_timer_process();
	}

	return 0;
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	if (ring_check() < 0) {
		fprintf(stderr, "  ==> ring checks failed\n");
		return -1;
	}
	printf("  ==> ring checks succeeded\n");
	if (list_check() < 0) {
		fprintf(stderr, "  ==> list checks failed\n");
		return -1;
	}
	printf("  ==> list checks succeeded\n");

	if (htable_check(1024) < 0) {
		fprintf(stderr, "  ==> htable checks failed (htable size: 1024)\n");
		return -1;
	}
	if (htable_check(4) < 0) {
		fprintf(stderr, "  ==> htable checks failed (htable size: 4)\n");
		return -1;
	}

	printf("  ==> htable checks succeeded\n");

	if (timer_check() < 0) {
		fprintf(stderr, "  ==> timer checks failed\n");
		return -1;
	}
	printf("  ==> timer checks succeeded\n");
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
