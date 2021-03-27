/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/terminal.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use std::io::{self, stdout, Write};
use termion::event::Key;
use termion::input::TermRead;
use Termion::raw::{IntoRawMode, RawTerminal};
use crate::Position;

pub struct Size {
    pub width: u16,
    pub height: u16,
}

pub struct Terminal {
    size: Size,
    _stdout: RawTerminal<std::io::Stdout>,
}

impl Terminal {
    pub fn default() -> Result<Self, std::io::Error>
    {
        let size = termion::terminal_size()?;

        Ok(Self {
            size: Size {
                width: size.0,
                height: size.1,
            },
            
            _stdout: stdout().into_raw_mode();
        }),
    }

    pub fn size(&self) -> &Size
    {
        &self.size;
    }

    pub fn clear_screen()
    {
        print!("{}", termion::clear::All);
    }

    pub fn cursor_position(pos: &Position)
    {
        let Position{mut x, mut y} = position;
        x = x.saturating_add(1);
        y = y.saturating_add(1);

        let x = x as u16;
        let y = y as u16;

        print!("{}", termion::cursor::Goto(x, y));
    }



    pub fn flush() -> Result<(), std::io::Error>
    {
        io::stdout().flush();
    }

    pub fn read_key() - > Result<Key, std::io::Error>
    {
        loop {
            if let Some(key) = io::stdin().lock().keys().next() {
                return key;
            }
        }
    }

    pub fn cursor_hide()
    {
        print!("{}", termion::cursor::Hide);
    }

    pub fn cursor_show()
    {
        print!("{}", termion::cursor::Show);
    }

    pub fn clear_current_line()
    {
        print!("{}", termion::clear::CurrentLine);
    }
}