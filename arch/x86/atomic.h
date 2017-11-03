#ifndef _ATOMIC_H_
#define _ATOMIC_H_

typedef volatile int atomic_t;

#define atomic_add_fetch(object, operand)	\
	__sync_add_and_fetch(object, operand)
#define atomic_sub_fetch(object, operand)	\
	__sync_sub_and_fetch(object, operand)
#define atomic_load(object) *(object)
#define atomic_store(object, operand) *object = operand

#endif
