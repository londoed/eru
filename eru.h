/**
 * ERU: A simple, lightweight, and portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { eru.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef ERU_H
#define ERU_H

//#define _DEFAULT_SOURCE
//#define _BSD_SOURCE
//#define _GNU_SOURCE

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

#include "history.h"
#include "point.h"

#define ERU_VERSION "0.0.5"
#define TAB_STOP 8
#define BUFFER_NAME_MAX 16
#define QUIT_TIMES 3
#define DEBUG_MODE 1
#define HIGHLIGHT_NUMBERS (1 << 0)
#define HIGHLIGHT_STRINGS (1 << 1)
#define HLDB_ENTRIES (sizeof(hldb) / sizeof(hldb[0]))

#define CTRL_KEY(k) ((k) & 0x1F)
#define ABUF_INIT {NULL, 0}

const char *BLACK 									= "\x1b[30m";
const char *RED 										= "\x1b[31m";
const char *GREEN 									= "\x1b[32m";
const char *YELLOW 									= "\x1b[33m";
const char *BLUE 										= "\x1b[34m";
const char *MAGENTA 								= "\x1b[35m"
const char *CYAN 										= "\x1b[36m";
const char *WHITE 									= "\x1b[37m";
const char *BBLACK 									= "\x1b[90m";
const char *BRED 										= "\x1b[91m";
const char *BGREEN 									= "\x1b[92m";
const char *BYELLOW 								= "\x1b[93m";
const char *BBLUE										= "\x1b[94m";
const char *BMAGENTA								= "\x1b[95m";
const char *BCYAN										= "\x1b[96m";
const char *BWHITE									= "\x1b[97m";

const char *TERM_RESET							= "\x1b[m";
const char *TERM_RESET_FG						= "\xb1[39m";
const char *TERM_INVERT							= "\x1b[7m";

const char *TERM_CLEAR_SCREEN 			= "\x1b[2J";
const char *TERM_CLEAR_ROW					= "\x1b[K";
const char *TERM_HIDE_CUR						= "\x1b[?25L";
const char *TERM_SHOW_CUR						= "\x1b[?25h";
const char *TERM_MOVE_CUR_DEFAULT 	= "\x1b[H";
const char *TERM_QUERY_CUR_POS			= "\x1b[6n";

enum eru_key {
	SPACE = 32,
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

enum eru_highlight {
	HIGHLIGHT_NORMAL = 0,
	HIGHLIGHT_COMMENT,
	HIGHLIGHT_ML_COMMENT,
	HIGHLIGHT_STRING,
	HIGHLIGHT_NUMBER,
	HIGHLIGHT_MATCH,
	HIGHLIGHT_KEYW1,
	HIGHLIGHT_KEYW2,
};

enum editor_mode {
	MODE_NORMAL,
	MODE_INSERT,
};

typedef struct Row {
	int idx;
	int size, rsize;
	int hl_open_comment;
	char *chars;
	char *render;
	unsigned char *highlight;
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
	int mode;
	char filename;
	char status_msg[80];
	time_t status_msg_time;
	struct Syntax *syntax;
	struct Editor *prev;
	struct Editor *next;
	Row *row;
};

struct AppendBuffer {
	char *buf;
	int len;
};

struct Syntax {
	char *filetype;
	char **file_match;
	char *sline_comment_start;
	char *mline_comment_start;
	char *mline_comment_end;
	char **keywords;
	int flags;
};

typedef struct Buffer {
	Buffer *next_chain_entry;
	char buf_name[BUFFER_NAME_MAX];
	Point point;

	int cur_line;
	int num_chars;
	int num_lines;
	struct Mark *mark_list;
	struct Storage *contents;

	char filename[FILENAME_MAX];
	time_t file_time;
	struct Mode *mode_list;
} Buffer;

struct World {
	Buffer *buffer_chain;
	Buffer *cur_buf;
};

struct Mark {
	struct Mark *next;
	char *name;
	Point loc;
	bool is_fixed;
};

void eru_error(const char *);
void disable_raw_mode(void);
void enable_raw_mode(void);

void eru_open(char *);
void eru_save(void);
void eru_scroll(void);
void eru_draw_rows(struct AppendBuffer *);
void eru_clear_screen(void);
int eru_read_key(void);

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

int buffer_create(char *buf_name);
int buffer_clear(Buffer *buf);
int buffer_delete(Buffer *buf);
int buffer_set_current(Buffer *buf);
void *buffer_set_next(Buffer *buf, Buffer target_buf);
int *buffer_set_name(Buffer *buf);
char *buffer_get_name(Buffer *buf);

int get_window_size(int *, int *);
int get_cursor_position(int *, int *);

int eru_syntax_colored(int);
void eru_select_syntax_highlight(void);
void eru_update_syntax(Row *);

int is_separator(int);
void eru_process_keypress(void);
void eru_move_cursor(int);

void eru_row_insert_char(Row *, int, int);
void eru_row_del_char(Row *, int);
void eru_del_char(void);
void eru_insert_char(int);

void eru_free_row(Row *);
void eru_del_row(int);
void eru_insert_newline(void);

char *eru_prompt(char *, void (char *, int));
void eru_search(void);
void eru_search_cb(char *, int);

void eru_init(void);

#endif
