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

//! # Rust Bitcoin Library
//!
//! This is a library for which supports the Bitcoin network protocol and associated
//! primitives. It is designed for Rust programs built to work with the Bitcoin
//! network.
//!
//! It is also written entirely in Rust to illustrate the benefits of strong type
//! safety, including ownership and lifetime, for financial and/or cryptographic
//! software.
//!

#![crate_name = "bitcoin"]
#![crate_type = "dylib"]
#![crate_type = "rlib"]

// Experimental features we need
#![cfg_attr(all(test, feature = "unstable"), feature(test))]

// Clippy whitelist
#![cfg_attr(feature = "clippy", allow(needless_range_loop))] // suggests making a big mess of array newtypes
#![cfg_attr(feature = "clippy", allow(extend_from_slice))]   // `extend_from_slice` only available since 1.6

// Coding conventions
#![forbid(unsafe_code)]
#![deny(non_upper_case_globals)]
#![deny(non_camel_case_types)]
#![deny(non_snake_case)]
#![deny(unused_mut)]
#![deny(dead_code)]
#![deny(unused_imports)]
#![deny(missing_docs)]
#![forbid(unsafe_code)]

// In general, rust is absolutely horrid at supporting users doing things like,
// for example, compiling Rust code for real environments. Disable useless lints
// that don't do anything but annoy us and cant actually ever be resolved.
#![allow(bare_trait_objects)]
#![allow(ellipsis_inclusive_range_patterns)]

// Re-exported dependencies.
pub extern crate bitcoin_hashes as hashes;
pub extern crate secp256k1;
pub extern crate bech32;

#[cfg(any(test, feature = "serde"))] extern crate hex;
#[cfg(feature = "serde")] extern crate serde;
#[cfg(all(test, feature = "serde"))] #[macro_use] extern crate serde_derive; // for 1.22.0 compat
#[cfg(all(test, feature = "serde"))] extern crate serde_json;
#[cfg(all(test, feature = "serde"))] extern crate serde_test;
#[cfg(all(test, feature = "unstable"))] extern crate test;
#[cfg(feature="bitcoinconsensus")] extern crate bitcoinconsensus;

#[cfg(target_pointer_width = "16")]
compile_error!("rust-bitcoin cannot be used on 16-bit architectures");

#[cfg(test)]
#[macro_use]
mod test_macros;
#[macro_use]
mod internal_macros;
#[macro_use]
pub mod network;
pub mod blockdata;
pub mod util;
pub mod consensus;

pub use blockdata::block::Block;
pub use blockdata::block::BlockHeader;
pub use blockdata::script::Script;
pub use blockdata::transaction::Transaction;
pub use blockdata::transaction::TxIn;
pub use blockdata::transaction::TxOut;
pub use blockdata::transaction::OutPoint;
pub use blockdata::transaction::SigHashType;
pub use consensus::encode::VarInt;
pub use network::constants::Network;
pub use util::Error;
pub use util::address::Address;
pub use util::address::AddressType;
pub use util::amount::Amount;
pub use util::amount::SignedAmount;
pub use util::hash::BitcoinHash;
pub use util::key::PrivateKey;
pub use util::key::PublicKey;
pub use util::merkleblock::MerkleBlock;
