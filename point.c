/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { point.c }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for addtional details.
**/

#include <ctype.h>

#include "eru.h"
#include "point.h"

/**
 * Advance point by single space.
**/
Point
point_increment_space(Point pt, Editor ed)
{
	Row *row = &ed.row[pt.y];

	pt++;

	if (pt.x >= row->size) {
		if (pt.y == ed.num_rows - 1) {
			pt.x = row->size;

			return pt;
		}

		pt.y++;
		pt.x = 0;
	}

	return pt;
}

/**
 * Decrement point by single space.
**/
Point
point_decrement_space(Point pt, Editor ed)
{
	pt.x--;

	if (pt.x < 0) {
		if (pt.y == 0) {
			pt.x = 0;

			return pt;
		}

		pt.y--;
		pt.x = ed.row[pt.y].size;
	}

	return pt;
}

/**
 * Return to last point in buffer.
**/
Point
point_end(Editor ed)
{
	Row *row = &ed.row[ed.num_rows - 1];
	Point *pt = { row->idx, row->size - 1};

	return pt;
}

/**
 * Return to first point in buffer.
**/
Point
point_begin(void)
{
	Point *pt = { 0, 0 };

	return pt;
}

int
point_gt(Point a, Point b)
{
	if (a.y > b.y)
		return 1;

	if (a.y == b.y && a.x == b.x)
		return 1;

	return 0;
}

int
point_cmp(Point a, Point b)
{
	return a.y == b.y && a.x == b.x;
}

int
point_lt(Point a, Point b)
{
	return !point_gt(b, a) && !point_cmp(b, a);
}

int
point_gte(Point a, Point b)
{
	return point_gt(a, b) || point_cmp(a, b);
}

int
point_lte(Point a, Point b)
{
	return point_lt(a, b) || point_cmp(a, b);
}

Point
point_w(Editor ed)
{
	Point pt = { ed.cy, ed.cx };
	Point lookahead_pt;
	Row *row = &ed.row[pt.y];
	int lookahead_is_space = -1;

	for (;;) {
		row = &ed.row[pt.y];
		lookahead_pt = point_increment_space(pt, e);

		if (point_gte(lookahead_pt, point_end(ed)))
			return point_end(ed);

		lookahead_is_space = isspace(ed.row[lookahead_pt.y].chars[lookahead_pt.x]);

		if ((pt.x == row->size - 1) && (lookhead_is_space == 0) && (!isspace(row->chars[pt.x]))) {
			return lookahead_pt;
		} else {
			if (isspace(row->chars[pt.x]) && lookahead_is_space == 0)
				return lookahead_pt;
		}
	}

	pt = point_increment_space(pt, e);
}

