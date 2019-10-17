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

//! Blockdata network messages
//!
//! This module describes network messages which are used for passing
//! Bitcoin data (blocks and transactions) around.
//!

use network::constants;
use consensus::encode::{self, Decodable, Encodable};
use hashes::sha256d;

use std::io;

#[derive(PartialEq, Eq, Clone, Debug)]
/// The type of an inventory object
pub enum InvType {
    /// Error --- these inventories can be ignored
    Error,
    /// Transaction
    Transaction,
    /// Block
    Block,
    /// Witness Block
    WitnessBlock,
    /// Witness Transaction
    WitnessTransaction
}

// Some simple messages

/// The `getblocks` message
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct GetBlocksMessage {
    /// The protocol version
    pub version: u32,
    /// Locator hashes --- ordered newest to oldest. The remote peer will
    /// reply with its longest known chain, starting from a locator hash
    /// if possible and block 1 otherwise.
    pub locator_hashes: Vec<sha256d::Hash>,
    /// References the block to stop at, or zero to just fetch the maximum 500 blocks
    pub stop_hash: sha256d::Hash,
}

/// The `getheaders` message
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct GetHeadersMessage {
    /// The protocol version
    pub version: u32,
    /// Locator hashes --- ordered newest to oldest. The remote peer will
    /// reply with its longest known chain, starting from a locator hash
    /// if possible and block 1 otherwise.
    pub locator_hashes: Vec<sha256d::Hash>,
    /// References the header to stop at, or zero to just fetch the maximum 2000 headers
    pub stop_hash: sha256d::Hash
}

/// An inventory object --- a reference to a Bitcoin object
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct Inventory {
    /// The type of object that is referenced
    pub inv_type: InvType,
    /// The object's hash
    pub hash: sha256d::Hash
}

impl GetBlocksMessage {
    /// Construct a new `getblocks` message
    pub fn new(locator_hashes: Vec<sha256d::Hash>, stop_hash: sha256d::Hash) -> GetBlocksMessage {
        GetBlocksMessage {
            version: constants::PROTOCOL_VERSION,
            locator_hashes: locator_hashes.clone(),
            stop_hash: stop_hash
        }
    }
}

impl_consensus_encoding!(GetBlocksMessage, version, locator_hashes, stop_hash);

impl GetHeadersMessage {
    /// Construct a new `getheaders` message
    pub fn new(locator_hashes: Vec<sha256d::Hash>, stop_hash: sha256d::Hash) -> GetHeadersMessage {
        GetHeadersMessage {
            version: constants::PROTOCOL_VERSION,
            locator_hashes: locator_hashes,
            stop_hash: stop_hash
        }
    }
}

impl_consensus_encoding!(GetHeadersMessage, version, locator_hashes, stop_hash);

impl Encodable for Inventory {
    #[inline]
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let inv_len = match self.inv_type {
            InvType::Error => 0u32,
            InvType::Transaction => 1,
            InvType::Block => 2,
            InvType::WitnessBlock => 0x40000002,
            InvType::WitnessTransaction => 0x40000001
        }.consensus_encode(&mut s)?;
        Ok(inv_len + self.hash.consensus_encode(&mut s)?)
    }
}

impl Decodable for Inventory {
    #[inline]
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        let int_type: u32 = Decodable::consensus_decode(&mut d)?;
        Ok(Inventory {
            inv_type: match int_type {
                0 => InvType::Error,
                1 => InvType::Transaction,
                2 => InvType::Block,
                // TODO do not fail here
                _ => { panic!("bad inventory type field") }
            },
            hash: Decodable::consensus_decode(d)?
        })
    }
}

#[cfg(test)]
mod tests {
    use super::{GetHeadersMessage, GetBlocksMessage};

    use hex::decode as hex_decode;

    use consensus::encode::{deserialize, serialize};
    use std::default::Default;

    #[test]
    fn getblocks_message_test() {
        let from_sat = hex_decode("72110100014a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b0000000000000000000000000000000000000000000000000000000000000000").unwrap();
        let genhash = hex_decode("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b").unwrap();

        let decode: Result<GetBlocksMessage, _> = deserialize(&from_sat);
        assert!(decode.is_ok());
        let real_decode = decode.unwrap();
        assert_eq!(real_decode.version, 70002);
        assert_eq!(real_decode.locator_hashes.len(), 1);
        assert_eq!(serialize(&real_decode.locator_hashes[0]), genhash);
        assert_eq!(real_decode.stop_hash, Default::default());

        assert_eq!(serialize(&real_decode), from_sat);
    }

    #[test]
    fn getheaders_message_test() {
        let from_sat = hex_decode("72110100014a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b0000000000000000000000000000000000000000000000000000000000000000").unwrap();
        let genhash = hex_decode("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b").unwrap();

        let decode: Result<GetHeadersMessage, _> = deserialize(&from_sat);
        assert!(decode.is_ok());
        let real_decode = decode.unwrap();
        assert_eq!(real_decode.version, 70002);
        assert_eq!(real_decode.locator_hashes.len(), 1);
        assert_eq!(serialize(&real_decode.locator_hashes[0]), genhash);
        assert_eq!(real_decode.stop_hash, Default::default());

        assert_eq!(serialize(&real_decode), from_sat);
    }
}

