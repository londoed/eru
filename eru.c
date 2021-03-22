/**
 * ERU: A simple, lightweight, and portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { main.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <errno.h>

#include "eru.h"

struct Editor eru;

void
eru_error(const char *s)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

void
disable_raw_mode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &eru.orig) == -1)
		eru_error("[!] ERROR: eru: ");
}

void
enable_raw_mode(void)
{
	struct termios raw;

	if (tcgetattr(STDIN_FILENO, &eru.orig) == -1)
		eru_error("[!] ERROR: eru: ");

	atexit(disable_raw_mode);	
	raw = eru.orig;

	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | ISTRIP | INPCK);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		eru_error("[!] ERROR: eru: ");
}

/*void eru_info(void)
{
	int i;

	for (i = 0; i < eru.screen_rows,)
}
*/

void
eru_draw_rows(struct AppendBuffer *ab)
{
	int i;

	for (i = 0; i < eru.screen_rows; i++) {
		if (i == eru.screen_rows / 3) {
			char info[80];
			int info_len = snprintf(info, sizeof(info), "eru -- version %s", ERU_VERSION);

			if (info_len > eru.screen_cols)
				info_len = eru.screen_cols;

			int padding = (eru.screen_cols - info_len) / 2;

			if (padding) {
				abuf_append(ab, "~", 1);
				padding--;
			}

			while (padding--)
				abuf_append(ab, " ", 1);

			abuf_append(ab, info, info_len);
		} else {
			abuf_append(ab, "~", 1);
		}

		abuf_append(ab, "\x1b[K", 3);

		if (i < eru.screen_rows - 1)
			abuf_append(ab, "\r\n", 2);
	}
}

void
eru_clear_screen(void)
{
	struct AppendBuffer ab = ABUF_INIT;

	abuf_append(&ab, "\x1b[?25l", 6);
	abuf_append(&ab, "\x1b[H", 3);

	eru_draw_rows(&ab);;
	abuf_append(&ab, "\x1b[H", 3);
	abuf_append(&ab, "\x1b[?25h", 6);
	
	write(STDOUT_FILENO, ab.b, ab.len);
	abuf_free(&ab);
}

char
eru_read_key(void)
{
	int nread;
	char c;

	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			eru_error("[!] ERROR: eru: ");
	}

	return c;
}

void
abuf_append(struct AppendBuffer *ab, const char *s, int len)
{
	char *new_buf = realloc(ab->buf, ab->len + len);

	if (new_buf == NULL)
		return;

	memcpy(&new_buf[ab->len], s, len);
	ab->buf = new_buf;
	ab->len += len;
}

void
abuf_free(struct AppendBuffer *ab)
{
	free(ab->buf);
}

int
get_window_size(int *rows, int *cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return -1;

		return get_cursor_position(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;

		return 0;
	}
}

int
get_cursor_position(int *rows, int *cols)
{
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return -1;

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
			break;

		if (buf[i] == 'R')
			break;

		i++;
	}

	buf[i] = '\0';
	
	if (buf[0] != '\x1b' || buf[1] != '[')
		return -1;

	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
		return -1;

	return 0;
}

void
eru_process_keypress(void)
{
	char c = eru_read_key();

	switch (c) {
	case CTRL_KEY('q'):
		eru_clear_screen();
		exit(0);
		break;
	}
}

void
eru_init(void)
{
	eru.cur_x = 0;
	eru.cur_y = 0;

	if (get_window_size(&eru.screen_rows, &eru.screen_cols) == -1)
		eru_error("[!] ERROR: eru: ");
}

int
main()
{
	enable_raw_mode();
	eru_init();

	for (;;) {
		eru_clear_screen();
		eru_process_keypress();
	}

	return 0;
}
