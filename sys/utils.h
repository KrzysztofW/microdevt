#ifndef _UTILS_H_
#define _UTILS_H_

#define POWEROF2(x) ((((x) - 1) & (x)) == 0)

#define __error__(msg) (void)({__asm__(".error \""msg"\"");})
#define STATIC_ASSERT(cond)                                             \
	__builtin_choose_expr(cond, (void)0,                            \
			      __error__("static assertion failed: "#cond""))

#define htons(x) ((x) >> 8 | (x) << 8)
#define ntohs(x) htons(x)
#define htonl(l) (((l)<<24) | (((l)&0x00FF0000l)>>8) | (((l)&0x0000FF00l)<<8) | ((l)>>24))
#define ntohl(x) htonl(x)

#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif
