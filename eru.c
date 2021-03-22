/**
 * ERU: A simple, lightweight, and portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { main.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
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
eru_open(void)
{
	FILE *fp = fopen(filename, "r");

	if (!fp)
		eru_error("[!] ERROR: eru: ");
	char *line = NULL;
	size_t line_cap = 0;
	ssize_t line_len;

	line_len = getline(&line, &line_cap, fp);

	if (line_len != -1) {
		while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
			line_len--;
		
		eru_append_row(line, line_len);
	}

	free(line);
	fclose(fp);
}

void
eru_draw_rows(struct AppendBuffer *ab)
{
	int i;

	for (i = 0; i < eru.screen_rows; i++) {
		if (i >= eru.num_rows) {
			if (eru.num_rows == 0 && i == eru.screen_rows / 3) {
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
		} else {
			int len = eru.row.size;

			if (len > eru.screen_cols)
				len = eru.screen_cols;

			abuf_append(ab, eru.row.chars, len);
		}
	}
}

void
eru_clear_screen(void)
{
	struct AppendBuffer ab = ABUF_INIT;

	abuf_append(&ab, "\x1b[?25l", 6);
	abuf_append(&ab, "\x1b[H", 3);

	eru_draw_rows(&ab);;
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", eru.cur_y + 1, eru.cur_x + 1);

	abuf_append(&ab, buf, strlen(buf));
	abuf_append(&ab, "\x1b[?25h", 6);
	
	write(STDOUT_FILENO, ab.b, ab.len);
	abuf_free(&ab);
}

int
eru_read_key(void)
{
	int nread;
	char c;

	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			eru_error("[!] ERROR: eru: ");
	}
	
	if (c == '\x1b') {
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\x1b';

		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\x1b';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\x1b';

				if (seq[2] == '~') {
					switch (seq[1]) {
					case '1':
						return HOME;

					case '3':
						return DELETE;

					case '4':
						return END;

					case '5':
						return PAGE_UP;

					case '6':
						return PAGE_DOWN;

					case '7':
						return HOME;

					case '8':
						return END;
					}
				}
			} else {
				switch (seq[1]) {
				case 'A':
					return UP;

				case 'B':
					return DOWN;

				case 'C':
					return RIGHT;

				case 'D':
					return LEFT;

				case 'H':
					return HOME;

				case 'F':
					return END;
				}
			}
		} else if (seq[0] == '0') {
			switch (seq[1]) {
			case 'H':
				return HOME;

			case 'F':
				return END;
			}
		}

		return '\x1b';
	} else {
		return c;
	}
}

void
eru_append_row(char *s, size_t len)
{
	eru.row.size = len;
	eru.row.chars = malloc(len + 1);

	memcpy(eru.row.chars, s, len);
	eru.row.chars[len] = '\0';
	eru.num_rows = 1;
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
	int c = eru_read_key();

	switch (c) {
	case CTRL_KEY('q'):
		eru_clear_screen();
		exit(0);
		break;

	case UP:
	case DOWN:
	case LEFT:
	case RIGHT:
		eru_move_cursor(c);
		break;

	case PAGE_UP:
	case PAGE_DOWN:
		{
			int times = eru.screen_rows;

			while (times--)
				eru_move_cursor(c == PAGE_UP ? UP : DOWN);

			break;
		}

	case HOME:
		eru.cur_x = 0;
		break;

	case END:
		eru.cur_x = eru.screen_cols - 1;
		break;
	}
}

void
eru_move_cursor(int key)
{
	switch (key) {
	case LEFT:
		if (eru.cur_x != 0)
			eru.cur_x--;
		
		break;

	case RIGHT:
		if (eru.cur_x != eru.screen_cols - 1)
			eru.cur_x++;
		
		break;

	case UP:
		if (eru.cur_y != 0)
			eru.cur_y--;
		
		break;

	case DOWN:
		if (eru.cur_y != eru.screen_rows - 1)
			eru.cur_y++;
		
		break;
	}
}

void
eru_init(void)
{
	eru.cur_x = 0;
	eru.cur_y = 0;
	eru.num_rows = 0;
	eru.row = NULL;

	if (get_window_size(&eru.screen_rows, &eru.screen_cols) == -1)
		eru_error("[!] ERROR: eru: ");
}

int
main(int argc, char *argv[])
{
	enable_raw_mode();
	eru_init();

	if (argc >= 2)
		eru_open(argv[1]);

	for (;;) {
		eru_clear_screen();
		eru_process_keypress();
	}

	return 0;
}
