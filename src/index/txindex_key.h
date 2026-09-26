// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_KEY_H
#define BITCOIN_INDEX_TXINDEX_KEY_H

#include <consensus/consensus.h>
#include <crypto/siphash.h>
#include <primitives/transaction_identifier.h>
#include <serialize.h>
#include <uint256.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
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
inline const std::string DB_NEXT_BLOCK_SEQ{"next_block_seq"};
inline const std::string DB_TXID_HASH_SALT{"txid_hash_salt"};
inline const std::string DB_BEST_BLOCK_V2{"best_block_v2"};
//! Prefix of a legacy (pre-hashing) txindex row.
constexpr uint8_t DB_TXINDEX{'t'};

//! Empty value of a hashed txindex row, whose position is encoded in its key.
inline constexpr std::array<std::byte, 0> EMPTY_VALUE{};

//! Serialized size of a block header, the offset of the first byte after it.
constexpr uint32_t BLOCK_HEADER_SIZE{80};

//! The location of a transaction: the sequence number of the block that contains it
//! and the transaction's serialized byte offset from the start of that block
//! (including the header), so the on-disk position is simply
//! block_data_pos + tx_offset_in_block.
//!
struct BlockTxPosition {
    uint32_t block_seq{0};
    uint32_t tx_offset_in_block{0};

    friend bool operator==(const BlockTxPosition&, const BlockTxPosition&) = default;

    // tx_offset is encoded in 3-byte big-endian integer.
    // This can hold up to 16,777,216, which is >4x the maximum 4 million block weight position
    static constexpr uint32_t TX_OFFSET_SIZE{3};
    static_assert(MAX_BLOCK_SERIALIZED_SIZE <= BigEndianFormatter<TX_OFFSET_SIZE>::MAX);

    SERIALIZE_METHODS(BlockTxPosition, obj)
    {
        READWRITE(VARINT(obj.block_seq),
                  Using<BigEndianFormatter<TX_OFFSET_SIZE>>(obj.tx_offset_in_block));
    }
};

//! Key for looking up the hash of the block with the given sequence number.
struct BlockSeqKey {
    uint32_t block_seq{0};

    SERIALIZE_METHODS(BlockSeqKey, obj)
    {
        uint8_t prefix{DB_BLOCK_SEQ};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_BLOCK_SEQ) throw std::ios_base::failure("Invalid format for txindex block seq key");
        READWRITE(VARINT(obj.block_seq));
    }
};

//! Key for looking up the sequence number assigned to the block with the given hash.
struct BlockHashKey {
    uint256 block_hash;

    SERIALIZE_METHODS(BlockHashKey, obj)
    {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_BLOCK_HASH) throw std::ios_base::failure("Invalid format for txindex block hash key");
        READWRITE(obj.block_hash);
    }
};

constexpr int HASH_PREFIX_SIZE{5};
using TxHashKeyPrefix = uint64_t;

inline TxHashKeyPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const Txid& txid)
{
    return hasher.Hash(txid.ToUint256()) >> (8 * (sizeof(TxHashKeyPrefix) - HASH_PREFIX_SIZE));
}

struct DBKey {
    TxHashKeyPrefix hash_prefix{0};
    BlockTxPosition pos;

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_TXINDEX_HASHED};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_TXINDEX_HASHED) throw std::ios_base::failure("Invalid format for txindex DB key");
        READWRITE(Using<BigEndianFormatter<HASH_PREFIX_SIZE>>(obj.hash_prefix), obj.pos);
    }
};

//! Key of a legacy (pre-hashing) txindex row: the full txid under the 't' prefix.
inline std::pair<uint8_t, uint256> LegacyTxKey(const Txid& txid)
{
    return {DB_TXINDEX, txid.ToUint256()};
}

} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
