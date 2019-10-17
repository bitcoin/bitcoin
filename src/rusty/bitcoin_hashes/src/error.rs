// Bitcoin Hashes Library
// Written in 2018 by
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

//! # Error Type
//!

use core::fmt;

/// [bitcoin_hashes] error.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum Error {
    /// Tried to create a fixed-length hash from a slice with the wrong size (expected, got).
    InvalidLength(usize, usize),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::InvalidLength(ell, ell2) => write!(f, "bad slice length {} (expected {})", ell2, ell),
        }
    }
}

