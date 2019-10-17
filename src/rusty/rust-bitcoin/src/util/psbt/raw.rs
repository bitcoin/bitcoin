// Rust Bitcoin Library
// Written by
//   The Rust Bitcoin developers
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

//! # Raw PSBT Key-Value Pairs
//!
//! Raw PSBT key-value pairs as defined at
//! https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki.

use std::{fmt, io};

use consensus::encode::{self, Decodable, Encodable, VarInt, MAX_VEC_SIZE};
use hashes::hex::ToHex;
use util::psbt::Error;

/// A PSBT key in its raw byte form.
#[derive(Debug, PartialEq, Hash, Eq, Clone, Ord, PartialOrd)]
pub struct Key {
    /// The type of this PSBT key.
    pub type_value: u8,
    /// The key itself in raw byte form.
    pub key: Vec<u8>,
}

/// A PSBT key-value pair in its raw byte form.
#[derive(Debug, PartialEq)]
pub struct Pair {
    /// The key of this key-value pair.
    pub key: Key,
    /// The value of this key-value pair in raw byte form.
    pub value: Vec<u8>,
}

impl fmt::Display for Key {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "type: {:#x}, key: {}",
            self.type_value,
            self.key[..].to_hex()
        )
    }
}

impl Decodable for Key {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        let VarInt(byte_size): VarInt = Decodable::consensus_decode(&mut d)?;

        if byte_size == 0 {
            return Err(Error::NoMorePairs.into());
        }

        let key_byte_size: u64 = byte_size - 1;

        if key_byte_size > MAX_VEC_SIZE as u64 {
            return Err(encode::Error::OversizedVectorAllocation {
                requested: key_byte_size as usize,
                max: MAX_VEC_SIZE,
            })
        }

        let type_value: u8 = Decodable::consensus_decode(&mut d)?;

        let mut key = Vec::with_capacity(key_byte_size as usize);
        for _ in 0..key_byte_size {
            key.push(Decodable::consensus_decode(&mut d)?);
        }

        Ok(Key {
            type_value: type_value,
            key: key,
        })
    }
}

impl Encodable for Key {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let mut len = 0;
        len += VarInt((self.key.len() + 1) as u64).consensus_encode(&mut s)?;

        len += self.type_value.consensus_encode(&mut s)?;

        for key in &self.key {
            len += key.consensus_encode(&mut s)?
        }

        Ok(len)
    }
}

impl Encodable for Pair {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let len = self.key.consensus_encode(&mut s)?;
        Ok(len + self.value.consensus_encode(s)?)
    }
}

impl Decodable for Pair {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        Ok(Pair {
            key: Decodable::consensus_decode(&mut d)?,
            value: Decodable::consensus_decode(d)?,
        })
    }
}
