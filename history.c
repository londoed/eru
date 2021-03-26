/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { history.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"

History hist;

Editor
*editor_copy(Editor *old_ed)
{
	Editor *ed = malloc(sizeof(Editor));

	ed->cur_x 										= old_ed->cur_x;
	ed->cur_y 										= old_ed->cur_y;
	ed->ren_x 										= old_ed->ren_x;
	ed->row_offset 								= old_ed->row_offset;
	ed->col_offset 								= old_ed->col_offset;

	ed->screen_rows 							= old_ed->screen_rows;
	ed->screen_cols 							= old_ed->screen_cols;
	ed->dirty 										= old_ed->dirty;
	ed->num_rows 									= old_ed->num_rows;

	ed->filename									= old_ed->filename;
	ed->status_msg_time						= old_ed->status_msg_time;
	ed->syntax										= old_ed->syntax;
	ed->orig											= old_ed->orig;
	ed->mode											= old_ed->mode;
	ed->status_msg[0]							= *old_ed->status_msg;

	ed->row = malloc(sizeof(Row) * (old_ed->num_rows));
	int i;

	for (i = 0; i < ed->num_rows; i++) {
		ed->row[i].idx = old_ed->row[i].idx;
		ed->row[i].size = old_ed->row[i].size;
		ed->row[i].rsize = old_ed->row[i].rsize;
		ed->row[i].hl_open_comment = old_ed->row[i].hl_open_comment;

		ed->row[i].chars = malloc(old_ed->row[i].size);
		memcpy(ed->row[i].chars, old_ed->row[i].chars, old_ed->row[i].size);

		ed->row[i].render = malloc(old_ed->row[i].rsize);
		memcpy(ed->row[i].render, old_ed->row[i].render, old_ed->row[i].rsize);

		ed->row[i].highlight = malloc(old_ed->row[i].risze);
		memcpy(ed->row[i].highlight, old_ed->row[i].highlight, old_ed->row[i].rsize);
	}

	return ed;
}

struct Editor *
editor_history_push(struct Editor *snapshot)
{
	Editor *ed = editor_copy(snapshot);

	snapshot->redo = ed;
	ed->undo = snapshot;

	return new_e;
}

struct Editor *
editor_history_undo(struct Editor *ed)
{
	if (ed->undo)
		return ed->undo;

	return ed;
}

struct Editor *
editor_history_redo(struct Editor *ed)
{
	if (ed->redo)
		return ed->redo;

	return ed;
}


