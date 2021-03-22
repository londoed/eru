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
#include <termios.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1F)

void eru_error(const char *);
void disable_raw_mode(void);
void enable_raw_mode(void);
void eru_draw_rows(void);
void eru_clear_screen(void);
void eru_read_key(void);

int get_window_size(int *, int *);
int get_cursor_position(int *, int *);
void eru_process_keypress(void);

void eru_init(void);

struct Editor {
	struct termios orig;
	int screen_rows, screen_cols;
};

