/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { history.h }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#ifndef HISTORY_H
#define HISTORY_H

#include "eru.h"

typedef struct History {
	Editor *buffer[5];
	int size;
} History;

#endif
