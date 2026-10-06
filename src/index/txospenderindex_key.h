// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSPENDERINDEX_KEY_H
#define BITCOIN_INDEX_TXOSPENDERINDEX_KEY_H

#include <crypto/siphash.h>
#include <index/block_seq.h>
#include <index/disktxpos.h>
#include <primitives/transaction.h>
#include <serialize.h>

#include <cstdint>
#include <ios>
#include <string>

namespace txospenderindex {
/*
 * Database layout:
 *
 *   ['x', hash prefix, block seq, tx offset]   -> (empty)
 *   ['q', block seq]                           -> block hash
 *   ['h', block hash]                          -> block seq
 *   ["next_block_seq"]                         -> next block seq to assign
 *   ["outpoint_hash_salt"]                     -> outpoint hasher salt
 *   ["best_block_v2"]                          -> current sync locator
 *   ['s', siphash(outpoint), CDiskTxPos]       -> legacy entry (empty)
 *   ["siphash_key"]                            -> legacy siphash key
 *   ['B']                                      -> legacy sync locator
 *
 * Each row maps an outpoint to the position of the transaction spending it.
 * Legacy rows use the 's' prefix, so the block seq map uses 'q' instead.
 */

constexpr uint8_t DB_SPENDER_HASHED{'x'};
constexpr uint8_t DB_BLOCK_SEQ{'q'};
constexpr uint8_t DB_BLOCK_HASH{'h'};
inline const std::string DB_OUTPOINT_HASH_SALT{"outpoint_hash_salt"};
//! Prefix of a legacy (pre-hashing) spender row.
constexpr uint8_t DB_LEGACY_SPENDER{'s'};
//! Key of the legacy siphash key. Previous versions used a string literal, which
//! serializes as its raw characters including the terminating null byte.
inline constexpr char DB_LEGACY_SIPHASH_KEY[]{"siphash_key"};

using BlockSeqKey = block_seq::BlockSeqKey<DB_BLOCK_SEQ>;
using BlockHashKey = block_seq::BlockHashKey<DB_BLOCK_HASH>;
using DBKey = block_seq::HashedPositionKey<DB_SPENDER_HASHED>;

inline block_seq::HashKeyPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const COutPoint& outpoint)
{
    return block_seq::TruncateHash(hasher.Hash(outpoint.hash.ToUint256(), outpoint.n));
}

struct LegacyDBKey {
    uint64_t hash{0};
    CDiskTxPos pos;

    SERIALIZE_METHODS(LegacyDBKey, obj)
    {
        uint8_t prefix{DB_LEGACY_SPENDER};
        READWRITE(prefix);
        if (prefix != DB_LEGACY_SPENDER) {
            throw std::ios_base::failure("Invalid format for spender index DB key");
        }
        READWRITE(obj.hash);
        READWRITE(obj.pos);
    }
};

inline uint64_t CreateLegacyKeyPrefix(const PresaltedSipHasher& hasher, const COutPoint& outpoint)
{
    return hasher(outpoint.hash.ToUint256(), outpoint.n);
}

} // namespace txospenderindex

#endif // BITCOIN_INDEX_TXOSPENDERINDEX_KEY_H
