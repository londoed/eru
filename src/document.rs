/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/document.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use crate::Row;
use crate::Terminal;
use std::fs;

pub struct Document {
    rows: Vec<Row>,
    pub file_name: Option<String>,
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
}