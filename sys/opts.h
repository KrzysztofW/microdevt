/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2020, Krzysztof Witek
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

#ifndef _OPTS_H_
#define _OPTS_H_
#include "buf.h"

typedef enum arg_type {
	ARG_TYPE_NONE,
	ARG_TYPE_CHAR,
	ARG_TYPE_BOOL,
	ARG_TYPE_UINT8,
	ARG_TYPE_UINT16,
	ARG_TYPE_STRING,
} arg_type_t;

#ifndef OPT_MAX_ARGS
#define OPT_MAX_ARGS 2
#endif
typedef struct cmd {
	const sbuf_t *s;
	uint8_t args[OPT_MAX_ARGS];
	uint8_t cmd;
} cmd_t;

int opts_parse_buf(const cmd_t *cmds, uint8_t cmds_len, buf_t *in,
		   buf_t *args, void (*cb)(uint8_t cmd, buf_t *args));
void opts_print_usage(cmd_t *cmds, uint8_t cmd_len);
static inline void
opts_get_string(const cmd_t *cmds, uint8_t cmds_len, uint8_t cmd)
{
	uint8_t i;

	for (i = 0; i < cmds_len; i++)
		if (cmd == cmds[i].cmd)
			LOG("%2u %s\n", cmds[i].cmd, cmds[i].s->data);
}

#endif
