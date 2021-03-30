//
  // ERU: A simple, lightweight, and portable text editor for POSIX systems.
  //
  // Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/highlight.rs }.
  // This software is distributed under the GNU General Public License Version 2.0.
  // Refer to the file LICENSE for additional details.
//

use termion::color;

#[derive(PartialEq)]
pub enum Type {
    None,
    Number,
    Match,
}

impl Type {
    pub fn to_color(&self) -> impl color::Color
    {
        match self {
            Type::Number => color::Rgb(220, 163, 163),
            Type::Match => color::Rgb(38, 139, 210),
            _ => color::Rgb(255, 255, 255),
        }
    }
}