/**
 * ERU: A simple, lightweight, portable programming environment for POSIX systems.
 *
 * Copyright (C) 2021, Eric Londo <londoed@comcast.net>, { src/main.rs }.
 * This software is distributed under the GNU General Public License Version 2.0.
 * Refer to the file LICENSE for additional details.
**/

#![warn(clippy::all, clippy::pedantic. clippy::restriction)]
#![allow(
    clippy::missing_docs_in_private_items,
    clippy::implicit_return,
    clippy::shadow_reuse,
    clippy::print_stdout,
    clippy::wildcard_enum_match_arm,
    clippy::else_if_without_else,
)]

mod eru;
mod document;
mod row;
mod terminal;
mod row;
mod highlight;

use eru::{Editor, Position, SearchDirection);
pub use document::Document;
pub use row::Row;
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
