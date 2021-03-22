/**
 * ERU: A simple, lightweight, and portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { eru.h }.
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

#define ERU_VERSION "0.0.2"
#define TAB_STOP 8
#define QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1F)
#define ABUF_INIT {NULL, 0}

enum eru_key {
	BACKSPACE = 127,
	LEFT = 1000,
	RIGHT,
	UP,
	DOWN,
	PAGE_UP,
	PAGE_DOWN,
	HOME,
	END,
	DELETE,
};

typedef struct Row {
	int size, rsize;
	char *chars;
	char *render;
} Row;

struct Editor {
	struct termios orig;
	int cur_x, cur_y;
	int ren_x;
	int screen_rows, screen_cols;
	int num_rows;
	int row_offset;
	int col_offset;
	int dirty;
	char filename;
	char status_msg[80];
	time_t status_msg_time;
	Row *row;
};

struct AppendBuffer {
	char *buf;
	int len;
};

void eru_error(const char *);
void disable_raw_mode(void);
void enable_raw_mode(void);

void eru_open(char *);
void eru_save(void);
void eru_scroll(void);
void eru_draw_rows(AppendBuffer *);
void eru_clear_screen(void);
void eru_read_key(void);

void eru_insert_row(int, char *, size_t len);
void eru_update_row(Row *);
int eru_row_curx_to_renx(Row *, int);
int eru_row_renx_to_curx(Row *, int);
char *eru_rows_to_string(int *);
void eru_row_append_string(Row *, char *, size_t);

void eru_draw_status_bar(struct AppendBuffer *);
void eru_set_status_msg(const char *, ...);
void eru_draw_msg_bar(struct AppendBuffer *);

void abuf_append(struct AppendBuffer *, const char *, int);
void abuf_free(struct AppendBuffer *);

int get_window_size(int *, int *);
int get_cursor_position(int *, int *);
void eru_process_keypress(void);
void eru_move_cursor(char);

void eru_row_insert_char(Row *, int, int);
void eru_row_del_char(Row *, int);
void eru_del_char(void);
void eru_insert_char(int);

void eru_free_row(Row);
void eru_del_row(int)
void eru_insert_newline(void);

char *eru_prompt(char *);
void eru_search(void);
void eru_init(void);
