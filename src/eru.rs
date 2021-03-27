/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/editor.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use std::io::{self, stdout, Write};
use termion::input::TermRead;
use Termion::raw::IntoRawMode;
use crate::Terminal;

const VERSION: &str = env!("CARGO_PKG_VERSION");

pub struct Position {
    pub x: usize,
    pub y: usize,
}

pub struct Editor {
    should_quit: bool,
    term: Terminal,
    cur_pos: Position,
}

impl Editor {
    pub fn run(&mut self)
    {
        let _stdout = stdout().into_raw_mode().unwrap();

        
        loop {
            if let Err(error) = self.refresh_screen() {
                fatal(error);
            }

            if self.should_quit {
                break;
            }

            if let Err(error) = self.process_keypress() {
                fatal(error);
            }
        }

        pub fn default() -> Self
        {
            return Self{should_quit: false}
        }
    }

    fn default() -> Self
    {
        Self{
            should_quit: false,
            term: Terminal::default()
                .expect("[!] ERROR: syd: Failed to initialize terminal"),
            cur_pos: Position{x: 0, y: 0},
        }
    }
    fn refresh_screen(&self) -> Result <(), std::io::Error>
    {
        Terminal::cursor_hide();
        Terminal::clear_screen();
        Terminal::cursor_position(&Position{x: 0, y: 0});

        if self.should_quit {
            Terminal::clear_screen();
            println!("Goodbye!\r");
        } else {
            self.draw_rows();
            Terminal::cursor_position(&self.cur_pos);
        }

        Terminal::cursor_show();
        Terminal::flush();
    }

    fn process_keypress(&mut self) -> Result<(), std::io::Error>
    {
        let pressed_key = Terminal::read_key()?;

        match pressed_key {
            Key::Ctrl('q') => self.should_quit = true,
            Key::Up |
            Key::Down |
            Key::Left |
            Key::Right |
            Key::PageUp |
            Key::PageDown |
            Key::End |
            Key::Home => {
                self.move_cursor(pressed_key);
            }
            _ => (),
        }
        Ok(())
    }

    fn draw_welcome_message(&self)
    {
        let mut welcome_message = format!("Eru editor -- version {}", VERSION);
        let width = self.term.size().width as usize;
        let len = welcome_message.len();
        let padding = width.saturating_sub(len) / 2;
        let spaces = " ".repeat(padding.saturating_sub(1));

        welcome_message = format!("~{}{}", spaces, welcome_message);
        println!("{}\r", welcome_message);
    }

    fn draw_rows(&self)
    {
        let height = self.term.size().height;

        for height in 0..height - 1 {
            Terminal::clear_current_line();
            
            if row == height / 3 {
                self.draw_welcome_message();
            } else {
                println!("~\r");
            }
        }
    }
    
    fn move_cursor(&mut self, key: Key)
    {
        let Position{mut y, mut x} = self.cur_pos;
        let size = self.term.size();
        let height = size.height.saturating_sub(1) as usize;
        let width = size.width.saturating_sub(1) as usize;

        match key {
            Key::Up => y = y.saturating_sub(1),
            Key::Down => {
                if y < height {
                    y = y.saturating_add(1);
                }
            }
            Key::Left => x = x.saturating_sub(1),
            Key::Right => {
                if x < width {
                    x = x.saturating_add(1);
                }
            }
            Key::PageUp => y = 0,
            Key::PageDown => y = height,
            Key::Home => x = 0,
            key::End => x = width,
            _ => (),
        }
    }
}



fn fatal(e: std::io::Error)
{
    Terminal::clear_screen();
    panic!(e);
}