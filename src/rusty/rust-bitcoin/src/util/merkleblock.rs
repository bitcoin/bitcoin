// Rust Bitcoin Library
// Written by
//   John L. Jegutanis
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
//
// This code was translated from merkleblock.h, merkleblock.cpp and pmt_tests.cpp
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! Merkle Block and Partial Merkle Tree
//!
//! Support proofs that transaction(s) belong to a block.
//!
//! # Examples
//!
//! ```rust
//! extern crate bitcoin;
//! use bitcoin::hashes::sha256d;
//! use bitcoin::hashes::hex::FromHex;
//! use bitcoin::{Block, MerkleBlock};
//!
//! # fn main() {
//! // Get the proof from a bitcoind by running in the terminal:
//! // $ TXID="5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2"
//! // $ bitcoin-cli gettxoutproof [\"$TXID\"]
//! let mb_bytes = Vec::from_hex("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b913719\
//!     0000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b\
//!     1b01e32f570200000002252bf9d75c4f481ebb6278d708257d1f12beb6dd30301d26c623f789b2ba6fc0e2d3\
//!     2adb5f8ca820731dff234a84e78ec30bce4ec69dbd562d0b2b8266bf4e5a0105").unwrap();
//! let mb: MerkleBlock = bitcoin::consensus::deserialize(&mb_bytes).unwrap();
//!
//! // Authenticate and extract matched transaction ids
//! let mut matches: Vec<sha256d::Hash> = vec![];
//! let mut index: Vec<u32> = vec![];
//! assert!(mb.extract_matches(&mut matches, &mut index).is_ok());
//! assert_eq!(1, matches.len());
//! assert_eq!(
//!     sha256d::Hash::from_hex(
//!         "5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2").unwrap(),
//!     matches[0]
//! );
//! assert_eq!(1, index.len());
//! assert_eq!(1, index[0]);
//! # }
//! ```

use std::collections::HashSet;
use std::io;

use hashes::{sha256d, Hash};

use blockdata::constants::{MAX_BLOCK_WEIGHT, MIN_TRANSACTION_WEIGHT};
use consensus::encode::{self, Decodable, Encodable};
use util::hash::BitcoinHash;
use util::merkleblock::MerkleBlockError::*;
use {Block, BlockHeader};

/// An error when verifying the merkle block
#[derive(Clone, PartialEq, Eq, Debug)]
pub enum MerkleBlockError {
    /// When header merkle root don't match to the root calculated from the partial merkle tree
    MerkleRootMismatch,
    /// When partial merkle tree contains no transactions
    NoTransactions,
    /// When there are too many transactions
    TooManyTransactions,
    /// General format error
    BadFormat(String),
}

/// Data structure that represents a partial merkle tree.
///
/// It represents a subset of the txid's of a known block, in a way that
/// allows recovery of the list of txid's and the merkle root, in an
/// authenticated way.
///
/// The encoding works as follows: we traverse the tree in depth-first order,
/// storing a bit for each traversed node, signifying whether the node is the
/// parent of at least one matched leaf txid (or a matched txid itself). In
/// case we are at the leaf level, or this bit is 0, its merkle node hash is
/// stored, and its children are not explored further. Otherwise, no hash is
/// stored, but we recurse into both (or the only) child branch. During
/// decoding, the same depth-first traversal is performed, consuming bits and
/// hashes as they written during encoding.
///
/// The serialization is fixed and provides a hard guarantee about the
/// encoded size:
///
///   SIZE <= 10 + ceil(32.25*N)
///
/// Where N represents the number of leaf nodes of the partial tree. N itself
/// is bounded by:
///
///   N <= total_transactions
///   N <= 1 + matched_transactions*tree_height
///
/// The serialization format:
///  - uint32     total_transactions (4 bytes)
///  - varint     number of hashes   (1-3 bytes)
///  - uint256[]  hashes in depth-first order (<= 32*N bytes)
///  - varint     number of bytes of flag bits (1-3 bytes)
///  - byte[]     flag bits, packed per 8 in a byte, least significant bit first (<= 2*N-1 bits)
/// The size constraints follow from this.
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct PartialMerkleTree {
    /// The total number of transactions in the block
    num_transactions: u32,
    /// node-is-parent-of-matched-txid bits
    bits: Vec<bool>,
    /// Transaction ids and internal hashes
    hashes: Vec<sha256d::Hash>,
}

impl PartialMerkleTree {
    /// Construct a partial merkle tree
    /// The `txids` are the transaction hashes of the block and the `matches` is the contains flags
    /// wherever a tx hash should be included in the proof.
    ///
    /// Panics when `txids` is empty or when `matches` has a different length
    ///
    /// # Examples
    ///
    /// ```rust
    /// extern crate bitcoin;
    /// use bitcoin::hashes::sha256d;
    /// use bitcoin::hashes::hex::FromHex;
    /// use bitcoin::util::merkleblock::PartialMerkleTree;
    ///
    /// # fn main() {
    /// // Block 80000
    /// let txids: Vec<sha256d::Hash> = [
    ///     "c06fbab289f723c6261d3030ddb6be121f7d2508d77862bb1e484f5cd7f92b25",
    ///     "5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2",
    /// ]
    /// .iter()
    /// .map(|hex| sha256d::Hash::from_hex(hex).unwrap())
    /// .collect();
    ///
    /// // Select the second transaction
    /// let matches = vec![false, true];
    /// let tree = PartialMerkleTree::from_txids(&txids, &matches);
    /// assert!(tree.extract_matches(&mut vec![], &mut vec![]).is_ok());
    /// # }
    /// ```
    pub fn from_txids(txids: &[sha256d::Hash], matches: &[bool]) -> Self {
        // We can never have zero txs in a merkle block, we always need the coinbase tx
        assert_ne!(txids.len(), 0);
        assert_eq!(txids.len(), matches.len());

        let mut pmt = PartialMerkleTree {
            num_transactions: txids.len() as u32,
            bits: Vec::with_capacity(txids.len()),
            hashes: vec![],
        };
        // calculate height of tree
        let mut height = 0;
        while pmt.calc_tree_width(height) > 1 {
            height += 1;
        }
        // traverse the partial tree
        pmt.traverse_and_build(height, 0, txids, matches);
        pmt
    }

    /// Extract the matching txid's represented by this partial merkle tree
    /// and their respective indices within the partial tree.
    /// returns the merkle root, or error in case of failure
    pub fn extract_matches(
        &self,
        matches: &mut Vec<sha256d::Hash>,
        indexes: &mut Vec<u32>,
    ) -> Result<sha256d::Hash, MerkleBlockError> {
        matches.clear();
        indexes.clear();
        // An empty set will not work
        if self.num_transactions == 0 {
            return Err(NoTransactions);
        };
        // check for excessively high numbers of transactions
        if self.num_transactions > MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT {
            return Err(TooManyTransactions);
        }
        // there can never be more hashes provided than one for every txid
        if self.hashes.len() as u32 > self.num_transactions {
            return Err(BadFormat(
                "Proof contains more hashes than transactions".to_owned(),
            ));
        };
        // there must be at least one bit per node in the partial tree, and at least one node per hash
        if self.bits.len() < self.hashes.len() {
            return Err(BadFormat("Proof contains less bits than hashes".to_owned()));
        };
        // calculate height of tree
        let mut height = 0;
        while self.calc_tree_width(height) > 1 {
            height += 1;
        }
        // traverse the partial tree
        let mut bits_used = 0u32;
        let mut hash_used = 0u32;
        let hash_merkle_root =
            self.traverse_and_extract(height, 0, &mut bits_used, &mut hash_used, matches, indexes)?;
        // Verify that all bits were consumed (except for the padding caused by
        // serializing it as a byte sequence)
        if (bits_used + 7) / 8 != (self.bits.len() as u32 + 7) / 8 {
            return Err(BadFormat("Not all bit were consumed".to_owned()));
        }
        // Verify that all hashes were consumed
        if hash_used != self.hashes.len() as u32 {
            return Err(BadFormat("Not all hashes were consumed".to_owned()));
        }
        Ok(hash_merkle_root)
    }

    /// Helper function to efficiently calculate the number of nodes at given height
    /// in the merkle tree
    #[inline]
    fn calc_tree_width(&self, height: u32) -> u32 {
        (self.num_transactions + (1 << height) - 1) >> height
    }

    /// Calculate the hash of a node in the merkle tree (at leaf level: the txid's themselves)
    fn calc_hash(&self, height: u32, pos: u32, txids: &[sha256d::Hash]) -> sha256d::Hash {
        if height == 0 {
            // Hash at height 0 is the txid itself
            txids[pos as usize]
        } else {
            // Calculate left hash
            let left = self.calc_hash(height - 1, pos * 2, txids);
            // Calculate right hash if not beyond the end of the array - copy left hash otherwise
            let right = if pos * 2 + 1 < self.calc_tree_width(height - 1) {
                self.calc_hash(height - 1, pos * 2 + 1, txids)
            } else {
                left
            };
            // Combine subhashes
            PartialMerkleTree::parent_hash(left, right)
        }
    }

    /// Recursive function that traverses tree nodes, storing the data as bits and hashes
    fn traverse_and_build(
        &mut self,
        height: u32,
        pos: u32,
        txids: &[sha256d::Hash],
        matches: &[bool],
    ) {
        // Determine whether this node is the parent of at least one matched txid
        let mut parent_of_match = false;
        let mut p = pos << height;
        while p < (pos + 1) << height && p < self.num_transactions {
            parent_of_match |= matches[p as usize];
            p += 1;
        }
        // Store as flag bit
        self.bits.push(parent_of_match);

        if height == 0 || !parent_of_match {
            // If at height 0, or nothing interesting below, store hash and stop
            let hash = self.calc_hash(height, pos, txids);
            self.hashes.push(hash);
        } else {
            // Otherwise, don't store any hash, but descend into the subtrees
            self.traverse_and_build(height - 1, pos * 2, txids, matches);
            if pos * 2 + 1 < self.calc_tree_width(height - 1) {
                self.traverse_and_build(height - 1, pos * 2 + 1, txids, matches);
            }
        }
    }

    /// Recursive function that traverses tree nodes, consuming the bits and hashes produced by
    /// TraverseAndBuild. It returns the hash of the respective node and its respective index.
    fn traverse_and_extract(
        &self,
        height: u32,
        pos: u32,
        bits_used: &mut u32,
        hash_used: &mut u32,
        matches: &mut Vec<sha256d::Hash>,
        indexes: &mut Vec<u32>,
    ) -> Result<sha256d::Hash, MerkleBlockError> {
        if *bits_used as usize >= self.bits.len() {
            return Err(BadFormat("Overflowed the bits array".to_owned()));
        }
        let parent_of_match = self.bits[*bits_used as usize];
        *bits_used += 1;
        if height == 0 || !parent_of_match {
            // If at height 0, or nothing interesting below, use stored hash and do not descend
            if *hash_used as usize >= self.hashes.len() {
                return Err(BadFormat("Overflowed the hash array".to_owned()));
            }
            let hash = self.hashes[*hash_used as usize];
            *hash_used += 1;
            if height == 0 && parent_of_match {
                // in case of height 0, we have a matched txid
                matches.push(hash);
                indexes.push(pos);
            }
            Ok(hash)
        } else {
            // otherwise, descend into the subtrees to extract matched txids and hashes
            let left = self.traverse_and_extract(
                height - 1,
                pos * 2,
                bits_used,
                hash_used,
                matches,
                indexes,
            )?;
            let right;
            if pos * 2 + 1 < self.calc_tree_width(height - 1) {
                right = self.traverse_and_extract(
                    height - 1,
                    pos * 2 + 1,
                    bits_used,
                    hash_used,
                    matches,
                    indexes,
                )?;
                if right == left {
                    // The left and right branches should never be identical, as the transaction
                    // hashes covered by them must each be unique.
                    return Err(BadFormat("Found identical transaction hashes".to_owned()));
                }
            } else {
                right = left;
            }
            // and combine them before returning
            Ok(PartialMerkleTree::parent_hash(left, right))
        }
    }

    /// Helper method to produce SHA256D(left + right)
    fn parent_hash(left: sha256d::Hash, right: sha256d::Hash) -> sha256d::Hash {
        let mut encoder = sha256d::Hash::engine();
        left.consensus_encode(&mut encoder).unwrap();
        right.consensus_encode(&mut encoder).unwrap();
        sha256d::Hash::from_engine(encoder)
    }
}

impl Encodable for PartialMerkleTree {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let ret = self.num_transactions.consensus_encode(&mut s)?
            + self.hashes.consensus_encode(&mut s)?;
        let mut bytes: Vec<u8> = vec![0; (self.bits.len() + 7) / 8];
        for p in 0..self.bits.len() {
            bytes[p / 8] |= (self.bits[p] as u8) << (p % 8) as u8;
        }
        Ok(ret + bytes.consensus_encode(s)?)
    }
}

impl Decodable for PartialMerkleTree {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        let num_transactions: u32 = Decodable::consensus_decode(&mut d)?;
        let hashes: Vec<sha256d::Hash> = Decodable::consensus_decode(&mut d)?;

        let bytes: Vec<u8> = Decodable::consensus_decode(d)?;
        let mut bits: Vec<bool> = vec![false; bytes.len() * 8];

        for (p, bit) in bits.iter_mut().enumerate() {
            *bit = (bytes[p / 8] & (1 << (p % 8) as u8)) != 0;
        }
        Ok(PartialMerkleTree {
            num_transactions,
            hashes,
            bits,
        })
    }
}

/// Data structure that represents a block header paired to a partial merkle tree.
///
/// NOTE: This assumes that the given Block has *at least* 1 transaction. If the Block has 0 txs,
/// it will hit an assertion.
#[derive(PartialEq, Eq, Clone, Debug)]
pub struct MerkleBlock {
    /// The block header
    pub header: BlockHeader,
    /// Transactions making up a partial merkle tree
    pub txn: PartialMerkleTree,
}

impl MerkleBlock {
    /// Create a MerkleBlock from a block, that should contain proofs for the txids.
    ///
    /// The `block` is a full block containing the header and transactions and `match_txids` is a
    /// set containing the transaction ids that should be included in the partial merkle tree.
    ///
    /// # Examples
    ///
    /// ```rust
    /// extern crate bitcoin;
    /// use bitcoin::hashes::sha256d;
    /// use bitcoin::hashes::hex::FromHex;
    /// use bitcoin::{Block, MerkleBlock};
    ///
    /// # fn main() {
    /// // Block 80000
    /// let block_bytes = Vec::from_hex("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad2\
    ///     7b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33\
    ///     a5914ce6ed5b1b01e32f5702010000000100000000000000000000000000000000000000000000000000\
    ///     00000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287e\
    ///     ff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413\
    ///     bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9d\
    ///     c12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aee\
    ///     d3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d\
    ///     5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b6\
    ///     5d35549d88ac00000000").unwrap();
    /// let block: Block = bitcoin::consensus::deserialize(&block_bytes).unwrap();
    ///
    /// // Create a merkle block containing a single transaction
    /// let txid = sha256d::Hash::from_hex(
    ///     "5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2").unwrap();
    /// let match_txids = vec![txid].into_iter().collect();
    /// let mb = MerkleBlock::from_block(&block, &match_txids);
    ///
    /// // Authenticate and extract matched transaction ids
    /// let mut matches: Vec<sha256d::Hash> = vec![];
    /// let mut index: Vec<u32> = vec![];
    /// assert!(mb.extract_matches(&mut matches, &mut index).is_ok());
    /// assert_eq!(txid, matches[0]);
    /// # }
    /// ```
    pub fn from_block(block: &Block, match_txids: &HashSet<sha256d::Hash>) -> Self {
        let header = block.header;

        let mut matches: Vec<bool> = Vec::with_capacity(block.txdata.len());
        let mut hashes: Vec<sha256d::Hash> = Vec::with_capacity(block.txdata.len());

        for hash in block.txdata.iter().map(BitcoinHash::bitcoin_hash) {
            matches.push(match_txids.contains(&hash));
            hashes.push(hash);
        }

        let pmt = PartialMerkleTree::from_txids(&hashes, &matches);
        MerkleBlock { header, txn: pmt }
    }

    /// Extract the matching txid's represented by this partial merkle tree
    /// and their respective indices within the partial tree.
    /// returns Ok(()) on success, or error in case of failure
    pub fn extract_matches(
        &self,
        matches: &mut Vec<sha256d::Hash>,
        indexes: &mut Vec<u32>,
    ) -> Result<(), MerkleBlockError> {
        let merkle_root = self.txn.extract_matches(matches, indexes)?;

        if merkle_root.eq(&self.header.merkle_root) {
            Ok(())
        } else {
            Err(MerkleRootMismatch)
        }
    }
}

impl Encodable for MerkleBlock {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let len = self.header.consensus_encode(&mut s)?
            + self.txn.consensus_encode(s)?;
        Ok(len)
    }
}

impl Decodable for MerkleBlock {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        Ok(MerkleBlock {
            header: Decodable::consensus_decode(&mut d)?,
            txn: Decodable::consensus_decode(d)?,
        })
    }
}

#[cfg(test)]
mod tests {
    use std::cmp::min;

    use hashes::hex::{FromHex, ToHex};
    use hashes::{sha256d, Hash};
    use secp256k1::rand::prelude::*;

    use consensus::encode::{deserialize, serialize};
    use util::hash::{bitcoin_merkle_root, BitcoinHash};
    use util::merkleblock::{MerkleBlock, PartialMerkleTree};
    use {hex, Block};

    #[test]
    fn pmt_tests() {
        let mut rng = thread_rng();
        let tx_counts = vec![1, 4, 7, 17, 56, 100, 127, 256, 312, 513, 1000, 4095];

        for num_tx in tx_counts {
            // Create some fake tx ids
            let txids = (1..num_tx + 1) // change to `1..=num_tx` when min Rust >= 1.26.0
                .map(|i| sha256d::Hash::from_hex(&format!("{:064x}", i)).unwrap())
                .collect::<Vec<_>>();

            // Calculate the merkle root and height
            let merkle_root_1 = bitcoin_merkle_root(txids.clone());
            let mut height = 1;
            let mut ntx = num_tx;
            while ntx > 1 {
                ntx = (ntx + 1) / 2;
                height += 1;
            }

            // Check with random subsets with inclusion chances 1, 1/2, 1/4, ..., 1/128
            for att in 1..15 {
                let mut matches = vec![false; num_tx];
                let mut match_txid1 = vec![];
                for j in 0..num_tx {
                    // Generate `att / 2` random bits
                    let rand_bits = match att / 2 {
                        0 => 0,
                        bits => rng.gen::<u64>() >> (64 - bits),
                    };
                    let include = rand_bits == 0;
                    matches[j] = include;

                    if include {
                        match_txid1.push(txids[j]);
                    };
                }

                // Build the partial merkle tree
                let pmt1 = PartialMerkleTree::from_txids(&txids, &matches);
                let serialized = serialize(&pmt1);

                // Verify PartialMerkleTree's size guarantees
                let n = min(num_tx, 1 + match_txid1.len() * height);
                assert!(serialized.len() <= 10 + (258 * n + 7) / 8);

                // Deserialize into a tester copy
                let pmt2: PartialMerkleTree =
                    deserialize(&serialized).expect("Could not deserialize own data");

                // Extract merkle root and matched txids from copy
                let mut match_txid2 = vec![];
                let mut indexes = vec![];
                let merkle_root_2 = pmt2
                    .extract_matches(&mut match_txid2, &mut indexes)
                    .expect("Could not extract matches");

                // Check that it has the same merkle root as the original, and a valid one
                assert_eq!(merkle_root_1, merkle_root_2);
                assert_ne!(merkle_root_2, sha256d::Hash::default());

                // check that it contains the matched transactions (in the same order!)
                assert_eq!(match_txid1, match_txid2);

                // check that random bit flips break the authentication
                for _ in 0..4 {
                    let mut pmt3: PartialMerkleTree = deserialize(&serialized).unwrap();
                    pmt3.damage(&mut rng);
                    let mut match_txid3 = vec![];
                    let merkle_root_3 = pmt3
                        .extract_matches(&mut match_txid3, &mut indexes)
                        .unwrap();
                    assert_ne!(merkle_root_3, merkle_root_1);
                }
            }
        }
    }

    #[test]
    fn pmt_malleability() {
        // Create some fake tx ids with the last 2 hashes repeating
        let txids: Vec<sha256d::Hash> = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 10]
            .iter()
            .map(|i| sha256d::Hash::from_hex(&format!("{:064x}", i)).unwrap())
            .collect();

        let matches = vec![
            false, false, false, false, false, false, false, false, false, true, true, false,
        ];

        let tree = PartialMerkleTree::from_txids(&txids, &matches);
        // Should fail due to duplicate txs found
        let result = tree.extract_matches(&mut vec![], &mut vec![]);
        assert!(result.is_err());
    }

    #[test]
    fn merkleblock_serialization() {
        // Got it by running the rpc call
        // `gettxoutproof '["220ebc64e21abece964927322cba69180ed853bb187fbc6923bac7d010b9d87a"]'`
        let mb_hex =
            "0100000090f0a9f110702f808219ebea1173056042a714bad51b916cb6800000000000005275289558f51c\
            9966699404ae2294730c3c9f9bda53523ce50e9b95e558da2fdb261b4d4c86041b1ab1bf930900000005fac\
            7708a6e81b2a986dea60db2663840ed141130848162eb1bd1dee54f309a1b2ee1e12587e497ada70d9bd10d\
            31e83f0a924825b96cb8d04e8936d793fb60db7ad8b910d0c7ba2369bc7f18bb53d80e1869ba2c32274996c\
            ebe1ae264bc0e2289189ff0316cdc10511da71da757e553cada9f3b5b1434f3923673adb57d83caac392c38\
            af156d6fc30b55fad4112df2b95531e68114e9ad10011e72f7b7cfdb025700";

        let mb: MerkleBlock = deserialize(&hex::decode(mb_hex).unwrap()).unwrap();
        assert_eq!(get_block_13b8a().bitcoin_hash(), mb.header.bitcoin_hash());
        assert_eq!(
            mb.header.merkle_root,
            mb.txn.extract_matches(&mut vec![], &mut vec![]).unwrap()
        );
        // Serialize again and check that it matches the original bytes
        assert_eq!(mb_hex, serialize(&mb).to_hex().as_str());
    }

    /// Create a CMerkleBlock using a list of txids which will be found in the
    /// given block.
    #[test]
    fn merkleblock_construct_from_txids_found() {
        let block = get_block_13b8a();

        let txids: Vec<sha256d::Hash> = [
            "74d681e0e03bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20",
            "f9fc751cb7dc372406a9f8d738d5e6f8f63bab71986a39cf36ee70ee17036d07",
        ]
        .iter()
        .map(|hex| sha256d::Hash::from_hex(hex).unwrap())
        .collect();

        let txid1 = txids[0];
        let txid2 = txids[1];
        let txids = txids.into_iter().collect();

        let merkle_block = MerkleBlock::from_block(&block, &txids);

        assert_eq!(merkle_block.header.bitcoin_hash(), block.bitcoin_hash());

        let mut matches: Vec<sha256d::Hash> = vec![];
        let mut index: Vec<u32> = vec![];

        assert_eq!(
            merkle_block
                .txn
                .extract_matches(&mut matches, &mut index)
                .unwrap(),
            block.header.merkle_root
        );
        assert_eq!(matches.len(), 2);

        // Ordered by occurrence in depth-first tree traversal.
        assert_eq!(matches[0], txid2);
        assert_eq!(index[0], 1);

        assert_eq!(matches[1], txid1);
        assert_eq!(index[1], 8);
    }

    /// Create a CMerkleBlock using a list of txids which will not be found in the given block
    #[test]
    fn merkleblock_construct_from_txids_not_found() {
        let block = get_block_13b8a();
        let txids = ["c0ffee00003bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20"]
            .iter()
            .map(|hex| sha256d::Hash::from_hex(hex).unwrap())
            .collect();

        let merkle_block = MerkleBlock::from_block(&block, &txids);

        assert_eq!(merkle_block.header.bitcoin_hash(), block.bitcoin_hash());

        let mut matches: Vec<sha256d::Hash> = vec![];
        let mut index: Vec<u32> = vec![];

        assert_eq!(
            merkle_block
                .txn
                .extract_matches(&mut matches, &mut index)
                .unwrap(),
            block.header.merkle_root
        );
        assert_eq!(matches.len(), 0);
        assert_eq!(index.len(), 0);
    }

    impl PartialMerkleTree {
        /// Flip one bit in one of the hashes - this should break the authentication
        fn damage(&mut self, rng: &mut ThreadRng) {
            let n = rng.gen_range(0, self.hashes.len());
            let bit = rng.gen::<u8>();
            let hashes = &mut self.hashes;
            let mut hash = hashes[n].into_inner();
            hash[(bit >> 3) as usize] ^= 1 << (bit & 7);
            hashes[n] = sha256d::Hash::from_slice(&hash).unwrap();
        }
    }

    /// Returns a real block (0000000000013b8ab2cd513b0261a14096412195a72a0c4827d229dcc7e0f7af)
    /// with 9 txs.
    fn get_block_13b8a() -> Block {
        let block_hex =
            "0100000090f0a9f110702f808219ebea1173056042a714bad51b916cb6800000000000005275289558f51c\
            9966699404ae2294730c3c9f9bda53523ce50e9b95e558da2fdb261b4d4c86041b1ab1bf930901000000010\
            000000000000000000000000000000000000000000000000000000000000000ffffffff07044c86041b0146\
            ffffffff0100f2052a01000000434104e18f7afbe4721580e81e8414fc8c24d7cfacf254bb5c7b949450c3e\
            997c2dc1242487a8169507b631eb3771f2b425483fb13102c4eb5d858eef260fe70fbfae0ac000000000100\
            00000196608ccbafa16abada902780da4dc35dafd7af05fa0da08cf833575f8cf9e836000000004a4930460\
            22100dab24889213caf43ae6adc41cf1c9396c08240c199f5225acf45416330fd7dbd022100fe37900e0644\
            bf574493a07fc5edba06dbc07c311b947520c2d514bc5725dcb401ffffffff0100f2052a010000001976a91\
            4f15d1921f52e4007b146dfa60f369ed2fc393ce288ac000000000100000001fb766c1288458c2bafcfec81\
            e48b24d98ec706de6b8af7c4e3c29419bfacb56d000000008c493046022100f268ba165ce0ad2e6d93f089c\
            fcd3785de5c963bb5ea6b8c1b23f1ce3e517b9f022100da7c0f21adc6c401887f2bfd1922f11d76159cbc59\
            7fbd756a23dcbb00f4d7290141042b4e8625a96127826915a5b109852636ad0da753c9e1d5606a50480cd0c\
            40f1f8b8d898235e571fe9357d9ec842bc4bba1827daaf4de06d71844d0057707966affffffff0280969800\
            000000001976a9146963907531db72d0ed1a0cfb471ccb63923446f388ac80d6e34c000000001976a914f06\
            88ba1c0d1ce182c7af6741e02658c7d4dfcd388ac000000000100000002c40297f730dd7b5a99567eb8d27b\
            78758f607507c52292d02d4031895b52f2ff010000008b483045022100f7edfd4b0aac404e5bab4fd3889e0\
            c6c41aa8d0e6fa122316f68eddd0a65013902205b09cc8b2d56e1cd1f7f2fafd60a129ed94504c4ac7bdc67\
            b56fe67512658b3e014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5\
            d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffffca5065ff9617cbcb\
            a45eb23726df6498a9b9cafed4f54cbab9d227b0035ddefb000000008a473044022068010362a13c7f9919f\
            a832b2dee4e788f61f6f5d344a7c2a0da6ae740605658022006d1af525b9a14a35c003b78b72bd59738cd67\
            6f845d1ff3fc25049e01003614014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c34\
            23e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffff01001e\
            c4110200000043410469ab4181eceb28985b9b4e895c13fa5e68d85761b7eee311db5addef76fa862186513\
            4a221bd01f28ec9999ee3e021e60766e9d1f3458c115fb28650605f11c9ac000000000100000001cdaf2f75\
            8e91c514655e2dc50633d1e4c84989f8aa90a0dbc883f0d23ed5c2fa010000008b48304502207ab51be6f12\
            a1962ba0aaaf24a20e0b69b27a94fac5adf45aa7d2d18ffd9236102210086ae728b370e5329eead9accd880\
            d0cb070aea0c96255fae6c4f1ddcce1fd56e014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf3\
            3d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffff\
            ffff02404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac002d310100000\
            0001976a9141befba0cdc1ad56529371864d9f6cb042faa06b588ac000000000100000001b4a47603e71b61\
            bc3326efd90111bf02d2f549b067f4c4a8fa183b57a0f800cb010000008a4730440220177c37f9a505c3f1a\
            1f0ce2da777c339bd8339ffa02c7cb41f0a5804f473c9230220585b25a2ee80eb59292e52b987dad92acb0c\
            64eced92ed9ee105ad153cdb12d001410443bd44f683467e549dae7d20d1d79cbdb6df985c6e9c029c8d0c6\
            cb46cc1a4d3cf7923c5021b27f7a0b562ada113bc85d5fda5a1b41e87fe6e8802817cf69996ffffffff0280\
            651406000000001976a9145505614859643ab7b547cd7f1f5e7e2a12322d3788ac00aa0271000000001976a\
            914ea4720a7a52fc166c55ff2298e07baf70ae67e1b88ac00000000010000000586c62cd602d219bb60edb1\
            4a3e204de0705176f9022fe49a538054fb14abb49e010000008c493046022100f2bc2aba2534becbdf062eb\
            993853a42bbbc282083d0daf9b4b585bd401aa8c9022100b1d7fd7ee0b95600db8535bbf331b19eed8d961f\
            7a8e54159c53675d5f69df8c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d\
            0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff03ad0e58\
            ccdac3df9dc28a218bcf6f1997b0a93306faaa4b3a28ae83447b2179010000008b483045022100be12b2937\
            179da88599e27bb31c3525097a07cdb52422d165b3ca2f2020ffcf702200971b51f853a53d644ebae9ec8f3\
            512e442b1bcb6c315a5b491d119d10624c83014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf3\
            3d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffff\
            ffff2acfcab629bbc8685792603762c921580030ba144af553d271716a95089e107b010000008b483045022\
            100fa579a840ac258871365dd48cd7552f96c8eea69bd00d84f05b283a0dab311e102207e3c0ee9234814cf\
            bb1b659b83671618f45abc1326b9edcc77d552a4f2a805c0014104462e76fd4067b3a0aa42070082dcb0bf2\
            f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8eb\
            bb12dcd4ffffffffdcdc6023bbc9944a658ddc588e61eacb737ddf0a3cd24f113b5a8634c517fcd20000000\
            08b4830450221008d6df731df5d32267954bd7d2dda2302b74c6c2a6aa5c0ca64ecbabc1af03c75022010e5\
            5c571d65da7701ae2da1956c442df81bbf076cdbac25133f99d98a9ed34c014104462e76fd4067b3a0aa420\
            70082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15\
            312ef1c0e8ebbb12dcd4ffffffffe15557cd5ce258f479dfd6dc6514edf6d7ed5b21fcfa4a038fd69f06b83\
            ac76e010000008b483045022023b3e0ab071eb11de2eb1cc3a67261b866f86bf6867d4558165f7c8c8aca2d\
            86022100dc6e1f53a91de3efe8f63512850811f26284b62f850c70ca73ed5de8771fb451014104462e76fd4\
            067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812\
            b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff01404b4c00000000001976a9142b6ba7c9d796b75eef794\
            2fc9288edd37c32f5c388ac00000000010000000166d7577163c932b4f9690ca6a80b6e4eb001f0a2fa9023\
            df5595602aae96ed8d000000008a4730440220262b42546302dfb654a229cefc86432b89628ff259dc87edd\
            1154535b16a67e102207b4634c020a97c3e7bbd0d4d19da6aa2269ad9dded4026e896b213d73ca4b63f0141\
            04979b82d02226b3a4597523845754d44f13639e3bf2df5e82c6aab2bdc79687368b01b1ab8b19875ae3c90\
            d661a3d0a33161dab29934edeb36aa01976be3baf8affffffff02404b4c00000000001976a9144854e695a0\
            2af0aeacb823ccbc272134561e0a1688ac40420f00000000001976a914abee93376d6b37b5c2940655a6fca\
            f1c8e74237988ac0000000001000000014e3f8ef2e91349a9059cb4f01e54ab2597c1387161d3da89919f7e\
            a6acdbb371010000008c49304602210081f3183471a5ca22307c0800226f3ef9c353069e0773ac76bb58065\
            4d56aa523022100d4c56465bdc069060846f4fbf2f6b20520b2a80b08b168b31e66ddb9c694e24001410497\
            6c79848e18251612f8940875b2b08d06e6dc73b9840e8860c066b7e87432c477e9a59a453e71e6d76d5fe34\
            058b800a098fc1740ce3012e8fc8a00c96af966ffffffff02c0e1e400000000001976a9144134e75a6fcb60\
            42034aab5e18570cf1f844f54788ac404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37\
            c32f5c388ac00000000";
        deserialize(&hex::decode(block_hex).unwrap()).unwrap()
    }
}
