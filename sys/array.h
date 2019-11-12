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

#ifndef _ARRAY_H_
#define _ARRAY_H_

void array_shell_sort(int *array, unsigned size);
int array_get_median(int *array, unsigned int size);
int array_get_average(int *array, unsigned int size);
void array_left_shift(int *array, unsigned int size, unsigned int shift);
void array_right_shift(int *array, unsigned int size, unsigned int shift);
void array_copy(int *dst, int *src, unsigned int size);
void array_print(int *array, unsigned size);
int array_tests(void);

#endif
