#ifndef _UTILS_H_
#define _UTILS_H_

#define POWEROF2(x) ((((x) - 1) & (x)) == 0)

#define __error__(msg) (void)({__asm__(".error \""msg"\"");})
#define STATIC_ASSERT(cond)                                             \
	__builtin_choose_expr(cond, (void)0,                            \
			      __error__("static assertion failed: "#cond""))

#endif
