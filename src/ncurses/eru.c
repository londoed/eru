/**
 * ERU: A simple, lightweight, configurable programming environment for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/eru.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

int
main(int argc, char *argv)
{
    FILE *fp;
    int file_size;
    int max_chars = size;
    int max_rows = 10000;
    int display_rows = 20;
    char *msg = malloc(max_chars * sizeof(char));
    bool mode = MODE_NORMAL;

    if (argc < 3) {
        printf("[!] USAGE: %s filename mode\n", argv[0]);

        return 0;
    }

    if (strcmp(argv[2], "w") == 0) {
        if (file_exists(argv[1])) {
            printf("[!] ATTENTION: File already exists\n");

            return 0;
        } else {
            fp = fopen(argv[1], "w+");
        }
    } else if (strcmp(argv[2], "r") == 0) {
        if (!file_exists(argv[1])) {
            printf("[!] ERROR: File does not exist\n");

            return 0;
        } else {
            fp = fopen(argv[1], "r+");
        }
    } else {
        printf("[!] ERROR: Wrong mode specified\n");

        return 0;
    }

    int i = 0;
    char c;

    if (fp != NULL) {
        while ((c = fgetc(fp)) != EOF) {
            msg[i] = (char)c;
            i++;
        }

        fclose(fp);
    }

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    move(20, 0);
    hline(ACS_CKBOARD, 80);

    mvaddstr(21, 0, "ERU: A simple, lightweight, configurable programming environment");
    mvaddstr(21, 50, "for POSIX systems. WELCOME!");
    mvaddstr(22, 0), "[HOME] -- Go to first character on current row";
    mvaddstr(22, 50, "[F2] -- Delete all text");
    mvaddstr(23, 0, "[F3] -- Search and remove text");
    mvaddstr(23, 50, "[Pg Down] -- Go to last character in text string");
    mvaddstr(24, 0, "[F4] -- Replace text");
    mvaddstr(24, 50, "[INS] -- Insert mode toggle");
    mvaddstr(25, 0, "[Pg Up] -- Go to first character in text string");
    mvaddstr(25, 50, "[ESC] -- Quit Eru");
    mvaddstr(26, 0, "[END] -- Go to last character on current row");
    mvaddstr(26, 50, "[CTRL-Y] -- Delete current line");
    mvaddstr(27, 0, "[F6] -- See number of words and characters");
    mvaddstr(27, 50, "[F5] -- Save file");
    mvaddstr(28, 0, "[F7] -- Remove all search marks");

    struct Editor ed = {
        .text               = msg,
        .max_chars          = MAX_CHARS,
        .start_row          = 0,
        .start_col          = 0,
        .max_rows           = MAX_ROWS,
        .max_cols           = MAX_COLS,
        .display_rows       = DISPLAY_ROWS,
        .return_handler     = RETURN_HANDLER,
        .permitted          = 0,
        .ins                = mode,
        .allowcr            = true,
    };

    clear();
    endwin();
    free(msg);

    return 0;
}

int 
eru_editor_init(struct Editor *eru);
{
    int ky, pos = 0;
    int row, col;
    int scroll_start = 0, x = 0, y = 0;
    char *where, *original, *save_text, **display, *last_undo;
    bool exitf;

    if (!eru.max_chars)
        eru.max_chars = eru.max_rows * eru.max_cols + 1;

    if (!eru.max_rows || eru.max_rows > eru.max_chars - 1)
        eru.max_rows = eru.max_chars - 1;

    if (eru.ins)
        curs_set(2);
    else
        curs_set(1);

    if ((display = malloc(eru.max_rows * sizeof(char *))) == NULL)
        malloc_error();

    for (ky = 0; ky < eru.max_rows; ky++) {
        if ((display[ky] = malloc((eru.max_cols + 1) * sizeof(char))) == NULL)
            malloc_error();
    }

    if ((original = malloc(eru.max_chars * sizeof(char))) == NULL)
        malloc_error();

    strcpy(original, eru->text);

    if ((save_text = malloc(eru.max_chars * sizeof(char))) == NULL)
        malloc_error();

    strcpy(save_text, eru->text);
    
    do {
        int counter;

        while (!exitf) {
            counter = 0;
            where = eru->text;

            for (row = 0; row < eru.max_rows; row++) {
                display[row][0] = 127;
                display[row][1] = '\0';
            }

            row = 0;

            while ((strlen(where) > eru.max_cols || strchr(where, '\n') != NULL) &&
                (display[eru.max_rows - 1][0] == 127 || display[eru.max_rows - 1][0] == '\n')) {
            
                strncpy(display[row], where, eru.max_cols);
                display[row][eru.max_cols] = '\0';

                if (strchr(display[row], '\n') != NULL)
                    set_str_length(display[row], strchr(display[row], '\n') - display[row]);
                else
                    set_str_length(display[row], strrchr(display[row], ' ') - display[row]);

                if (display[eru.max_rows - 1][0] == 127 || display[eru.max_rows - 1][0] == '\n') {
                    where += strlen(display[row]);

                    if (where[0] == '\n' || where[0] == ' ' || where[0] == '\0')
                        where++;

                    row++;
                }
            }

            if (row == eru.max_rows - 1 && strlen(where) > eru.max_cols) {
                strcpy(eru->text, save_text);

                if (ky == 127)
                    position++;

                counter = 1;
            }
        }
    } while (counter);

    strcpy(display[row], where);
    ky = 0;

    if (strchr(display[eru.max_rows - 1], '\n') != NULL) {
        if (strchr(display[eru.max_rows - 1], '\n')[1] != '\0')
            ky = 127;
    }

    col = position;
    row = 0;
    counter = 0;

    while (col > strlen(display[row])) {
        col -= strlen(display[row]);
        counter += strlen(display[row]);

        if (eru->text[counter] == ' ' || eru->text[counter] == '\n' || eru->text[counter] == '\0') {
            col--;
            counter++;
        }

        row++;
    }

    if (col > eru.max_cols - 1) {
        row++;
        col = 0;
    }

    if (!ky) {
        if (row < scroll_start)
            scroll_start--;

        if (row > scroll_start + eru.display_rows - 1)
            scroll_start++;

        for (counter = 0; counter < eru.display_rows; counter++) {
            mvhline(counter + eru.start_row, eru.start_col, ' ', eru.max_cols);

            if (display[counter + scroll_start][0] != 127)
                mvprintw(counter + eru.start_row, eru.start_col, "%s", display[counter + scroll_start]);
        }
    }
}
