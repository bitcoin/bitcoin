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

#include <cstdint>
#include <ios>
#include <string>
#include <vector>

// Internal format shared with encoding and collision-injection tests.
namespace txoscriptindex {
/*
 * Database layout:
 *
 *   ['s', hash prefix, height] -> transaction offsets
 *   ["spk_hash_salt"]          -> hasher salt
 *   ['B']                     -> sync locator
 *
 * hash prefix is SipHash-1-3-UJ of SHA256(scriptPubKey), truncated to 6 bytes.
 * height is an OrderedVarInt so prefix+height range seeks are height-ordered.
 * One row is stored per distinct prefix in a block. Transaction offsets are
 * stored as VARINT-encoded deltas from zero, filling the LevelDB value.
 */

constexpr uint8_t DB_TXOSCRIPTINDEX{'s'};
inline const std::string DB_SPK_HASH_SALT{"spk_hash_salt"};

constexpr uint32_t BLOCK_HEADER_SIZE{80};

constexpr uint32_t HASH_PREFIX_SIZE{6};
using HashPrefix = uint64_t;

inline HashPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const uint256& script_hash)
{
    return hasher.Hash(script_hash) >> (8 * (sizeof(HashPrefix) - HASH_PREFIX_SIZE));
}

struct DBKey {
    HashPrefix hash_prefix{0};
    uint32_t height{0};

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_TXOSCRIPTINDEX};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_TXOSCRIPTINDEX) {
            throw std::ios_base::failure("Invalid format for txoscriptindex DB key");
        }
        READWRITE(Using<BigEndianFormatter<HASH_PREFIX_SIZE>>(obj.hash_prefix),
                  Using<OrderedVarIntFormatter>(obj.height));
    }
};

struct DBValue {
    std::vector<uint32_t> offsets;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (offsets.empty()) throw std::ios_base::failure("Empty txoscriptindex value");
        uint32_t previous{0};
        for (const uint32_t offset : offsets) {
            if (offset <= previous || offset >= MAX_BLOCK_SERIALIZED_SIZE) {
                throw std::ios_base::failure("Invalid txoscriptindex transaction offset");
            }
            s << VARINT(offset - previous);
            previous = offset;
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        offsets.clear();
        uint32_t offset{0};
        do {
            uint32_t delta;
            s >> VARINT(delta);
            if (delta == 0 || delta >= MAX_BLOCK_SERIALIZED_SIZE - offset || offsets.size() >= MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT) {
                throw std::ios_base::failure("Invalid txoscriptindex transaction offset");
            }
            offsets.push_back(offset += delta);
        } while (!s.empty());
    }
};

} // namespace txoscriptindex

#endif // BITCOIN_INDEX_TXOSCRIPTINDEX_KEY_H
