#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "kbd-layout-us-reference.h"
#include "modifiers.h"

typedef struct kbd_ctx {
	unsigned char pressed_keys[6];
	unsigned char prev_key;
	int pressed_keys_count;
	int pressed_keys_pos;
} kbd_ctx_t;

static int kbd_us_pos;

static int fd;
static int oldkbmode;
struct termios old;
static char *lang;

static const char *conspath[] = {
	"/proc/self/fd/0",
	"/dev/tty",
	"/dev/tty0",
	"/dev/vc/0",
	"/dev/systty",
	"/dev/console",
	NULL
};

static int is_a_console(int fd)
{
	char arg;

	arg = 0;
	return (isatty(fd) && ioctl(fd, KDGKBTYPE, &arg) == 0 &&
		((arg == KB_101) || (arg == KB_84)));
}

static int open_a_console(const char *fnam)
{
	int fd;

	/*
	 * For ioctl purposes we only need some fd and permissions
	 * do not matter. But setfont:activatemap() does a write.
	 */
	fd = open(fnam, O_RDWR);
	if (fd < 0)
		fd = open(fnam, O_WRONLY);
	if (fd < 0)
		fd = open(fnam, O_RDONLY);
	if (fd < 0)
		return -1;
	return fd;
}

static int getfd(const char *fnam)
{
	int fd, i;

	if (fnam) {
		if ((fd = open_a_console(fnam)) >= 0) {
			if (is_a_console(fd))
				return fd;
			close(fd);
		}
		fprintf(stderr, "Couldn't open %s", fnam);
		fprintf(stderr, "\n");
		exit(1);
	}

	for (i = 0; conspath[i]; i++) {
		if ((fd = open_a_console(conspath[i])) >= 0) {
			if (is_a_console(fd))
				return fd;
			close(fd);
		}
	}

	for (fd = 0; fd < 3; fd++)
		if (is_a_console(fd))
			return fd;

	fprintf(stderr, "Couldn't get a file descriptor referring "
		"to the console. Are you root?");
	fprintf(stderr, "\n");

	/* total failure */
	exit(1);
}

static int kbmode(int fd) {
	int mode;

	if (ioctl(fd, KDGKBMODE, &mode) < 0)
		return -1;
	else
		return mode;
}

/* Get a file descriptor suitable for setting the keymap.  Anything not in
 * raw mode will do - it doesn't have to be the foreground console - but we
 * prefer Unicode if we can get it.
 */
int getfd_keymap(void) {
	int fd, bestfd = -1;
	char ttyname[sizeof("/dev/ttyNN")] = "/dev/tty";
	int i;

#define CHECK_FD_KEYMAP do {						\
		if (is_a_console(fd)) {					\
			int mode = kbmode(fd);				\
			if (mode == K_UNICODE) {			\
				if (bestfd != -1)			\
					close(bestfd);			\
				return fd;				\
			} else if (mode == K_XLATE && bestfd == -1)	\
				bestfd = fd;				\
			else						\
				close(fd);				\
		} else							\
			close(fd);					\
	} while (0)

	for (i = 0; conspath[i]; i++) {
		if ((fd = open_a_console(conspath[i])) >= 0)
			CHECK_FD_KEYMAP;
	}

	for (i = 1; i <= 12; ++i) {
		snprintf(ttyname + sizeof("/dev/tty") - 1, 3, "%d", i);
		if ((fd = open_a_console(ttyname)) >= 0)
			CHECK_FD_KEYMAP;
	}

	fprintf(stderr,
		"Couldn't get a file descriptor referring to the console\n");

	/* total failure */
	exit(1);
}

/*
 * version 0.81 of showkey would restore kbmode unconditially to XLATE,
 * thus making the console unusable when it was called under X.
 */
static void get_mode(void)
{
	const char *m;

	if (ioctl(fd, KDGKBMODE, &oldkbmode))
		fprintf(stderr, "Unable to read keyboard mode\n");

	switch (oldkbmode) {
		case K_RAW:
			m = "RAW";
			break;
		case K_XLATE:
			m = "XLATE";
			break;
		case K_MEDIUMRAW:
			m = "MEDIUMRAW";
			break;
		case K_UNICODE:
			m = "UNICODE";
			break;
		default:
			m = "?UNKNOWN?";
			break;
	}
	printf("kb mode was %s\n", m);
	if (oldkbmode != K_XLATE) {
		printf("[ if you are trying this under X, it might not work\n"
		       "since the X server is also reading /dev/console ]\n");
	}
	printf("\n");
}

static void clean_up(void)
{
	if (ioctl(fd, KDSKBMODE, oldkbmode))
		fprintf(stderr, "ioctl KDSKBMODE\n");
	if (tcsetattr(fd, 0, &old) == -1)
		fprintf(stderr, "tcsetattr");
	close(fd);
}

static void die(int x)
{
	printf("caught signal %d, cleaning up...\n", x);
	clean_up();
	exit(EXIT_FAILURE);
}

static int kbd_get_modifier(int keycode)
{
	switch (keycode) {
	case MODIFIER_ALT:
		return HID_KEYBOARD_SC_RIGHT_ALT;
	case MODIFIER_ALT_GT:
		return HID_KEYBOARD_SC_LEFT_ALT;
	case MODIFIER_CTL_RIGHT:
		return HID_KEYBOARD_SC_RIGHT_CONTROL;
	case MODIFIER_CTL_LEFT:
		return HID_KEYBOARD_SC_LEFT_CONTROL;
	case MODIFIER_SHIFT_LEFT:
		return HID_KEYBOARD_SC_LEFT_SHIFT;
	case MODIFIER_SHIFT_RIGHT:
		return HID_KEYBOARD_SC_RIGHT_SHIFT;
	case MODIFIER_WIN:
		return HID_KEYBOARD_SC_RIGHT_GUI;
	default:
		return -1;
	}
}

static int
kbd_get_layout(kbd_ctx_t *kbd_ctx, key_map_t *kbd_layout, unsigned char *buf,
	       int n)
{
	int i = 0;

	while (i < n) {
		int kc;
		int bufi = buf[i];

		if (bufi == kbd_ctx->prev_key) {
			i++;
			continue;
		}
		kbd_ctx->prev_key = bufi;

		if (i + 2 < n &&
		    (buf[i] & 0x7f) == 0 &&
		    (buf[i + 1] & 0x80) != 0 &&
		    (buf[i + 2] & 0x80) != 0) {
			kc = ((buf[i + 1] & 0x7f) << 7) |
				(buf[i + 2] & 0x7f);
			i += 3;
		} else {
			kc = (buf[i] & 0x7f);
			i++;
		}
		if (!(bufi & 0x80)) {
			/* key pressed */
			kbd_ctx->pressed_keys[kbd_ctx->pressed_keys_count++] = kc;
			kbd_ctx->pressed_keys_pos++;
			continue;
		}
		/* key released */
		if (--kbd_ctx->pressed_keys_count)
			continue;

		if (kbd_ctx->pressed_keys_pos <= 1)
			kbd_layout[kbd_us_pos].modifier = 0;
		else
			kbd_layout[kbd_us_pos].modifier =
				kbd_get_modifier(kbd_ctx->pressed_keys[0]);
		kbd_layout[kbd_us_pos].c = kbd_layout_us[kbd_us_pos].c;
		kbd_layout[kbd_us_pos].code = kbd_layout_us[kbd_us_pos].code;

		/* for (a = 0; a < kbd_ctx->pressed_keys_pos; a++) */
		/* 	printf("%d ", kbd_ctx->pressed_keys[a]); */
		puts("");
		kbd_ctx->pressed_keys_pos = 0;

		if (++kbd_us_pos > sizeof(kbd_layout_us) / sizeof(key_map_t))
			return -1;
		printf("\npress %c ", kbd_layout_us[kbd_us_pos].c);
		fflush(stdout);
	}
	return 0;
}

static void kbd_print_layout(key_map_t *kbd_layout)
{
	int i;

	printf("#include \"lufa-key-codes.h\"\n\n"
	       "static key_map_t kbd_layout_%s[] = {\n", lang);

	for (i = 0; i < sizeof(kbd_layout_us) / sizeof(key_map_t); i++) {
		printf("\t{'");
		if (kbd_layout[i].c == '\'' || kbd_layout[i].c == '\\')
			printf("\\");
		printf("%c', 0x%x, 0x%x},\n", kbd_layout[i].c,
		       kbd_layout[i].modifier, kbd_layout[i].code);
	}
	printf("};\n");
}
static void init_signals(void)
{
	signal(SIGHUP, die);
	signal(SIGINT, die);
	signal(SIGQUIT, die);
	signal(SIGILL, die);
	signal(SIGTRAP, die);
	signal(SIGABRT, die);
	signal(SIGIOT, die);
	signal(SIGFPE, die);
	signal(SIGKILL, die);
	signal(SIGUSR1, die);
	signal(SIGSEGV, die);
	signal(SIGUSR2, die);
	signal(SIGPIPE, die);
	signal(SIGTERM, die);
#ifdef SIGSTKFLT
	signal(SIGSTKFLT, die);
#endif
	signal(SIGCHLD, die);
	signal(SIGCONT, die);
	signal(SIGSTOP, die);
	signal(SIGTSTP, die);
	signal(SIGTTIN, die);
	signal(SIGTTOU, die);
}

int main(int argc, char *argv[])
{
	struct termios new = { 0 };
	unsigned char buf[18]; /* divisible by 3 */
	key_map_t kbd_layout[sizeof(kbd_layout_us) / sizeof(key_map_t)];
	kbd_ctx_t kbd_ctx;

	if (argc != 2) {
		printf("usage: %s [lang]\nExample: %s fr\n"
		       "\t%s pl\n", argv[0], argv[0], argv[0]);
		return -1;
	}
	lang = argv[1];
	memset(&kbd_ctx, 0, sizeof(kbd_ctx_t));
	setlocale(LC_ALL, "");
	init_signals();

	if ((fd = getfd(NULL)) < 0)
		fprintf(stderr, "couldn't get a file descriptor referring "
			"to the console. Are you root?\n");

	get_mode();
	if (tcgetattr(fd, &old) == -1)
		fprintf(stderr, "tcgetattr\n");
	if (tcgetattr(fd, &new) == -1)
		fprintf(stderr, "tcgetattr\n");

	new.c_lflag &= ~((tcflag_t)(ICANON | ECHO | ISIG));
	new.c_iflag     = 0;
	new.c_cc[VMIN]  = sizeof(buf);
	new.c_cc[VTIME] = 1; /* 0.1 sec intercharacter timeout */

	if (tcsetattr(fd, TCSAFLUSH, &new) == -1)
		fprintf(stderr, "tcsetattr\n");
	if (ioctl(fd, KDSKBMODE, K_MEDIUMRAW))
		fprintf(stderr, "ioctl KDSKBMODE\n");

	printf("\npress %c ", kbd_layout_us[kbd_us_pos].c);
	fflush(stdout);

	while (1) {
		int n = read(fd, buf, sizeof(buf));

		if (kbd_get_layout(&kbd_ctx, kbd_layout, buf, n) < 0)
			break;
	}

	kbd_print_layout(kbd_layout);

	clean_up();
	return EXIT_SUCCESS;
}
