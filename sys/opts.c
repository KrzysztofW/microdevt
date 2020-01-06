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

#include "opts.h"

static int opts_get_arg(uint8_t arg, buf_t *args, buf_t *buf)
{
	sbuf_t sbuf;
	sbuf_t space = SBUF_INITS(" ");
	buf_t b;
	char c_end;
	sbuf_t end;
	uint16_t v16;

	buf_skip_spaces(buf);
	if (arg == ARG_TYPE_NONE || buf->len <= 1)
		return -1;
	if (arg == ARG_TYPE_STRING) {
		/* take the first string from buf */
		if (buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, &space) < 0)
			sbuf_init(&sbuf, buf->data, buf->len);
		return buf_addsbuf(args, &sbuf);
	}
	if (arg == ARG_TYPE_CHAR)
		return buf_addc(args, buf->data[0]);

	c_end = '\0';
	end = SBUF_INIT(&c_end, 1);

	if (buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, &space) < 0
	    && buf_get_sbuf_upto_sbuf_and_skip(buf, &sbuf, &end) < 0)
		return -1;

	b = BUF(8);
	if (sbuf.len == 0 || buf_addsbuf(&b, &sbuf) < 0
	    || buf_addc(&b, c_end) < 0)
		return -1;

	v16 = atoi((char *)b.data);

	switch (arg) {
	case ARG_TYPE_UINT8:
		if (v16 > UINT8_MAX)
			return -1;
		return buf_addc(args, v16);
	case ARG_TYPE_UINT16:
		return buf_add(args, &v16, sizeof(int16_t));
	default:
		return -1;
	}
	return 0;
}

int opts_parse_buf(const cmd_t *cmds, uint8_t cmds_len, buf_t *in,
		   buf_t *args, void (*cb)(uint8_t cmd, buf_t *args))
{
	uint8_t i;

	for (i = 0; i < cmds_len; i++) {
		const cmd_t *cmd = &cmds[i];
		const uint8_t *a;
		sbuf_t sbuf;

		if (buf_get_sbuf_upto_sbuf_and_skip(in, &sbuf, cmd->s) < 0)
			continue;
		if (in->len > args->size)
			return -1;

		a = cmd->args;
		while (a[0] != ARG_TYPE_NONE) {
			if (opts_get_arg(a[0], args, in) < 0)
				return -1;
			a++;
		}
		(*cb)(cmd->cmd, args);
		return 0;
	}
	return -1;
}

void opts_print_usage(cmd_t *cmds, uint8_t cmd_len)
{
	uint8_t i;

	for (i = 0; i < cmd_len; i++) {
		const cmd_t *cmd = &cmds[i];
		const uint8_t *a = cmd->args;
		uint8_t j;

		LOG("%s", cmd->s->data);
		for (j = 0; j < countof(cmd->args) &&
			     *a != ARG_TYPE_NONE; j++) {
			LOG(" ");
			switch (*a) {
			case ARG_TYPE_CHAR:
				LOG("<char>");
				break;
			case ARG_TYPE_UINT8:
				LOG("<uint8>");
				break;
			case ARG_TYPE_UINT16:
				LOG("<uint16>");
				break;
			case ARG_TYPE_STRING:
				LOG("<string>");
				break;
			default:
				__abort();
			}
			a++;
		}
		LOG("\n");
	}
}
