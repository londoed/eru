/**
 * ERU: A simple, lightweight, configurable programming environment for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/eru.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SIZE 600000
#define MAX_CHARS SIZE
#define MAX_ROWS 10000
#define MAX_COLS 60
#define DISPLAY_ROWS 20
#define RETURN_HANDLER 1

enum MODE {
    MODE_INSERT,
    MODE_NORMAL,
};


typedef struct Position {
    int x, y;
};

struct Editor {
    char *text;
    int max_chars;
    int start_row;
    int start_col;
    int max_rows;
    int max_cols;
    int display_rows;
    int return_handler;
    char *permitted;
    int mode;
    bool allowcr;
};


static Position *pos;
bool search_marks = false;

char *set_str_length(char *str, const int len);
void malloc_error(void);
int eru_editor_init(Editor ed);
void remove_search_marks(char *text);
int file_exists(char *path);
void clr_bottom();
int save_file(char *text);
int insert_string(int pos, char *string, char *data);
int search_string(char *string, char *data);
int delete_string(int pos, int len, char *data);
int count_words_chars(char *data);
int replace_string(char *data);
