#ifndef _AVR_STDIO_H_
#define _AVR_STDIO_H_
#include <stdio.h>

void init_stream0(FILE **out_fd, FILE **in_fd);
void init_stream1(FILE **out_fd, FILE **in_fd);

#endif
