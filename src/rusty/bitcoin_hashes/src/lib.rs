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

//! # Rust Hashes Library
//!
//! This is a simple, no-dependency library which implements the hash functions
//! needed by Bitcoin. These are SHA256, SHA256d, and RIPEMD160. As an ancillary
//! thing, it exposes hexadecimal serialization and deserialization, since these
//! are needed to display hashes anway.
//!

// Coding conventions
#![deny(non_upper_case_globals)]
#![deny(non_camel_case_types)]
#![deny(non_snake_case)]
#![deny(unused_mut)]
#![deny(missing_docs)]

#![cfg_attr(all(not(test), not(feature = "std")), no_std)]
#![cfg_attr(all(test, feature = "unstable"), feature(test))]
#[cfg(all(test, feature = "unstable"))] extern crate test;

#[cfg(any(test, feature="std"))] extern crate core;
#[cfg(feature="serde")] extern crate serde;
#[cfg(all(test,feature="serde"))] extern crate serde_test;

#[macro_use] mod util;
#[macro_use] mod serde_macros;
#[cfg(any(test, feature = "std"))] mod std_impls;
pub mod error;
pub mod hex;
pub mod hash160;
pub mod hmac;
pub mod ripemd160;
pub mod sha1;
pub mod sha256;
pub mod sha512;
pub mod sha256d;
pub mod siphash24;
pub mod cmp;

use core::{borrow, fmt, hash, ops};

pub use hmac::{Hmac, HmacEngine};
pub use error::Error;

/// A hashing engine which bytes can be serialized into. It is expected
/// to implement the `io::Write` trait, but to never return errors under
/// any conditions.
pub trait HashEngine: Clone + Default {
    /// Byte array representing the internal state of the hash engine
    type MidState;

    /// Outputs the midstate of the hash engine. This function should not be
    /// used directly unless you really know what you're doing.
    fn midstate(&self) -> Self::MidState;

    /// Length of the hash's internal block size, in bytes
    const BLOCK_SIZE: usize;

    /// Add data to the hash engine
    fn input(&mut self, data: &[u8]);
}

/// Trait which applies to hashes of all types
pub trait Hash: Copy + Clone + PartialEq + Eq + Default + PartialOrd + Ord +
    hash::Hash + fmt::Debug + fmt::Display + fmt::LowerHex +
    ops::Index<ops::RangeFull, Output = [u8]> +
    ops::Index<ops::RangeFrom<usize>, Output = [u8]> +
    ops::Index<ops::RangeTo<usize>, Output = [u8]> +
    ops::Index<ops::Range<usize>, Output = [u8]> +
    ops::Index<usize, Output = u8> +
    borrow::Borrow<[u8]>
{
    /// A hashing engine which bytes can be serialized into. It is expected
    /// to implement the `io::Write` trait, and to never return errors under
    /// any conditions.
    type Engine: HashEngine;

    /// The byte array that represents the hash internally
    type Inner: hex::FromHex;

    /// Construct a new engine
    fn engine() -> Self::Engine {
        Self::Engine::default()
    }

    /// Produce a hash from the current state of a given engine
    fn from_engine(e: Self::Engine) -> Self;

    /// Length of the hash, in bytes
    const LEN: usize;

    /// Copies a byte slice into a hash object
    fn from_slice(sl: &[u8]) -> Result<Self, Error>;

    /// Hashes some bytes
    fn hash(data: &[u8]) -> Self {
        let mut engine = Self::engine();
        engine.input(data);
        Self::from_engine(engine)
    }

    /// Flag indicating whether user-visible serializations of this hash
    /// should be backward. For some reason Satoshi decided this should be
    /// true for `Sha256dHash`, so here we are.
    const DISPLAY_BACKWARD: bool = false;

    /// Unwraps the hash and returns the underlying byte array
    fn into_inner(self) -> Self::Inner;

    /// Constructs a hash from the underlying byte array
    fn from_inner(inner: Self::Inner) -> Self;
}
