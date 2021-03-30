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
    int row, col, scroll_start;
    char *where, *original, *save_text, **display, *last_undo;
    bool exitf;

    pos.x = 0;
    pos.y = 0;

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

        move(row + eru.start_row - scroll_start, col + eru.start_col);
        ky = getch();
    }

    switch (ky) {
    case 27:
        exitf = true;
        break;

    case KEY_F(2):
        memset(eru->text, '\0', eru.max_chars);
        position = 0;
        scroll_start = 0;
        break;

    case KEY_F(3):
        getyx(stdscr, pos.y, pos.x);
        remove_search_marks(eru->text);
        search_string(NULL, eru->text);
        clr_bottom();

        move(pos.y, pos.x);
        break;
    
    case KEY_F(4):
        getxy(stdscr, pos.y, pos.x);
        replace_string(eru->text);
        clr_bottom();
        
        move(pos.y, pos.x);
        break;

    case KEY_F(5):
        getyx(stdscr, pos.y, pos.x);
        save_file(eru->text);
        clr_bottom();

        move(pos.y, pos.x);
        break;

    case KEY_F(6):
        clr_bottom();
        count_words_chars(eru->text);

    case KEY_F(7):
        remove_search_marks(eru->text);

    case KEY_HOME:
        if (col) {
            position = 0;

            for (col = 0; col < row; col++) {
                position += strlen(display[col]);

                if ((strchr(display[row], '\n') != NULL) || (strchr(display[row], ' ') != NULL))
                    position++;
            }
        }

        break;

    case KEY_END:
        if (col < strlen(display[row])) {
            position = -1;

            for (col = 0; col <= row; col++) {
                position += strlen(display[col]);

                if ((strchr(display[row], '\n') != NULL) || (strchr(display[row], ' ') != NULL))
                    position++;
            }
        }

        break;

    case KEY_PPAGE:
        position = 0;
        scroll_start = 0;
        break;
    
    case KEY_NPAGE:
        position = strlen(eru->text);

        for (counter = 0; counter < eru.max_rows; counter++) {
            if (display[counter][0] == 127)
                break;
        }

        scroll_start = counter - eru.display_rows;

        if (scroll_start < 0)
            scroll_start = 0;

        break;

    case KEY_LEFT:
        if (position)
            position--;

        break;

    case KEY_RIGHT:
        if (position < strlen(text) && (row != eru.max_rows - 1 || col < eru.max_cols - 1))
            position++;

        break;

    case KEY_UP:
        if (row) {
            if (col > strlen(display[row - 1]))
                position = strlen(display[row - 1]);
            else
                position = col;

            ky = 0;

            for (col = 0; col < row - 1; col++) {
                position += strlen(display[col]);
                ky += strlen(display[col]);

                if ((strlen(display[col]) < eru.max_cols) || (eru->text[ky] == ' ' &&
                    strlen(display[col]) == eru.max_cols)) {
                    
                    position++;
                    ky++;
                }
            }
        }

        break;

    case KEY_DOWN:
        if (row < eru.max_rows - 1) {
            if (display[row + 1][0] != 127) {
                if (col < strlen(display[row + 1]))
                    position = col;
                else
                    position = strlen(display[row + 1]);

                ky = 0;

                for (col = 0; col <= row; col++) {
                    position += strlen(display[col]);
                    ky += strlen(display[col]);

                    if ((strlen(display[col]) < eru.max_cols) || (eru->text[ky] == ' ' &&
                        strlen(display[col]) == eru.max_cols)) {
                        
                        position++;
                        ky++;
                    }
                }
            }
        }

        break;

    case KEY_IC:
        eru.ins = !eru.ins;

        if (ins)
            curs_set(2);
        else
            curs_set(1);

        break;

    case 330:
        if (strlen(eru->text)) {
            strcpy(save_text, eru->text);
            memmove(&eru->text[position], &eru->text[position + 1], eru.max_chars - position);
        }

        break;

    case KEY_BACKSPACE:
        if (strlen(text) && position) {
            strcpy(save_text, eru->text);
            position--;
            memmove(&eru->text[position], &eru->text[position + 1], eru.max_chars - position);
        }

        break;

    case 25:
        if (display[1][0] != 127) {
            position -= col;
            ky = 0;

            do {
                memmove(&eru->text[position], &eru->text[position + 1], eru.max_chars - position);
                ky++;
            } while (ky < strlen(display[row]));
        } else {
            memset(eru->text, '\0', eru.max_chars);
        }

        break;

    case 10:
        switch (eru.return_handler) {
        case 1:
            if (display[eru.max_rows - 1][0] == 127 || display[eru.max_rows - 1][0] == '\n') {
                memmove(&eru->text[position + 1], &eru->text[position], eru.max_chars - position);
                eru->text[position] = '\n';
                position++;
            }

            break;

        case 2:
            ky = ' ';
            ungetch(ky);
            break;

        case 3:
            exitf = true;

        default:
            break;
        }

        break;

    default:
        if (((eru->permitted == NULL && ky > 31 && ky < 127) || 
            (eru->permitted != NULL && strchr(eru->permitted, ky))) &&
            strlen(eru->text) < eru.max_chars - 1 &&
            (row != eru.max_rows - 1) || (strlen(display[eru.max_rows - 1]) < eru.max_cols ||
            (eru.ins && (row != eru.max_rows - 1 && col < eru.max_cols)))) {
            
            if (eru.ins || eru->text[position + 1] == '\n' || eru->text[position] == '\n')
                memmove(&eru->text[position + 1], &eru->text[position], eru.max_chars - position);

            eru->text[position] = ky;

            if (row != eru.max_rows - 1 || col < eru.max_cols - 1)
                position++;
        }
    }

    if (!eru.allowcr) {
        if (eru->text[0] == '\n') {
            memmove(&eru->text[0], &eru->text[1], eru.max_chars - 1);

            if (position)
                position--;
        }
    }

    free(original);
    free(save_text);
    free(last_undo);

    for (scroll_start = 0; scroll_start < eru.max_rows; scroll_start++)
        free(display[scroll_start]);

    free(display);

    return 0;
}

char *
set_string_length(char *str, const int len)
{
    if (strlen(str) > len)
        str[len] = '\0';

    return str;
}

void
malloc_error(void)
{
    endwin();
    fprintf(stderr, "[!] ERROR: eru: Out of memory\n");
    exit(EXIT_FAILURE);
}

int
file_exists(char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL)
        return 0;

    fclose(fp);

    return 1;
}

void
clr_bottom(void)
{
    int i = 0, j = 0;
    int max_y, max_x;

    getmaxyx(stdscr, max_y, max_x);

    for (i = 29; i < max_y; i++) {
        for (j = 0; j < max_x; j++) {
            move(i, j);
            addch(' ');
        }
    }
}

int
save_file(char *text)
{
    char path[200], ch_over, spath = 'a';
    int sp, i = 0;
    FILE *fps;

    move(33, 0);
    printw("[!] ATTENTION: eru: Enter path to save text: ");

    while (spath != '\n') {
        spath = getch();
        sp = (int)spath;

        if ((sp >= 65 && sp <= 90) || (sp >= 48 && sp <= 57) || (sp >= 97 && sp <= 122) || sp == 95 || sp == 46) {
            addch(spath);
            path[i] = spath;
            i++
        }

        if (sp == 27) {
            move(33, 0);
            deleteln();

            return 0;
        }
    }

    path[i] = '\0';

    if (file_exist(path)) {
        move(33, 0);
        deleteln();
        printw("\n[!] ERROR: eru: File by this name already exist...overwrite? [Y/n]");
        
        ch_over = getch();
        addch(ch_over);

        if (ch_over == 'y' || ch_over == 'Y') {
            fps = fopen(path, "w+");
            fprintf(fps, "%s", text);
            fclose(fps);

            move(33, 0);
            printw("\n[!] ATTENTION: eru: File saved successfully!");
            
            return 1;
        }

        move(33, 0);
        deleteln();
        printw("[!] ATTENTION: eru: File not saved!");

        return 0;
    }

    fps = fopen(path, "w+");
    fprintf(fps, "%s", text);
    move(33, 0);
    deleteln();

    printw("[!] ATTENTION: eru: File saved successfully!");
    fclose(fps);

    return 1;
}

int
insert_string(int pos, char *string, char *data)
{
    int i = pos, j = 0;

    if (strlen(data) == 0) {
        for (i = 0; i < strlen(string); i++)
            data[i] = string[i];
    } else {
        for (i = strlen(data); i >= pos; i--)
            data[i + strlen(string)] = data[i];

        for (i = pos; i < pos + strlen(string); i++, j++) {
            if (string[j] != '\0')
                data[i] = string[j];
        }
    }

    return 0;
}

int
search_string(char *string, char *data)
{
    int i = 0, j = 0, start[100], end[100], n_o_occur = 0, max_chars = SIZE;
    char ch = 'a', s_a = 'n', st[2] = '[', en[2] = ']';

    move(32, 0);

    if (string == NULL) {
        s_a = 'y';
        string = malloc(500 * sizeof(char));
        printw("[!] SEARCH: Enter search pattern: ");

        while (ch != '\n') {
            ch = getch();
            addch(ch);
            string[i] = ch;
            i++;

            if (ch == 127) {
                getyx(stdscr, y, x);
                move(y, x - 1);
            }

            if (ch == 27) {
                move(33, 0);
                deleteln();

                return 0;
            }
        }

        string[i] = '\0';
    }

    int flag = 0, len1, len2;
    len1 = strlen(string);
    len2 = strlen(data);

    if (len1 > len2)
        return 0;

    for (i = 0; i <= len2; ++i) {
        if (string[0] == data[i]) {
            flag = 1;

            for (j = 1; j < (len1 - 1) && (i + j) < strlen(data); ++j) {
                if (string[j] != data[i + j]) {
                    flag = 0;
                    break;
                }
            }

            if ((flag == 1 || (i + j) > strlen(data)) && s_a == '\n')
                break;

            if ((flag == 1 || (i + j) > strlen(data)) && s_a == 'y') {
                insert_string(i, st, data);
                insert_string(i + j + 1, en, data);
                i++;

                search_marks = true;
            }
        }
    }

    if (flag == 1)
        return i;

    return -1;
}

int
delete_string(int pos, int len, char *data)
{
    int i = pos;

    for (i; i < strlen(data) - len; i++)
        data[i] = data[i + len];

    data[i] = '\0';

    return 0;
}

int
count_words_chars(char *data)
{
    int i, wrd_count = 0;

    for (i = 0; i < strlen(data); i++) {
        if (i == 0)
            wrd_count++;

        if (data[i] == ' ' || data[i] == '\n')
            wrd_count++;
    }

    move(34, 0);
    printw("[!] INFO: eru: Characters: %d, Words: %d", strlen(data), wrd_count);

    return 0;
}

int
replace_string(char *data)
{
    int pos, i = 0, j = 0, start[100], end[100], n_o_occur = 0, max_chars = SIZE;
    char string[500], *replace_with, ch = 'a';

    replace_with = malloc(500 * sizeof(char));
    move(32, 0);
    deleteln();
    printw("[!] REPLACE: Enter pattern to replace: ");

    while (ch != '\n') {
        ch = getch();
        addch(ch);

        if (ch != '\n') {
            string[i] = ch;
            i++;
        }

        if (ch == 27) {
            move(33, 0);
            deleteln();

            return 0;
        }
    }

    string[i] = '\0';
    ch = 'a';
    move(33, 0);
    i = 0;
    printw("[!] REPLACE: Enter replacement string: ");

    while (ch != '\n') {
        ch = getch();
        addch(ch);

        if (ch != '\n') {
            replace_with[i] = ch;
            i++;
        }

        if (ch == 27) {
            move(33, 0);
            deleteln();

            return 0;
        }
    }

    deleteln();
    replace_with[i] = '\0';

    while ((pos = search_string(string, data)) != -1) {
        delete_string(pos, strlen(string), data);
        insert_string(pos, replace_with, data);
    }

    free(replace_with);

    return 0;
}

void
remove_search_marks(char *text)
{
    int i;

    if (search_marks == true) {
        search_marks = false;

        for (i = 0; i < strlen(text); i++) {
            if (text[i] == '[' || text[i] == ']')
                delete_string(i, 1, text);
        }
    }
}
