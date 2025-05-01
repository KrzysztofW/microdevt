/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2024, Krzysztof Witek
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/buf.h>
#include "payload-script.h"
#include "kbd-layout-us-reference.h"
#include "kbd-layout-fr.h"

static char *filename;
static char *output_filename;
#define MAX_LINE_LENGTH 1024
#define SCRIPT_MAX_LENGTH 8192
static uint8_t payload_data[SCRIPT_MAX_LENGTH];
static uint8_t string_data[SCRIPT_MAX_LENGTH];
static buf_t payload = BUF_INIT_BIN(payload_data);
static buf_t string = BUF_INIT_BIN(string_data);
static int line_nb = 1;
static char *keyboard_layout;
typedef enum keyboard_layout_enum {
	KBD_ENGLISH_US,
	KBD_FRENCH_FR,
} keyboard_layout_t;
static keyboard_layout_t keyboard_layout_code;
static key_map_t *kbd_layout;

typedef struct block_state {
	uint8_t rem : 1;
	uint8_t string : 1;
	uint8_t stringln : 1;
} block_state_t;

static void usage(char *progname)
{
	printf("%s: -f filename -l keyboard_layout_code -o payload_file "
	       "[ -s list supported keyboards]\n",
	       progname);
}

static int parse_opts(int argc, char *argv[])
{
	int opt;
	const char *opt_string = "f:hl:so:";

	while ((opt = getopt(argc, argv, opt_string)) != -1) {
		switch (opt) {
		case 'f':
			filename = optarg;
			break;
		case 'l':
			keyboard_layout = optarg;
			break;
		case 's':
			printf("supported keyboards:\n"
			       "en_us - English, US\n"
			       "fr_fr - French, France\n");
			return -1;
		case 'o':
			output_filename = optarg;
			break;
		case 'h':
		default:
			usage(argv[0]);
			return -1;
		}
	}

	if (filename == NULL || output_filename == NULL ||
	    keyboard_layout == NULL) {
		usage(argv[0]);
		return -1;
	}
	return 0;
}

static void expect(int val, int expect, const char *msg)
{
	if (val == expect)
		return;
	fprintf(stderr, "line %d: %s\n", line_nb, msg);
	exit(-1);
}

static int
kbd_lookup(char c, int *modifier, int *scan_code, const key_map_t *kbd_layout)
{
	int i;

	for (i = 0; i < sizeof(kbd_layout_us) / sizeof(key_map_t); i++) {
		if (kbd_layout[i].c == c) {
			*scan_code = kbd_layout[i].code;
			*modifier = kbd_layout[i].modifier;
			return 0;
		}
	}
	return -1;
}

static int parse_string(sbuf_t *str)
{
	while (str->len && str->data[0] != '\0') {
		char c = str->data[0];
		int code, modifier;

		__sbuf_skip(str, 1);

		/* Check if we have at least 2 bytes available in the
		 * payload buffer.
		 */
		if (buf_get_free_space(&payload) < 2)
			return -1;

		/* if (isupper(c)) { */
		/* 	__buf_addc(&payload, HID_KEYBOARD_SC_LEFT_SHIFT); */
		/* 	c = tolower(c); */
		/* } */

		/* Direct mapping between ASCII codes and
		 * HID_KEYBOARD_SC codes.
		 */
		/* if (c >= 97 && c <= 122) */
		/* 	__buf_addc(&payload, c - 93); */

		if (kbd_lookup(c, &modifier, &code, kbd_layout) < 0) {
			fprintf(stderr,
				"invalid character: %c (0x%02X)\n",
				c, c);
			return -1;
		}
		if (modifier)
			__buf_addc(&payload, modifier);
		__buf_addc(&payload, code);

	/* 	switch (c) { */
	/* 	case '!': */
	/* 		switch (keyboard_layout) { */
	/* 		case KBD_ENGLISH_US: */
	/* 			__buf_addc(&payload, */
	/* 				   HID_KEYBOARD_SC_LEFT_SHIFT); */
	/* 			code = HID_KEYBOARD_SC_1_AND_EXCLAMATION; */
	/* 			break; */
	/* 		case KBD_FRENCH_FR: */
	/* 			code = HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK; */
	/* 			break; */
	/* 		} */
	/* 		__buf_addc(&payload, code); */
	/* 		break; */
	/* 	case '@': */
	/* 		switch (keyboard_layout) { */
	/* 		case KBD_ENGLISH_US: */
	/* 			__buf_addc(&payload, */
	/* 				   HID_KEYBOARD_SC_LEFT_SHIFT); */
	/* 			code = HID_KEYBOARD_SC_2_AND_AT; */
	/* 			break; */
	/* 		case KBD_FRENCH_FR: */
	/* 			__buf_addc(&payload, */
	/* 				   HID_KEYBOARD_SC_RIGHT_ALT); */
	/* 			code = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS; */
	/* 			break; */
	/* 		} */
	/* 		__buf_addc(&payload, code); */
	/* 		break; */
	/* 	case '#': */
	/* 		switch (keyboard_layout) { */
	/* 		case KBD_ENGLISH_US: */
	/* 			__buf_addc(&payload, */
	/* 				   HID_KEYBOARD_SC_LEFT_SHIFT); */
	/* 			code = HID_KEYBOARD_SC_2_AND_AT; */
	/* 			break; */
	/* 		case KBD_FRENCH_FR: */
	/* 			__buf_addc(&payload, */
	/* 				   HID_KEYBOARD_SC_RIGHT_ALT); */
	/* 			code = HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS; */
	/* 			break; */
	/* 		} */
	/* 		__buf_addc(&payload, code); */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */
	/* 	case '': */
	/* 		break; */

	/* 	} */
	}
	return 0;
}

static int parse_line(sbuf_t *line, block_state_t *block_state)
{
	int cmd_start = 0;
	buf_t cmd = BUF(128);
	//sbuf_t end_rem = SBUF_INITS("END_REM\n");

	if (strncmp(line->data, "END_REM\n", strlen("END_REM\n")) == 0) {
		expect(block_state->rem, 1, "REM block not opened");
		block_state->rem = 0;
		return 0;
	}
	if (block_state->rem)
		return 0;
	if (strncmp(line->data, "REM_BLOCK\n", strlen("REM_BLOCK\n")) == 0) {
		expect(block_state->rem, 0, "REM block already opened");
		block_state->rem = 1;
		return 0;
	}

	if (strncmp(line->data, "END_STRING\n",
		    strlen("END_STRING\n")) == 0) {
		sbuf_t tmp;

		expect(block_state->string, 1,
		       "STRING block not opened");
		block_state->string = 0;
		tmp = buf2sbuf(&string);
		buf_reset(&string);
		return parse_string(&tmp);
	}

	if (strncmp(line->data, "END_STRINGLN\n",
		    strlen("END_STRINGLN\n")) == 0) {
		sbuf_t tmp;

		expect(block_state->stringln, 1,
		       "STRINGLN block not opened");
		block_state->stringln = 0;
		tmp = buf2sbuf(&string);
		buf_reset(&string);
		return parse_string(&tmp);
	}
	if (strncmp(line->data, "STRINGLN\n", strlen("STRINGLN\n")) == 0) {
		expect(block_state->string, 0, "STRINGLN block already opened");
		block_state->stringln = 1;
		buf_reset(&string);
		return 0;
	}

	if (block_state->stringln) {
		expect(buf_addsbuf(&string, line), 0, "line too long");
		return 0;
	}

	if (strncmp(line->data, "STRING\n", strlen("STRING\n")) == 0) {
		expect(block_state->string, 0, "STRING block already opened");
		block_state->string = 1;
		buf_reset(&string);
		return 0;
	}

	if (block_state->string) {
		expect(buf_addsbuf(&string, line), 0, "line too long");

		/* strip the trailing \n character */
		if (string.data[string.len - 2] == '\n')
			string.len--;
		return 0;
	}
	if (strncmp(line->data, "STRINGLN ", strlen("STRINGLN ")) == 0) {
		__sbuf_skip(line, strlen("STRINGLN "));
		return parse_string(line);
	}
	if (strncmp(line->data, "STRING ", strlen("STRING ")) == 0) {
		__sbuf_skip(line, strlen("STRING "));

		/* strip the trailing \n character */
		if (line->data[line->len - 2] == '\n')
			line->len--;
		return parse_string(line);
	}
	if (strncmp(line->data, "REM ", strlen("REM ")) == 0)
		return 0;

	if (line->data[line->len - 2] == '\n')
		return 0;

	return -1;
}

static int dos2unix(sbuf_t *line, buf_t *out)
{
	int i;

	if (out->size < line->len) {
		fprintf(stderr, "line too long: %d\n", line_nb);
		return -1;
	}

	for (i = 0; i < line->len; i++) {
		if (line->data[i] == '\r')
			continue;
		__buf_addc(out, line->data[i]);
	}
	return 0;
}

int write_payload(void)
{
	FILE *file = fopen(output_filename, "w");
	int ret;
	script_t script = {
		.magic = PAYLOAD_DATA_MAGIC,
		.size = payload.len,
		//.checksum = cksum(payload.data, payload.len);
	};

	if (file == NULL) {
		fprintf(stderr, "cannot write to file: %s\n", output_filename);
		return -1;
	}
	ret = fwrite(&script, 1, sizeof(script), file);
	if (ret != sizeof(script))
		goto error;

	ret = fwrite(payload.data, 1, payload.len, file);
	if (ret != payload.len)
		goto error;
	return 0;

 error:
	fprintf(stderr, "failed writing to file '%s`\n",
		output_filename);
	return -1;
}

int main(int argc, char **argv)
{
	FILE *script;
	char line[MAX_LINE_LENGTH];
	block_state_t block_state;
	buf_t line_buf = BUF(MAX_LINE_LENGTH);
	char *s;

	if (parse_opts(argc, argv) < 0)
		return -1;

	if (strcmp(keyboard_layout, "en_us") == 0) {
		keyboard_layout_code = KBD_ENGLISH_US;
		kbd_layout = kbd_layout_us;
	} else if (strcmp(keyboard_layout, "fr_fr") == 0) {
		keyboard_layout_code = KBD_FRENCH_FR;
		kbd_layout = kbd_layout_fr;
	} else {
		fprintf(stderr, "unsupported keyboard layout: %s\n",
			keyboard_layout);
		return -1;
	}
	if ((script = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "cannot open file `%s`: %p\n", filename,
			script);
		return -1;
	}

	memset(&block_state, 0, sizeof(block_state));

	while ((s = fgets(line, MAX_LINE_LENGTH, script)) != NULL) {
		sbuf_t line_sbuf;

		sbuf_init(&line_sbuf, line, strlen(line));
		if (dos2unix(&line_sbuf, &line_buf) < 0) {
			fprintf(stderr, "line too long: %d\n", line_nb);
			return -1;
		}
		if (buf_addc(&line_buf, '\0') < 0) {
			fprintf(stderr, "line too long: %d\n", line_nb);
			return -1;
		}
		line_sbuf = buf2sbuf(&line_buf);
		if (parse_line(&line_sbuf, &block_state) < 0) {
			fprintf(stderr, "Cannot parse the script. "
				"Syntax error at line %d\n", line_nb);
			return -1;
		}
	end:
		buf_reset(&line_buf);
		line_nb++;
	}
	return write_payload();
}
