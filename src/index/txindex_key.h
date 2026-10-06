// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_KEY_H
#define BITCOIN_INDEX_TXINDEX_KEY_H

#include <crypto/siphash.h>
#include <index/block_seq.h>
#include <primitives/transaction_identifier.h>
#include <uint256.h>

#include <cstdint>
#include <string>
#include <utility>

namespace txindex {
/*
 * Database layout:
 *
 *   ['x', hash prefix, block seq, tx offset] -> (empty)
 *   ['s', block seq]                         -> block hash
 *   ['h', block hash]                        -> block seq
 *   ["next_block_seq"]                       -> next block seq to assign
 *   ["txid_hash_salt"]                       -> txid hasher salt
 *   ["best_block_v2"]                        -> current sync locator
 *   ['t', txid]                              -> legacy CDiskTxPos
 *   ['B']                                    -> legacy sync locator
 */

constexpr uint8_t DB_TXINDEX_HASHED{'x'};
constexpr uint8_t DB_BLOCK_SEQ{'s'};
constexpr uint8_t DB_BLOCK_HASH{'h'};
inline const std::string DB_TXID_HASH_SALT{"txid_hash_salt"};
//! Prefix of a legacy (pre-hashing) txindex row.
constexpr uint8_t DB_TXINDEX{'t'};

using block_seq::BLOCK_HEADER_SIZE;
using block_seq::BlockTxPosition;
using block_seq::EMPTY_VALUE;
using BlockSeqKey = block_seq::BlockSeqKey<DB_BLOCK_SEQ>;
using BlockHashKey = block_seq::BlockHashKey<DB_BLOCK_HASH>;
using DBKey = block_seq::HashedPositionKey<DB_TXINDEX_HASHED>;
using TxHashKeyPrefix = block_seq::HashKeyPrefix;

inline TxHashKeyPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const Txid& txid)
{
    return block_seq::TruncateHash(hasher.Hash(txid.ToUint256()));
}

//! Key of a legacy (pre-hashing) txindex row: the full txid under the 't' prefix.
inline std::pair<uint8_t, uint256> LegacyTxKey(const Txid& txid)
{
    return {DB_TXINDEX, txid.ToUint256()};
}

} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
