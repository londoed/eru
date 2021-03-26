/**
 * ERU: A simple, lightweight, portable text editor of POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { point.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef POINT_H
#define POINT_H

#include "eru.h"

typedef struct Point {
	int y, x;
} Point;

Point point_w(Editor);

#endif
