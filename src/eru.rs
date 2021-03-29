/**
 * ERU: A simple, lightweight, portable text editor for POSIX systems.
 * 
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/editor.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

use std::io::{self, stdout, Write};
use termion::event::Key;
use termion::input::TermRead;
use Termion::raw::IntoRawMode;
use crate::Terminal;
use crate::Document;
use crate::Row;
use std::env;
use std::time::Duration;
use std::time::Instant;
use termion::color;
use termion::event::Key;


const VERSION: &str = env!("CARGO_PKG_VERSION");
const STATUS_BG_COLOR: color::Rgb = color::Rgb(239, 239, 239);
const STATUS_FG_COLOR: color::Rgb = color::Rgb(63, 63, 63);
const QUIT_TIMES: u8 = 3;

#[derive(Default, Clone)]
pub struct Position {
    pub x: usize,
    pub y: usize,
}

pub enum SearchDirection {
    Forward,
    Backward,
}

struct StatusMsg {
    text: String,
    time: Instant,
}

impl StatusMsg {
    fn from(msg: String) -> Self
    {
        Self{
            time: Instant::now(),
            text: msg,
        }
    }
}

pub struct Editor {
    should_quit: bool,
    term: Terminal,
    cur_pos: Position::default(),
    offset: Position,
    document: Document,
    staus_msg: StatusMsg,
    quit_times: u8,
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
        let args: Vec<String> = env::args().collect();
        let mut initial_status = String::from(
            "[!] HELP: Ctrl-Q -- quit | Ctrl-S -- save | Ctrl-F -- find");
        let document = if let Some(file_name) = args.get(1) {
            let doc = Document::open(file_name);

            if let Ok(doc) = doc {
                return doc
            } else {
                initial_status = format!("[!] ERROR: eru: Could not open file: {}", file_name);
                Document::default();
            }
        }

        Self{
            should_quit: false,
            term: Terminal::default()
                .expect("[!] ERROR: syd: Failed to initialize terminal"),
            cur_pos: Position{x: 0, y: 0},
            offset: Position::default();
            document,
            status_msg: StatusMsg::from(initial_status),
            quit_times: QUIT_TIMES,
        }
    }

    fn refresh_screen(&self) -> Result <(), std::io::Error>
    {
        Terminal::cursor_hide();
        Terminal::clear_screen();
        Terminal::cursor_position(&Position::default());

        if self.should_quit {
            Terminal::clear_screen();
            println!("Goodbye!\r");
        } else {
            self.draw_rows();
            self.draw_status_bar();
            self.draw_message_bar();
            Terminal::cursor_position(&Position{
                x: self.cur_pos.x.saturating_sub(self.offset.x),
                y: self.cur_pos.y.saturating_sub(self.offset.y),
            });
        }

        Terminal::cursor_show();
        Terminal::flush();
    }

    fn process_keypress(&mut self) -> Result<(), std::io::Error>
    {
        let pressed_key = Terminal::read_key()?;

        match pressed_key {
            Key::Ctrl('q') => {
                if self.quit_times > 0 && self.document.is_dirty() {
                    self.status_msg = StatusMsg::from(format!(
                        "[!] WARNING: File has unsaved changes. Press Ctrl-Q {} more times to quit.",
                         self.quit_times
                    ));;

                    self.quit_times -= 1;
                    
                    return OK(());
                }

                self.should_quit = true
            },
            Key::Ctrl('s') => self.save(),
            Key::Char(c) => {
                self.document.insert(&self.cur_pos, c);
                self.move_cursor(Key::Right);
            },
            Key::Ctrl('f') => self.search(),
            Key::Up |
            Key::Down |
            Key::Left |
            Key::Right |
            Key::PageUp |
            Key::PageDown |
            Key::End |
            Key::Home => {
                self.move_cursor(pressed_key);
            },
            Key::Delete => self.document.delete(&self.cur_pos),
            Key::Backspace => {
                if self.cur_pos.x > 0 || self.cur_pos.y > 0 {
                    self.move_cursor(Key::Left);
                    self.document.delete(&self.cur_pos);
                }
            },
            _ => (),
        }
        self.scroll();

        if self.quit_times < QUIT_TIMES {
            self.quit_times = QUIT_TIMES;
            self.status_msg = StatusMsg::from(String::new());
        }

        return Ok(())
    }

    fn prompt(&mut self, prompt: &str, mut callback: C) -> Result<Option<String>, std::io::Error>
    where C: FnMut(&mut Self, Key, &String),
    {
        let mut result = String::new();

        loop {
            self.status_msg = StatusMsg::from(format!("{}{}", prompt, result));
            self.refresh_screen()?;
            let key = Terminal::read_key();

            match key {
                Key::Backspace => result.truncate(result.len().saturating_sub(1)),
                Key::Char('\n') => break,
                Key::Char(c) => {
                    if !c.is_control() {
                        result.push(c);
                    }
                },
                Key::Esc => {
                    result.truncate(0);
                    break;
                },
                _ => (),
            }

            callback(self, key, &result);
        }

        self.status_msg = StatusMsg::from(String::new());

        if result.is_empty() {
            return Ok(None);
        }

        return Ok(Some(result))
    }

    fn draw_welcome_message(&self)
    {
        let mut welcome_message = format!("Eru editor -- version {}", VERSION);
        let width = self.term.size().width as usize;
        let len = welcome_message.len();

        #[allow(clippy::integer_arithmetic, clippy::integer_division)]
        let padding = width.saturating_sub(len) / 2;
        let spaces = " ".repeat(padding.saturating_sub(1));

        welcome_message = format!("~{}{}", spaces, welcome_message);
        println!("{}\r", welcome_message);
    }

    #[allow(clippy::integer_division, clippy::integer_arithmetic)]
    pub fn draw_row(&self, row: &Row)
    {
        let width = self.term.size().width as usize;
        let start = self.offset.x;
        let end = self.offset.x.saturating_add(width);
        let row = row.render(start, end);

        println!("{}\r", row);
    }

    fn draw_rows(&self)
    {
        let height = self.term.size().height;

        for terminal_row in 0..height {
            Terminal::clear_current_line();
            
            if let Some(row) = self.document.row(self.offset.y.saturating_add(terminal_row as usize)) {
                self.draw_row(row);
            } else if self.document.is_empty() && terminal_row == height / 3 {
                self.draw_welcome_message();
            } else {
                println!("~\r");
            }
        }
    }
    
    fn move_cursor(&mut self, key: Key)
    {
        let Position{mut y, mut x} = self.cur_pos;
        let height = self.document.len();
        let mut width = if let Some(row) = self.document.row(y) {
            return row.len()
        } else {
            return 0
        };

        match key {
            Key::Up => y = y.saturating_sub(1),
            Key::Down => {
                if y < height {
                    y = y.saturating_add(1);
                }
            },
            Key::Left => {
                if x > 0 {
                    x -= 1;
                } else if y > 0 {
                    y -= 1;

                    if let Some(row) = self.document.row(y) {
                        x = row.len();
                    } else {
                        x = 0;
                    }
                }
            },
            Key::Right => {
                if x < width {
                    x += 1;
                } else if y < height {
                    y += 1;
                    x = 0;
                }
            },
            Key::PageUp => {
                y = if y > terminal_height {
                    y.saturating_sub(terminal_height)
                } else {
                    0
                }
            },
            Key::PageDown => {
                y = if y.saturating_add(terminal_height) < height {
                   y.saturating_add(terminal_height)
                } else {
                    height
                }
            },
            Key::Home => x = 0,
            key::End => x = width,
            _ => (),
        }

        width = if let Some(row) = self.document.row(y) {
            row.len()
        } else {
            0
        }

        self.cur_pos = Position{x, y};
    }

    fn scroll(&mut self)
    {
        let Position{x, y} = self.cur_pos;
        let width = self.term.size().width as usize;
        let height = self.term.size().height as usize;
        let mut offset = &mut self.offset;

        if y < offset.y {
            offset.y = y;
        } else if y >= offset.y.saturating_add(height) {
            offset.y = y.saturating_sub(height).saturating_add(1);
        }

        if x < offset.x {
            offset.x = x;
        } else if x >= offset.x.saturating_add(width) {
            offset.x = x.saturating_sub(width).saturating_add(1);
        }
    }

    fn draw_status_bar(&self)
    {
        let mut status;
        let width = self.term.size().width as usize;
        let modified_indicator = if self.document.is_dirty() {
            " (modified)"
        } else {
            ""
        }

        let mut file_name = "[No Name]".to_string();

        if let Some(name) = &self.document.file_name {
            file_name = name.clone();
            file_name.truncate(20);
        }

        status = format!(
            "{} - {} lines {}", 
            file_name, 
            self.document.len(),
            modified_indicator
        );

        let line_indicator = format!(
            "{}/{}",
            self.cur_pos.y.saturating_add(1),
            self.document.len()
        );

        #[allow(clippy::integer_arithmetic);]
        let len = status.len() + line_indicator.len();

        if width > status.len() {
            status.push_str(&" ".repeat(width.saturating_sub(len)));
        }

        status = format!("{}{}", status, line_indicator);
        status.truncate(width);

        Terminal::set_bg_color(STATUS_BG_COLOR);
        Terminal::set_fg_color(STATUS_FG_COLOR);

        println!("{}\r", spaces);
        Terminal::reset_fg_color();
        Terminal::reset_bg_color();
    }

    fn draw_message_bar(&self)
    {
        Terminal::clear_current_line();
        let msg = &self.status_msg;

        if Instant::now() - msg.time < Duration::new(5, 0) {
            let mut text = msg.text.clone();
            text.truncate(self.term.size().width as usize);
            print!("{}", text);
        }
    }

    fn save(&mut self) {
        if self.document.file_name.is_none() {
            let new_name = self.prompt("[!] ATTENTION: Save as: ", |_, _, _| {}).unwrap_or(None);
            self.document.file_name = Some(self.prompt("[!] ATTENTION: Save as:")?);

            if new_name.is_none() {
                self.status_msg = StatusMsg::from("[!] ERROR: Save aborted...".to_string());
                return;
            }
        }
        
        if self.document.save().is_ok() {
            self.status_msg = "[!] ATTENTION: File saved successfully!"
                .to_string();
        } else {
            self.status_msg = StatusMsg::from("[!] ERROR: Error writing to file!"
                .to_string());
        }
    }

    fn search(&mut self)
    {
        let old_pos = self.cur_pos.clone();
        let mut direction = SearchDirection::Forward;

        if let query = self.prompt("[!] SEARCH (ESC to quit, Arrows to navigate): ", |ed, key, query| {
            let mut moved = false;

            match key {
                Key::Right | Key::Down => {
                    direction = SearchDirection::Forward;
                    ed.move_cursor(Key::Right);
                    moved = true;
                },
                Key::Left | Key::Up => direction = SearchDirection::Backward,
                _ => direction = SearchDirection::Forward,
            }
            if let Some(position) = ed.document.find(&query, &ed.cur_pos, direction) {
                ed.cur_pos = position;
                ed.scroll();
            } else if moved {
                ed.move_cursor(Key::Left);
            }
        },)
        .unwrap_or(None);


        if query.is_none() {
            self.cur_pos = old_pos;
            self.scroll();
        }
    }
}



fn fatal(e: std::io::Error)
{
    Terminal::clear_screen();
    panic!(e);
}