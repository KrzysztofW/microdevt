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

#include <avr/io.h>
#include <stdlib.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "utils.h"

#define __abort() do {							\
		DEBUG_LOG("%s:%d aborted\n", __func__, __LINE__);	\
		exit(1);						\
	} while (0)

#ifdef DEBUG
#define assert(cond) do {			\
		if (!(cond)) {			\
			DEBUG_LOG("%s:%d assert failed\n",		\
				  __func__, __LINE__);			\
			exit(1);					\
		}							\
	} while (0)
#else
#define assert(cond)
#endif
#endif
