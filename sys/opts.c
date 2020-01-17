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

typedef enum loop_ret {
	LOOP_STOP_FAILED,
	LOOP_STOP_SUCCESS,
	LOOP_CONTINUE,
} loop_ret_t;

typedef struct opts_parse_data {
	buf_t *in_buf;
	buf_t *arg_buf;
	void (*ext_cb)(uint8_t cmd, buf_t *args);
} opts_parse_data_t;

static int opts_get_arg(uint8_t arg, buf_t *args, buf_t *buf)
{
	sbuf_t sbuf;
	sbuf_t space = SBUF_INITS(" ");
	buf_t b;
	char c_end;
	sbuf_t end;
	int16_t v;

	buf_skip_spaces(buf);
	if (buf->len <= 1)
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

	v = atoi((char *)b.data);

	if (arg < ARG_TYPE_INT8 && v < 0)
		return -1;

	switch (arg) {
	case ARG_TYPE_UINT8:
	case ARG_TYPE_INT8:
		return buf_addc(args, v);
	case ARG_TYPE_UINT16:
	case ARG_TYPE_INT16:
		return buf_add(args, &v, sizeof(int16_t));
	default:
		return -1;
	}
	return 0;
}

static uint8_t opts_parse_buf_cb(const cmd_t *cmd, void *arg)
{
	const uint8_t *a;
	sbuf_t sbuf;
	opts_parse_data_t *opd = arg;

	if (buf_get_sbuf_upto_sbuf_and_skip(opd->in_buf, &sbuf, cmd->s) < 0)
		return LOOP_CONTINUE;

	if (opd->in_buf->len > opd->arg_buf->size)
		return LOOP_STOP_FAILED;

	a = cmd->args;
	while (a[0] != ARG_TYPE_NONE) {
		if (opts_get_arg(a[0], opd->arg_buf, opd->in_buf) < 0)
			return LOOP_STOP_FAILED;
		a++;
	}
	(*opd->ext_cb)(cmd->cmd, opd->arg_buf);
	return LOOP_STOP_SUCCESS;
}

static int
opts_for_each_cmd(const cmd_t *cmds, uint8_t cmds_len,
		  uint8_t (*cb)(const cmd_t *cmd, void *arg), void *arg)
{
	uint8_t i;

	for (i = 0; i < cmds_len; i++) {
		const cmd_t *cmd = &cmds[i];
		uint8_t ret = (*cb)(cmd, arg);

		switch (ret) {
		case LOOP_STOP_FAILED:
			return -1;
		case LOOP_STOP_SUCCESS:
			return 0;
		}
	}
	return -1;
}

int opts_parse_buf(const cmd_t *cmds, uint8_t cmds_len, buf_t *in,
		   buf_t *args, void (*cb)(uint8_t cmd, buf_t *args))

{
	opts_parse_data_t opd = {
		.in_buf = in,
		.arg_buf = args,
		.ext_cb = cb,
	};

	return opts_for_each_cmd(cmds, cmds_len, opts_parse_buf_cb, &opd);
}

static uint8_t opts_print_usage_cb(const cmd_t *cmd, void *args)
{
	const uint8_t *a = cmd->args;
	uint8_t j;

	LOG("%s", cmd->s->data);
	for (j = 0; j < countof(cmd->args) && *a != ARG_TYPE_NONE; j++) {
		switch (*a) {
		case ARG_TYPE_CHAR:
			LOG(" <char>");
			break;
		case ARG_TYPE_UINT8:
			LOG(" <uint8>");
			break;
		case ARG_TYPE_UINT16:
			LOG(" <uint16>");
			break;
		case ARG_TYPE_INT8:
			LOG(" <int8>");
			break;
		case ARG_TYPE_INT16:
			LOG(" <int16>");
			break;
		case ARG_TYPE_STRING:
			LOG(" <string>");
			break;
		default:
			goto end;
		}
		a++;
	}
 end:
	LOG("\n");
	return LOOP_CONTINUE;
}

void opts_print_usage(const cmd_t *cmds, uint8_t cmds_len)
{
	opts_for_each_cmd(cmds, cmds_len, opts_print_usage_cb, NULL);
}
