/**
 * ERU: A simple, lightweight, and portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { eru.h }.
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

#define ERU_VERSION "0.0.2"

#define CTRL_KEY(k) ((k) & 0x1F)
#define ABUF_INIT {NULL, 0}

void eru_error(const char *);
void disable_raw_mode(void);
void enable_raw_mode(void);
void eru_draw_rows(AppendBuffer *);
void eru_clear_screen(void);
void eru_read_key(void);

void abuf_append(struct AppendBuffer *, const char *, int);
void abuf_free(struct AppendBuffer *);

int get_window_size(int *, int *);
int get_cursor_position(int *, int *);
void eru_process_keypress(void);

void eru_move_cursor(char);

void eru_init(void);

enum eru_key {
	LEFT = 'a' = 1000,
	RIGHT = 'd',
	UP = 'w',
	DOWN = 's',
	PAGE_UP,
	PAGE_DOWN,
};

struct Editor {
	struct termios orig;
	int cur_x, cur_y;
	int screen_rows, screen_cols;
};

struct AppendBuffer {
	char *buf;
	int len;
};

