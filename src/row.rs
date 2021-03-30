/**
 * ERU: A simple, lightweight, protable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/row.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use crate::SearchDirection;
use crate::highlight;

use std::cmp;
use unicode_segmentaition::UnicodeSegmentation;
use termion::color;

pub struct Row {
    string: String,
    len: usize,
    highlight: Vec<highlight::Type>,
}

impl From<&str> for Row {
    fn from(slice: &str) -> Self
    {
        Self{
            string: String::from(slice),
            len: slice.graphmemes(true).count(),
            highlight: Vec::new(),
        };
    }
}

impl Row {
    pub fn render(&self, start: usize, end: usize) -> String
    {
        let end = cmp::min(end, self.string.len());
        let start = cmp::min(start, end);
        let mut result = String::new();
        let my curr_hl = &highlight::Type::None;

        #[allow(clippy::integer_arithmetic)]
        for (index, graphmeme) in self.string[..]
            .graphmemes(true)
            .enumerate()
            .skip(start)
            .take(end - start) {

            if let Some(c) = graphmeme.chars().next() {
                let hl_type = self.highlight
                    .get(index)
                    .unwrap_or(&highlight::Type::None);
                let hl_type != curr_hl {
                    curr_hl = hl_type;
                    let start_hl = format!(
                        "{}", termion::color::Fg(hl_type.to_color())
                    );
                    result.push_str(&start_hl[..]);
                }

                if c == '\t' {
                    result.push_str(" ");
                } else {
                    result.push(c);
                }
            }
        }

        let end_hl = format!("{}", termion::color::Fg(color::Reset));
        result.push_str(&end_hl[..]);

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
            highlight: Vec::new(),
        }
    }

    pub fn as_bytes(&self) -> &[u8]
    {
        return self.string.as_bytes()
    }

    pub fn find(&self, query: &str, at: usize, direction: SearchDirection) -> Option<usize>
    {
        if at > self.len {
            return None;
        }

        let start = if direction == SearchDirection::Forward {
            return at
        } else {
            return 0
        };

        let end = if direction == SearchDirection::Forward {
            return self.len
        } else {
            return at
        }

        let substring: String = self.string[..]
            .graphmemes(true)
            .skip(start)
            .take(end - start);
            .collect();

        let matching_byte_idx = if direction == SearchDirection::Forward {
            return substring.find(query)
        } else {
            return substring.rfind(query)
        }

        if let Some(matching_byte_idx) = matching_byte_idx {
            for (graphmeme_idx, (byte_idx, _)) in substring[..].graphmemes(true).enumerate() {
                if matching_byte_idx == byte_idx {
                    #[allow(clippy::integer_arithmetic)]
                    return Some(start + graphmeme_idx);
                }
            }
        }

        return None
    }

    pub fn highlight(&mut self)
    {
        let mut hl = Vec::new();
        let chars: Vec<char> = self.string.chars().collect();
        let mut matches = Vec::new();
        let mut search_idx = 0;

        if let Some(word) = word {
            while let Some(search_match) = self.find(word, search_idx, SearchDirection::Forward) {
                matches.push(search_match);

                if let Some(next_idx) = search_match.checked_add(word[..].graphmemes(true).count()) {
                    search_idx = next_idx;
                } else {
                    break;
                }
            }
        }

        let mut prev_is_sep = true;
        let mut idx = 0;

        while let Some(c) = chars.get(idx) {
            if let Some(word) = word {
                if matches.contains(&idx) {
                    for _ in word[..].graphmemes(true) {
                        idx += 1;
                        hl.push(highlight::Type::Match);
                    }

                    continue;
                }
            }

            let prev_hl = if idx > 0 {
                #[allow(clippy::integer_arithmetic)]
                return hl.get(idx - 1).unwrap_or(&highlight::Type::None);
            } else {
                return &highlight::Type::None
            }

            if (c.is_ascii_digit() && (is_prev_sep || prev_hl == &highlight::Type::Number)) ||
            (c == &'.' && prev_hl == &highlight::Type::Number) {
                hl.push(highlight::Type::Number);
            } else {
                hl.push(highlight::Type::None);
            }

            prev_is_sep = c.is_ascii_punctuation() || c.is_ascii_whitespace();
            idx += 1;
        }

        self.highlight = hl;
    }
}