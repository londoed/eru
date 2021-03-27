/**
 * ERU: A simple, lightweight, protable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/row.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use std::cmp;
use unicode_segmentaition::UnicodeSegmentation;

pub struct Row {
    string: String,
    len: usize,
}

impl From<&str> for Row {
    fn from(slice: &str) -> Self
    {
        Self{
            let mut row = Self {
                string: String::from(slice),
                len: 0,
            };

            row.update_len();
            return row;
        }
    }
}

impl Row {
    pub fn render(&self, start: usize, end: usize) -> String
    {
        let end = cmp::min(end, self.string.len());
        let start = cmp::min(start, end);
        let mut result = String::new();

        for graphmeme in self.string[..].graphmemes(true).skip(start).take(end - start) {
            if graphmeme == "\t" {
                result.push_str(" ");
            } else {
                result.push_str(graphmeme);
            }
        }

        return result;
    }

    pub fn len(&self) -> usize
    {
        return self.string[..].graphmemes(true).count();
    }

    pub fn is_empty(&self) -> bool
    {
        return self.len == 0;
    }

    fn update_len(&mut self)
    {
        self.len = self.string[..].graphmemes(true).count();
    }
}