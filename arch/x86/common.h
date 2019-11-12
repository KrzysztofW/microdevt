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

#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdlib.h>

#ifdef X86
#define __LOG__ LOG
#else
#define __LOG__ DEBUG_LOG
#endif

#define __abort() do {							\
		__LOG__("%s:%d aborted\n", __func__, __LINE__);		\
		abort();						\
	} while (0)

#ifdef DEBUG
#define assert(cond) do {			\
		if (!(cond)) {			\
			DEBUG_LOG("%s:%d assert failed\n",		\
				  __func__, __LINE__);			\
			abort();					\
		}							\
	} while (0)
#else
#define assert(cond)
#endif

#endif
