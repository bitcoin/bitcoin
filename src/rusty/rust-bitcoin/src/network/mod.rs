// Rust Bitcoin Library
// Written in 2014 by
//   Andrew Poelstra <apoelstra@wpsoftware.net>
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to
// the public domain worldwide. This software is distributed without
// any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//

//! Network Support
//!
//! This module defines support for (de)serialization and network transport 
//! of Bitcoin data and network messages.
//!

use std::fmt;
use std::io;
use std::error;

pub mod constants;

pub mod address;
pub mod message;
pub mod message_blockdata;
pub mod message_network;
pub mod message_filter;
pub mod stream_reader;

/// Network error
#[derive(Debug)]
pub enum Error {
    /// And I/O error
    Io(io::Error),
    /// Socket mutex was poisoned
    SocketMutexPoisoned,
    /// Not connected to peer
    SocketNotConnectedToPeer,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::Io(ref e) => fmt::Display::fmt(e, f),
            Error::SocketMutexPoisoned | Error::SocketNotConnectedToPeer => f.write_str(error::Error::description(self)),
        }
    }
}

#[doc(hidden)]
impl From<io::Error> for Error {
    fn from(err: io::Error) -> Self {
        Error::Io(err)
    }
}

impl error::Error for Error {
    fn description(&self) -> &str {
        match *self {
            Error::Io(ref e) => e.description(),
            Error::SocketMutexPoisoned => "socket mutex was poisoned",
            Error::SocketNotConnectedToPeer => "not connected to peer",
        }
    }

    fn cause(&self) -> Option<&error::Error> {
        match *self {
            Error::Io(ref e) => Some(e),
            Error::SocketMutexPoisoned | Error::SocketNotConnectedToPeer => None,
        }
    }
}
