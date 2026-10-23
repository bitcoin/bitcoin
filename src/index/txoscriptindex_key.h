// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSCRIPTINDEX_KEY_H
#define BITCOIN_INDEX_TXOSCRIPTINDEX_KEY_H

#include <consensus/consensus.h>
#include <crypto/siphash.h>
#include <index/ordered_varint.h>
#include <serialize.h>
#include <uint256.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <string>

// Internal format shared with encoding and collision-injection tests.
namespace txoscriptindex {
/*
 * Database layout:
 *
 *   ['x', hash prefix, height, tx offset] -> (empty)
 *   ["spk_hash_salt"]                     -> hasher salt
 *   ['B']                                 -> sync locator
 *
 * hash prefix is SipHash-1-3-UJ of SHA256(scriptPubKey), truncated to 5 bytes.
 * height is an OrderedVarInt so prefix+height range seeks are height-ordered.
 * One key is stored per distinct prefix in a transaction (not per output).
 */

constexpr uint8_t DB_TXOSCRIPTINDEX{'x'};
inline const std::string DB_SPK_HASH_SALT{"spk_hash_salt"};

inline constexpr std::array<std::byte, 0> EMPTY_VALUE{};

constexpr uint32_t BLOCK_HEADER_SIZE{80};

constexpr uint32_t HASH_PREFIX_SIZE{5};
using HashPrefix = uint64_t;

inline HashPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const uint256& script_hash)
{
    return hasher.Hash(script_hash) >> (8 * (sizeof(HashPrefix) - HASH_PREFIX_SIZE));
}

struct DBKey {
    HashPrefix hash_prefix{0};
    uint32_t height{0};
    uint32_t tx_offset_in_block{0};

    static constexpr uint32_t TX_OFFSET_SIZE{3};
    static_assert(MAX_BLOCK_SERIALIZED_SIZE <= BigEndianFormatter<TX_OFFSET_SIZE>::MAX);

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_TXOSCRIPTINDEX};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_TXOSCRIPTINDEX) {
            throw std::ios_base::failure("Invalid format for txoscriptindex DB key");
        }
        READWRITE(Using<BigEndianFormatter<HASH_PREFIX_SIZE>>(obj.hash_prefix),
                  Using<OrderedVarIntFormatter>(obj.height),
                  Using<BigEndianFormatter<TX_OFFSET_SIZE>>(obj.tx_offset_in_block));
    }
};

} // namespace txoscriptindex

#endif // BITCOIN_INDEX_TXOSCRIPTINDEX_KEY_H
