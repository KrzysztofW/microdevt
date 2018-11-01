#ifndef _LIST_H
#define _LIST_H

#include "utils.h"

#define LIST_POISON1 (void *)0xDEADBEEF
#define LIST_POISON2 (void *)0xBEEFDEAD

/* #define LIST_DEBUG */

struct list_head {
	struct list_head *next, *prev;
};

typedef struct list_head list_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) list_t name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(list_t *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(list_t *new, list_t *prev, list_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

#ifdef LIST_DEBUG
#define list_add(new, head)						\
	DEBUG_LOG("%s:%d add:%p\n", __func__, __LINE__, new);		\
	__list_add((new), (head), (head)->next)

#define list_add_tail(new, head)					\
	DEBUG_LOG("%s:%d add:%p\n", __func__, __LINE__, new);		\
	__list_add((new), (head)->prev, (head))
#else
static inline void list_add(list_t *new, list_t *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(list_t *new, list_t *head)
{
	__list_add(new, head->prev, head);
}
#endif

static inline void __list_del(list_t *prev, list_t *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void __list_del_entry(list_t *entry)
{
	__list_del(entry->prev, entry->next);
}

#ifdef LIST_DEBUG
#define list_del(e)							\
	DEBUG_LOG("%s:%d del:%p\n", __func__, __LINE__, e);		\
	__list_del_entry(e);						\
	(e)->next = LIST_POISON1;					\
	(e)->prev = LIST_POISON2
#else
static inline void list_del(list_t *entry)
{
	__list_del_entry(entry);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
#endif

#ifdef LIST_DEBUG
#define list_del_init(e)						\
	DEBUG_LOG("%s:%d del:%p\n", __func__, __LINE__, e);		\
	__list_del_entry(e);						\
	INIT_LIST_HEAD(e)
#else
static inline void list_del_init(list_t *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}
#endif

static inline void list_move(list_t *list, list_t *head)
{
	__list_del_entry(list);
	list_add(list, head);
}

static inline void list_move_tail(list_t *list, list_t *head)
{
	__list_del_entry(list);
	list_add_tail(list, head);
}

static inline int list_is_last(const list_t *list, const list_t *head)
{
	return list->next == head;
}

static inline int list_empty(const list_t *head)
{
	return head->next == head;
}

static inline int list_is_singular(const list_t *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

#define list_next_entry(pos, member)				\
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member)				\
	list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each(pos, head)					\
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head)					\
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_safe(pos, n, head)		       \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_prev_safe(pos, n, head)	\
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head);					\
	     pos = list_prev_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = n, n = list_next_entry(n, member))

/* singly linked lists */
typedef struct slist_node {
	struct slist_node *next;
} slist_node_t;

typedef struct slist_head {
	slist_node_t *head;
	slist_node_t *tail;
} slist_t;

#define SLIST_HEAD_INIT(name) { .tail = NULL, .head = NULL }
#define SLIST_HEAD(name) slist_t name = SLIST_HEAD_INIT(name)

static inline void INIT_SLIST_HEAD(slist_t *list)
{
	list->tail = NULL;
	list->head = NULL;
}

static inline int slist_empty(const slist_t *list)
{
	return list->tail == NULL;
}

static inline void slist_add(slist_node_t *node, slist_t *list)
{
	if (slist_empty(list))
	    list->tail = node;
	node->next = list->head;
	list->head = node;
}

static inline void slist_add_tail(slist_node_t *node, slist_t *list)
{
	if (slist_empty(list)) {
		slist_add(node, list);
		return;
	}
	node->next = NULL;
	list->tail->next = node;
	list->tail = node;
}

#define slist_first_entry(list, type, member)	\
	list_entry((list)->head, type, member)

static inline slist_node_t *slist_get_first(slist_t *list)
{
	slist_node_t *node;

	if (list->head == NULL)
		return NULL;
	node = list->head;
	list->head = node->next;
	if (list->tail == node)
		list->tail = NULL;
	return node;
}

#define slist_for_each(pos, list)				\
	for (pos = (list)->head; pos != NULL; pos = pos->next)

#endif
