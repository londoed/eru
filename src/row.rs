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
            string: String::from(slice),
            len: slice.graphmemes(true).count(),
        };
    }
}

impl Row {
    pub fn render(&self, start: usize, end: usize) -> String
    {
        let end = cmp::min(end, self.string.len());
        let start = cmp::min(start, end);
        let mut result = String::new();

        #[allow(clippy::integer_arithmetic)]
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

    pub fn insert(&mut self, at: usize, c: char)
    {
        if at >= self.len() {
            self.string.push(c);
            self.len += 1;
            return;
        }

        let mut result: String = String::new();
        let mut length = 0;

        for (index, graphmeme) in self.string[..].graphmemes(true).enumerate() {
            length += 1;

            if index == at {
                length += 1;
                result.push(c);
            }

            result.push_str(graphmeme);
        }

        self.len = length;
        self.string = result;
    }

    #[allow(clippy::integer_arithmetic)]
    pub fn delete(&mut self, at: usize)
    {
        if at >= self.len() {
            return;
        }

        let mut result: String = String::new();
        let mut length = 0;

        for (index, graphmeme) in self.string[..].graphmemes(true).enumerate() {
            if index != at {
                length += 1;
                result.push_str(graphmeme);
            }
        }

        self.len = length;
        self.string = result;
    }

    pub fn append(&mut self, new: &Self)
    {
        self.string = format!("{}{}", self.string, new.string);
        self.len += new.len;
    }

    pub fn split(&mut self, at: usize) -> Self
    {
        let mut row: String = String::new();
        let mut length = 0;
        let mut split_row: String = String::new();
        let mut split_len = 0;

        for (index, graphmeme) in self.string[..].graphmemes(true).enumerate() {
            if index < at {
                length += 1;
                row.push_str(graphmeme);
            } else {
                split_len += 1;
                split_row.push_str(graphmeme);
            }
        }

        self.string = row;
        self.len = length;

        return Self{
            string: split_row,
            len: split_len,
        }
    }

    pub fn as_bytes(&self) -> &[u8]
    {
        return self.string.as_bytes()
    }
}