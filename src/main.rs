/**
 * ERU: A simple, lightweight, portable programming environment for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/main.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#![warn(clippy::all, clippy::pedantic)]
mod eru;
use eru::{Editor, Position);
pub use terminal::Terminal;

fn fatal(e: std::io::Error)
{
    panic!(e);
}

fn main()
{
    let _stdout = stdout()
        .into_raw_mode()
        .unwrap();

    let ed = Editor::default()
        .run();

    ed.run();
}
