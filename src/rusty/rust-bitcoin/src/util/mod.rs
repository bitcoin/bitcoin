// Rust Bitcoin Library
// Written in 2014 by
//     Andrew Poelstra <apoelstra@wpsoftware.net>
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

//! Utility functions
//!
//! Functions needed by all parts of the Bitcoin library

pub mod key;
pub mod address;
pub mod amount;
pub mod base58;
pub mod bip32;
pub mod bip143;
pub mod contracthash;
pub mod hash;
pub mod merkleblock;
pub mod misc;
pub mod psbt;
pub mod uint;
pub mod bip158;

pub(crate) mod endian;

use std::{error, fmt};

use network;
use consensus::encode;

/// A trait which allows numbers to act as fixed-size bit arrays
pub trait BitArray {
    /// Is bit set?
    fn bit(&self, idx: usize) -> bool;

    /// Returns an array which is just the bits from start to end
    fn bit_slice(&self, start: usize, end: usize) -> Self;

    /// Bitwise and with `n` ones
    fn mask(&self, n: usize) -> Self;

    /// Trailing zeros
    fn trailing_zeros(&self) -> usize;

    /// Create all-zeros value
    fn zero() -> Self;

    /// Create value representing one
    fn one() -> Self;
}

/// A general error code, other errors should implement conversions to/from this
/// if appropriate.
#[derive(Debug)]
pub enum Error {
    /// Encoding error
    Encode(encode::Error),
    /// Network error
    Network(network::Error),
    /// The header hash is not below the target
    BlockBadProofOfWork,
    /// The `target` field of a block header did not match the expected difficulty
    BlockBadTarget,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::Encode(ref e) => fmt::Display::fmt(e, f),
            Error::Network(ref e) => fmt::Display::fmt(e, f),
            Error::BlockBadProofOfWork | Error::BlockBadTarget => f.write_str(error::Error::description(self)),
        }
    }
}

impl error::Error for Error {
    fn cause(&self) -> Option<&error::Error> {
        match *self {
            Error::Encode(ref e) => Some(e),
            Error::Network(ref e) => Some(e),
            Error::BlockBadProofOfWork | Error::BlockBadTarget => None
        }
    }

    fn description(&self) -> &str {
        match *self {
            Error::Encode(ref e) => e.description(),
            Error::Network(ref e) => e.description(),
            Error::BlockBadProofOfWork => "block target correct but not attained",
            Error::BlockBadTarget => "block target incorrect",
        }
    }
}

#[doc(hidden)]
impl From<encode::Error> for Error {
    fn from(e: encode::Error) -> Error {
        Error::Encode(e)
    }
}

#[doc(hidden)]
impl From<network::Error> for Error {
    fn from(e: network::Error) -> Error {
        Error::Network(e)
    }
}
