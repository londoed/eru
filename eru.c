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
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <errno.h>

#include "eru.h"

//struct Editor editor;
//struct Editor *eru = &editor;
struct Editor eru;

char *c_hl_exts[] = { ".c", ".h", ".cpp", ".cc", ".hpp", NULL };
char *c_hl_keywords[] = {
	"switch", "if", "while", "for", "break", "continue", "return", "else",
	"struct", "union", "typedef", "static", "enum", "class", "case",
	"int_", "long_", "double_", "float_", "char_", "unsigned", "signed_",
	"void_", "bool_", NULL, 
};

struct Syntax hldb[] = {
	{ "c", c_hl_exts, c_hl_keywords, "//", "/*", "*/", HIGHLIGHT_NUMBERS | HIGHLIGHT_STRINGS },
};

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
	eru_select_syntax_highlight();

	FILE *fp = fopen(filename, "r");

	if (!fp)
		eru_error("[!] ERROR: eru: ");

	char *line = NULL;
	size_t line_cap = 0;
	ssize_t line_len;

	while ((line_len = getline(&line, &line_cap, fp)) != -1) {
		while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
			line_len--;
		
		eru_insert_row(eru.num_rows, line, line_len);
	}

	free(line);
	fclose(fp);
	eru.dirty = 0;
}

void
eru_save(void)
{
	if (eru.filename == NULL) {
		eru.filename = eru_prompt("Save file as: %s", NULL);

		if (eru.filename == NULL) {
			eru_set_status_msg("[!] ATTENTION: Save aborted...");

			return;
		}

		eru_select_syntax_highlight();
	}

	int len;
	char *buf = eru_rows_to_string(&len);
	int fd = open(eru.filename, O_RDWR | O_CREAT, 0644);

	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);

				eru.dirty = 0;
				eru_set_status_msg("[!] INFO: eru: %d bytes written to disk!", len);

				return;
			}
		}

		close(fd);
	}

	free(buf);
	eru_set_status_msg("[!] ERROR: eru: Can't save, I/O error: %s", strerror(errno));
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

			char *c = &eru.row[file_row].render[eru.col_offset];
			unsigned char *hl = &eru.row[file_row].highlight[eru.col_offset];
			int curr_color = -1;
			int j;

			for (j = 0; j < len; j++) {
				if (iscntrl(c[j])) {
					char sym = (c[j] <= 26) ? '@' + c[j] : '?';
					abuf_append(ab, "\x1b[7m", 4);
					abuf_append(ab, &sym, 1);
					abuf_append(ab, "\x1b[m", 3);

					if (curr_color != -1) {
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", curr_color);
						abuf_append(ab, buf, clen);
					}
				} else if (hl[j] == HIGHLIGHT_NORMAL) {
					if (curr_color != -1) {
						abuf_append(ab, "\x1b[39m", 5);
						curr_color = -1;
					}

					abuf_append(ab, &c[j], 1);
				} else {
					int color = eru_syntax_colored(hl[j]);

					if (color != curr_color) {
						char buf[16];
						int c_len = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
						abuf_append(ab, buf, c_len);
					}

					abuf_append(ab, &c[j], 1);
				}
			}

			abuf_append(ab, "\x1b[K", 3);
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
	eru_draw_msg_bar(&ab);

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
eru_insert_row(int cur_pos, char *s, size_t len)
{
	if (cur_pos < 0 || cur_pos > eru.num_rows)
		return;
	
	eru.row = realloc(eru.row, sizeof(Row) * (eru.num_rows + 1));
	memmove(&eru.row[cur_pos + 1], &eru.row[cur_pos], sizeof(Row) * (eru.num_rows - cur_pos));

	for (int j = cur_pos + 1; j <= eru.num_rows; j++)
		eru.row[j].idx++;
	
	eru.row[cur_pos].idx = cur_pos;
	eru.row[cur_pos].size = len;
	eru.row[cur_pos].chars = malloc(len + 1);

	memcpy(eru.row[cur_pos].chars, s, len);
	eru.row[cur_pos].chars[len] = '\0';
	eru.row[cur_pos].rsize = 0;
	eru.row[cur_pos].render = NULL;
	eru.row[cur_pos].highlight = NULL;
	eru.row[cur_pos].hl_open_comment = 0;

	eru_update_row(&eru.row[cur_pos]);
	eru.num_rows++;
	eru.dirty++;
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
	eru_update_syntax(row);
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

int
eru_row_renx_to_curx(Row *row, int rx)
{
	int cur_rx = 0;
	int cx;

	for (cx = 0; cx < row->size; cx++) {
		if (row->chars[cx] == '\t')
			cur_rx += (TAB_STOP - 1) - (cur_rx % TAB_STOP);

		cur_rx++;

		if (cur_rx > rx)
			return cx;
	}

	return cx;
}

char *
eru_rows_to_string(int *buf_len)
{
	int total_len = 0;
	int i;

	for (i = 0; i < eru.num_rows; i++)
		total_len += eru.row[i].size + 1;

	*buf_len = total_len;
	char *buf = malloc(total_len);
	char *p = buf;

	for (i = 0; i < eru.num_rows; i++) {
		memcpy(p, eru.row[i].chars, eru.row[i].size);
		p += eru.row[i].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void
eru_row_append_string(Row *row, char *s, size_t len)
{
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';

	eru_update_row(row);
	eru.dirty++;
}

void
eru_draw_status_bar(struct AppendBuffer *ab)
{
	abuf_append(ab, "\x1b[7m", 4);
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.20s -- %d lines %s",
		eru.filename ? eru.filename : "[NO NAME]", eru.num_rows, eru.dirty ? "(modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", eru.syntax ? eru.syntax->filetype :
			"No Filetype", eru.cur_y + 1, eru.num_rows);

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
	abuf_append(ab, "\r\n", 2);
}

void
eru_set_status_msg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(eru.status_msg, sizeof(eru.status_msg), fmt, ap);
	va_end(ap);

	eru.status_msg_time = time(NULL);
}

void
eru_draw_msg_bar(struct AppendBuffer *ab)
{
	abuf_append(ab, "\x1b[K", 3);
	int msg_len = strlen(eru.status_msg);

	if (msg_len > eru.screen_cols)
		msg_len = eru.screen_cols;

	if (msg_len && time(NULL) - eru.status_msg_time < 5)
		abuf_append(ab, eru.status_msg, msg_len);
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
buffer_append(struct Buffer *buf, const char *s, int len)
{
	char *new_buf = realloc(buf->text, buf->len + len);

	if (new_buf == NULL)
		return;

	memcpy(&new_buf[buf->len], s, len);
	buf->text = new_buf;
	buf->len += len;
}

/*
void
buffer_insert(struct Buffer *buf, SDL_Keycode kc)
{
	char *line = &buf->text[buf->y];
	int i;

	for (i = line; i < strlen(line) - 1; i++)'
		line[i] = 
}
*/

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

int
eru_syntax_colored(int hl)
{
	switch (hl) {
	case HIGHLIGHT_ML_COMMENT:
	case HIGHLIGHT_COMMENT:
		return 36;
	case HIGHLIGHT_STRING:
		return 35;

	case HIGHLIGHT_NUMBER:
		return 31;

	case HIGHLIGHT_MATCH:
		return 34;

	case HIGHLIGHT_KEYW1:
		return 33;

	case HIGHLIGHT_KEYW2:
		return 32;

	default:
		return 37;
	}
}

void
eru_select_syntax_highlight(void)
{
	eru.syntax = NULL;

	if (eru.filename == NULL)
		return;

	char *ext = strrchr(eru.filename, '.');

	for (unsigned int i = 0; i < HLDB_ENTRIES; i++) {
		struct Syntax *s = &hldb[i];
		unsigned int j = 0;

		while (s->file_match[j]) {
			int is_ext = (s->file_match[j][0] == '.');

			if ((is_ext && ext && !strcmp(ext, s->file_match[j]) ||
				(!is_ext && strstr(eru.filename, s->file_match[j])))) {
				
				eru.syntax = s;
				int file_row;

				for (file_row = 0; file_row < eru.num_rows; file_row++)
					eru_update_syntax(&eru.row[file_row]);

				return;
			}

			i++;
		}
	}
}

void
eru_update_syntax(Row *row)
{
	row->highlight = realloc(row->highlight, row->rsize);
	memset(row->highlight, HIGHLIGHT_NORMAL, row->rsize);

	if (eru.syntax == NULL)
		return;
	
	char **keywords = eru.syntax->keywords;
	char *scs = eru.syntax->sline_comment_start;
	char *mcs = eru.syntax->mline_comment_start;
	char *mce = eru.syntax->mline_comment_end;
	
	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;
	int mce_len = mce ? strlen(mce) : 0;
	int prev_sep = 1;
	int in_str = 0;
	int in_cmt = (row->idx > 0 && eru.row[row->idx - 1].hl_open_comment);
	int i = 0;

	while (i < row->rsize) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->highlight[i - 1] : HIGHLIGHT_NORMAL;

		if (scs_len && !in_str && !in_cmt) {
			if (!strncmp(&row->render[i], scs, scs_len)) {
				memset(&row->highlight[i], HIGHLIGHT_COMMENT, row->rsize - i);
				break;
			}
		}

		if (mcs_len && mce_len && !in_str) {
			if (in_cmt) {
				row->highlight[i] = HIGHLIGHT_ML_COMMENT;

				if (!strncmp(&row->render[i], mce, mce_len)) {
					memset(&row->highlight[i], HIGHLIGHT_ML_COMMENT, mce_len);
					i += mce_len;
					in_cmt = 0;
					prev_sep = 1;
					continue;
				} else {
					i++;
					continue;
				}
			} else if (!strncmp(&row->render[i], mcs, mcs_len)) {
				memset(&row->highlight[i], HIGHLIGHT_ML_COMMENT, mcs_len);
				i += mcs_len;
				in_cmt = 1;
				continue;
			}
		}

		if (eru.syntax->flags & HIGHLIGHT_STRINGS) {
			if (in_str) {
				row->highlight[i] = HIGHLIGHT_STRING;

				if (c == '\\' && i + 1 < row->rsize) {
					row->highlight[i + 1] = HIGHLIGHT_STRING;
					i += 2;
					continue;
				}

				if (c == in_str)
					in_str = 0;

				i++;
				prev_sep = 1;
				continue;
			} else {
				if (c == '"' || c == '\'') {
					in_str = c;
					row->highlight[i] = HIGHLIGHT_STRING;
					i++;
					continue;
				}
			}
		}
		
		if (eru.syntax->flags & HIGHLIGHT_NUMBERS) {
			if (isdigit(c) && (prev_sep || prev_hl == HIGHLIGHT_NUMBER) ||
				(c == '.' && prev_hl == HIGHLIGHT_NUMBER)) {
			
				row->highlight[i] = HIGHLIGHT_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}

		if (prev_sep) {
			int j;

			for (j = 0; keywords[j]; j++) {
				int klen = strlen(keywords[j]);
				int kw2 = keywords[j][klen - 1] == "_";

				if (kw2)
					klen--;

				if (!strncmp(&row->render[i], keywords[j], klen) && is_separator(row->render[i + klen])) {
					memset(&row->highlight[i], kw2 ? HIGHLIGHT_KEYW2 : HIGHLIGHT_KEYW1, klen);
					i += klen;
					break;
				}
			}

			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
		}
		
		prev_sep = is_separator(c);
		i++;
	}

	int changed = (row->hl_open_comment != in_cmt);
	row->hl_open_comment = in_cmt;

	if (changed && row->idx + 1 < eru.num_rows)
		eru_update_syntax(&eru.row[row->idx + 1]);
}

int
is_separator(int c)
{
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void
eru_process_keypress(void)
{
	int c = eru_read_key();
	static int qt = QUIT_TIMES;

	switch (c) {
	case '\r':
		eru_insert_newline();
		break;

	case CTRL_KEY('q'):
		if (eru.dirty && qt > 0) {
			eru_set_status_msg("[!] WARNING: File has unsaved changes. Press Ctrl-Q %d more times to quit", qt);
			qt--;

			return;
		}

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

	case BACKSPACE:
	case CTRL_KEY('h'):
	case DELETE:
		if (c == DELETE)
			eru_move_cursor(RIGHT);

		eru_del_char();
		break;

	case CTRL_KEY('l'):
	case '\x1b':
		break;

	case CTRL_KEY('s'):
		eru_save();
		break;

	case CTRL_KEY('f'):
		eru_search();
		break;

	default:
		eru_insert_char(c);
		break;
	}

	qt = QUIT_TIMES;
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

	row = (eru.cur_y >= eru.num_rows) ? NULL : &eru.row[eru.cur_y];
	int row_len = row ? row->size : 0;

	if (eru.cur_x > row_len)
		eru.cur_x = row_len;
}

void
eru_row_insert_char(Row *row, int cur_pos, int c)
{
	if (cur_pos < 0 || cur_pos > row->size)
		cur_pos = row->size;

	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[cur_pos + 1], &row->chars[cur_pos], row->size - cur_pos + 1);
	row->size++;
	row->chars[cur_pos] = c;

	eru_update_row(row);
	eru.dirty++;
}

void
eru_row_del_char(Row *row, int cur_pos)
{
	if (cur_pos < 0 || cur_pos >= row->size)
		return;

	memmove(&row->chars[cur_pos], &row->chars[cur_pos + 1], row->size - cur_pos);
	row->size--;

	eru_update_row(row);
	eru.dirty++;
}

void eru_del_char(void)
{
	if (eru.cur_y == eru.num_rows)
		return;

	if (eru.cur_x == 0 && eru.cur_y == 0)
		return;

	Row *row = &eru.row[eru.cur_y];

	if (eru.cur_x > 0) {
		eru_row_del_char(row, eru.cur_x - 1);
		eru.cur_x--;
	} else {
		eru.cur_x = eru.row[eru.cur_y - 1].size;
		eru_row_append_string(&eru.row[eru.cur_y - 1], row->chars, row->size);
		eru_del_row(eru.cur_y);
		eru.cur_y--;
	}
}

void
eru_insert_char(int c)
{
	if (eru.cur_y == eru.num_rows)
		eru_insert_row(eru.num_rows, "", 0);

	eru_row_insert_char(&eru.row[eru.cur_y], eru.cur_x, c);
	eru.cur_x++;
}

void
eru_free_row(Row *row)
{
	free(row->render);
	free(row->chars);
	free(row->highlight);
}

void
eru_del_row(int cur_pos)
{
	if (cur_pos < 0 || cur_pos >= eru.num_rows)
		return;

	eru_free_row(&eru.row[cur_pos]);
	memmove(&eru.row[cur_pos], &eru.row[cur_pos + 1], sizeof(Row) * (eru.num_rows - cur_pos - 1));

	for (int i = cur_pos; i < eru.num_rows - 1; i++)
		eru.row[i].idx--;

	eru.num_rows--;
	eru.dirty++;
}

void
eru_insert_newline(void)
{
	int file_row = eru.row_offset + eru.cur_y;
	int file_col = eru.col_offset + eru.cur_x;
	Row *row = (file_row >= eru.num_rows) ? NULL : &eru.row[file_row];

	if (!row) {
		if (file_row == eru.num_rows) {
			eru_insert_row(file_row, "", 0);
			goto fix_cursor;
		}

		return;
	}

	if (file_col >= row->size)
		file_col = row->size;

	if (file_col == 0) {
		eru_insert_row(file_row, "", 0);
	} else {
		eru_insert_row(file_row + 1, row->chars + file_col, row->size - file_col);
		row = &eru.row[file_row];

		row->chars[file_col] = '\0';
		row->size = file_col;
		eru_update_row(row);
	}
	
fix_cursor:
	if (eru.cur_y == eru.screen_rows - 1)
		eru.row_offset++;
	else
		eru.cur_y++;

	eru.cur_x = 0;
	eru.col_offset = 0;
}

char *
eru_prompt(char *prompt, void (*func)(char *, int))
{
	size_t buf_size = 128;
	char *buf = malloc(buf_size);
	size_t buf_len = 0;
	
	buf[0] = '\0';

	for (;;) {
		eru_set_status_msg(prompt, buf);
		eru_clear_screen();
		int c = eru_read_key();

		if (c == DELETE || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buf_len != 0)
				buf[--buf_len] = '\0';
		} else if  (c == '\x1b') {
			eru_set_status_msg("");

			if (func)
				func(buf, c);

			free(buf);

			return NULL;
		} else if (c == '\r') {
			if (buf_len != 0) {
				eru_set_status_msg("");

				if (func)
					func(buf, c);

				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buf_len == buf_size - 1) {
				buf_size *= 2;
				buf = realloc(buf, buf_size);
			}

			buf[buf_len++] = c;
			buf[buf_len] = '\0';
		}

		if (func)
			func(buf, c);
	}
}

void
eru_search(void)
{
	int saved_cur_x = eru.cur_x;
	int saved_cur_y = eru.cur_y;
	int saved_col_offset = eru.col_offset;
	int saved_row_offset = eru.row_offset;
	char *query = eru_prompt("[!] SEARCH: %s (Use Arrows/Enter, ESC to quit)", eru_search_cb);
	
	if (query) {
		free(query);
	} else {
		eru.cur_x = saved_cur_x;
		eru.cur_y = saved_cur_y;
		eru.col_offset = saved_col_offset;
		eru.row_offset = saved_row_offset;
	}
}

void
eru_search_cb(char *query, int key)
{
	static int last_match = -1;
	static int direction = 1;
	static int saved_hl_line;
	static char *saved_hl = NULL;

	if (saved_hl) {
		memcpy(eru.row[saved_hl_line].highlight, saved_hl, eru.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;
	}

	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;

		return;
	} else if (key == RIGHT || key == DOWN) {
		direction = 1;
	} else if (key == LEFT || key == UP) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1)
		direction = 1;

	int curr = last_match;
	int i;

	for (i = 0; i < eru.num_rows; i++) {
		curr *= direction;

		if (curr == -1)
			curr = eru.num_rows - 1;
		else if (curr == eru.num_rows)
			curr = 0;

		Row *row = &eru.row[curr];
		char *match = strstr(row->render, query);

		if (match) {
			last_match = curr;
			eru.cur_y = curr;
			eru.cur_x = eru_row_renx_to_curx(row, match - row->render);
			eru.row_offset = eru.num_rows;

			saved_hl_line = curr;
			saved_hl = malloc(row->rsize);
			
			memcpy(saved_hl, row->highlight, row->rsize);
			memset(&row->highlight[match - row->render], HIGHLIGHT_MATCH, strlen(query));
			break;
		}
	}
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
	eru.dirty = 0;
	eru.filename = NULL;
	eru.status_msg[0] = '\0';
	eru.status_msg_time = 0;
	eru.row = NULL;
	eru.syntax = NULL;

	if (get_window_size(&eru.screen_rows, &eru.screen_cols) == -1)
		eru_error("[!] ERROR: eru: ");

	eru.screen_rows -= 2;
}

int
main(int argc, char *argv[])
{
	enable_raw_mode();
	eru_init();

	if (argc >= 2)
		eru_open(argv[1]);

	eru_set_status_message("[ERU] Quit: Ctrl+Q | Save: Ctrl+S | Find: Ctrl+F");

	for (;;) {
		eru_clear_screen();
		eru_process_keypress();
	}

	return 0;
}
