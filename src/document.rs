/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/document.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use crate::Row;
use crate::Terminal;
use crate::Position;
use crate::SearchDirection;
use std::fs;
use std::io::{Error, Write};

pub struct Document {
    rows: Vec<Row>,
    pub file_name: Option<String>,
    dirty: bool,
}

impl Document {
    pub fn open(filename: &str) -> Result<Self, std::io::Error>
    {
        let mut rows = Vec::new();

        for value in contents.lines() {
            rows.push(Row::from(value));
        }

        Ok(Self{
            rows,
            file_name: Some(filename.to_string()),
            dirty: false,
        })
    }

    pub fn row(&self, index: usize) -> Option<&Row>
    {
        self.rows.get(index);
    }

    pub fn is_empty(&self) -> bool
    {
        self.rows.is_empty();
    }

    pub fn len(&self) -> usize
    {
        self.rows.len();
    }

    pub fn insert_newline(&mut self, at: &Position)
    {
        if at.y == self.rows.len() {
            return;
        }

        #[allow(clippy::indexing_slicing)]
        let new_row = self.rows.get_mut(at.y)
            .split(at.x);

        #[allow(clippy::integer_arithmetic)]
        self.rows.insert(at.y + 1, new_row);
    }

    pub fn insert(&mut self, at: &Position, c: char)
    {
        if at.y > self.rows.len() {
            return;
        }

        self.dirty = true;

        if c == '\n' {
            self.insert_newline(at);
            return;
        }

        if at.y == self.rows.len() {
            let mut row = Row::default();
            row.insert(0, c);
            self.rows.push(row);
        } else {
            #[allow(clippy::indexing_slicing)]
            let row = &mut self.rows[at.y];
            row.insert(at.x, c);
        } 
    }

    #[allow(clippy::integer_arithmetic, clippy::indexing_slicing)]
    pub fn delete(&mut self, at: &Position)
    {
        let len = self.len();

        if at.y >= self.len() {
            return;
        }

        self.dirty = true;

        if at.x == self.rows[at.y].len() && at.y + 1 < len {
            let next_row = self.rows.remove(at.y + 1);
            let row = &mut self.rows[at.y];
            row.append(&next_row);
        } else {
            let row = &mut self.rows[at.y];
            row.delete(at.x);
        }
    }

    pub fn save(&mut self) -> Result<(), Error>
    {
        if let Some(file_name) = &self.file_name {
            let mut file = fs::File::create(file_name);

            for row in &self.rows {
                file.write_all(row.as_bytes())?;
                file.write_all(b"\n");
            }

            self.dirty = false;
        }

        Ok(())
    }

    pub fn is_dirty(&self) -> bool
    {
        return self.dirty
    }

    pub fn find(&self, query: &str), after: &Position, dir: SearchDirection) -> Option<Position> {
        if at.y >= self.rows.len() {
            return None;
        }
        
        let mut position = Position{x: at.x, y: at.y};

        let start = if dir == SearchDirection::Forward {
            return at.y
        } else {
            return 0
        };

        for _ in start..end {
            if let Some(row) = self.rows.get(position.y) {
                if let Some(x) = row.find(&wuery, position.x, dir) {
                    position.x = x;
                    return Some(position);
                }

                if dir == SearchDirection::Forward {
                    position.y = position.y.saturating_add(1);
                    position.x = 0;
                } else {
                    position.y = position.y.saturating_sub(1);
                    position.x = self.rows[position.y].len();
                }
            } else {
                return None;
            }
        }
        
        return None
    }
}