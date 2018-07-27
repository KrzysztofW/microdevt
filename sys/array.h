#ifndef _ARRAY_H_
#define _ARRAY_H_

void array_shell_sort(int *array, unsigned size);
int array_get_median(int *array, unsigned int size);
int array_get_average(int *array, unsigned int size);
void array_left_shift(int *array, unsigned int size, unsigned int shift);
void array_right_shift(int *array, unsigned int size, unsigned int shift);
void array_print(int *array, unsigned size);
int array_tests(void);

#endif
