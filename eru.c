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
#include <time.h>
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
eru_open(char *filename)
{
	free(eru.filename);
	eru.filename = strdup(filename);

	FILE *fp = fopen(filename, "r");

	if (!fp)
		eru_error("[!] ERROR: eru: ");

	char *line = NULL;
	size_t line_cap = 0;
	ssize_t line_len;

	while ((line_len = getline(&line, &line_cap, fp)) != -1) {
		while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
			line_len--;
		
		eru_append_row(line, line_len);
	}

	free(line);
	fclose(fp);
}

void
eru_scroll(void)
{
	eru.ren_x = 0;

	if (eru.cur_y < eru.num_rows)
		eru.ren_x = eru_row_curx_to_renx(&eru.row[eru.cur_y], eru.cur_x);

	if (eru.cur_y < eru.row_offset)
		eru.row_offset = eru.cur_y;

	if (eru.cur_y >= eru.row_offset + eru.screen_rows)
		eru.row_offset = eru.cur_y - eru.screen_rows + 1;

	if (eru.ren_x < eru.col_offset)
		eru.col_offset = eru.ren_x;

	if (eru.ren_x >= eru.col_offset + eru.screen_cols)
		eru.col_offset = eru.ren_x - eru.screen_cols + 1;
}

void
eru_draw_rows(struct AppendBuffer *ab)
{
	int i;

	for (i = 0; i < eru.screen_rows; i++) {
		int file_row = i + eru.row_offset;
		if (file_row >= eru.num_rows) {
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
			int len = eru.row[file_row].rsize - eru.col_offset;

			if (len < 0)
				len = 0;

			if (len > eru.screen_cols)
				len = eru.screen_cols;

			abuf_append(ab, &eru.row[file_row].render[eru.col_offset], len);
		}

		abuf_append(ab, "\x1b[K", 3);
		abuf_append(ab, "\r\n", 2);
	}
}

void
eru_clear_screen(void)
{
	struct AppendBuffer ab = ABUF_INIT;

	eru_scroll();

	abuf_append(&ab, "\x1b[?25l", 6);
	abuf_append(&ab, "\x1b[H", 3);

	eru_draw_rows(&ab);
	eru_draw_status_bar(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (eru.cur_y - eru.row_offset) + 1, 
		(eru.ren_x - eru.col_offset) + 1);

	abuf_append(&ab, buf, strlen(buf));
	abuf_append(&ab, "\x1b[?25h", 6);
	
	write(STDOUT_FILENO, ab.buf, ab.len);
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
	eru.row = realloc(eru.row, sizeof(Row) * (eru.num_rows + 1));
	int cur_row = eru.num_rows;

	eru.row[cur_row].size = len;
	eru.row[cur_row].chars = malloc(len + 1);

	memcpy(eru.row[cur_row].chars, s, len);
	eru.row[cur_row].chars[len] = '\0';
	eru.row[cur_row].rsize = 0;
	eru.row[cur_row].render = NULL;

	eru_update_row(&eru.row[cur_row]);
	eru.num_rows++;
}

void
eru_update_row(Row *row)
{
	int i, idx = 0, tabs = 0;
	
	for (i = 0; i < row->size; i++) {
		if (row->chars[i] == '\t')
			tabs++;
	}

	free(row->render);
	row->render = malloc(row->size + tabs * (TAB_STOP - 1) + 1);

	for (i = 0; i < row->size; i++) {
		if (row->chars[i] == '\t') {
			row->render[idx++] = ' ';

			while (idx % TAB_STOP != 0)
				row->render[idx++] = ' ';
		} else {
			row->render[idx++] = row->chars[i];
		}
	}

	row->render[idx] = '\0';
	row->rsize = idx;
}

int
eru_row_curx_to_renx(Row *row, int cx)
{
	int rx = 0, i;

	for (i = 0; i < cx; i++) {
		if (row->chars[i] == '\t')
			rx += (TAB_STOP - 1) - (rx % TAB_STOP);

		rx++;
	}

	return rx;
}

void
eru_draw_status_bar(struct AppendBuffer *ab)
{
	abuf_append(ab, "\x1b[7m", 4);
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.20s -- %d lines",
		eru.filename ? eru.filename : "[NO NAME]", eru.num_rows);
	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", eru.cur_y + 1, eru.num_rows);

	while (len < eru.screen_cols) {
		if (eru.screen_cols - len == rlen) {
			abuf_append(ab, " ", 1);
			break;
		} else {
			abuf_append(ab, " ", 1);
			len++;
		}
	}

	abuf_append(ab, "\x1b[m", 3);
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
			if (c == PAGE_UP)
				eru.cur_y = eru.row_offset;
			else if (c == PAGE_DOWN)
				eru.cur_y = eru.row_offset + eru.screen_rows - 1;

			if (eru.cur_y > eru.num_rows)
				eru.cur_y = eru.num_rows;

			int times = eru.screen_rows;

			while (times--)
				eru_move_cursor(c == PAGE_UP ? UP : DOWN);

			break;
		}

	case HOME:
		eru.cur_x = 0;
		break;

	case END:
		if (eru.cur_y < eru.num_rows)
			eru.cur_x = eru.row[eru.cur_y].size;
		break;
	}
}

void
eru_move_cursor(int key)
{
	Row *row = (eru.cur_y >= eru.num_rows) ? NULL : &eru.row[eru.cur_y];
	switch (key) {
	case LEFT:
		if (eru.cur_x != 0) {
			eru.cur_x--;
		} else if (eru.cur_y > 0) {
			eru.cur_y--;
			eru.cur_x = eru.row[eru.cur_y].size;
		}
		
		break;

	case RIGHT:
		if (row && eru.cur_x < row->size) {
			eru.cur_x++;
		} else if (row && eru.cur_x == row->size) {
			eru.cur_y++;
			eru.cur_x = 0;
		}
		
		break;

	case UP:
		if (eru.cur_y != 0)
			eru.cur_y--;
		
		break;

	case DOWN:
		if (eru.cur_y < eru.num_rows)
			eru.cur_y++;
		
		break;
	}

	row = (eru.cur_y >= eru.num_rows) ? NULL : eru.row[eru.cur_y];
	int row_len = row ? row->size : 0;

	if (eru.cur_x > row_len)
		eru.cur_x = row_len;
}

void
eru_init(void)
{
	eru.cur_x = 0;
	eru.cur_y = 0;
	eru.ren_x = 0;
	eru.row_offset = 0;
	eru.col_offset = 0;
	eru.num_rows = 0;
	eru.filename = NULL;
	eru.status_msg[0] = '\0';
	eru.status_msg_time = 0;
	eru.row = NULL;

	if (get_window_size(&eru.screen_rows, &eru.screen_cols) == -1)
		eru_error("[!] ERROR: eru: ");

	eru.screen_rows--;
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
