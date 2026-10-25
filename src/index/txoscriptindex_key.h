// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSCRIPTINDEX_KEY_H
#define BITCOIN_INDEX_TXOSCRIPTINDEX_KEY_H

#include <consensus/consensus.h>
#include <crypto/siphash.h>
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
 *   ['B']                             -> best block locator (managed by BaseIndex::DB)
 *   ['b', height, gen]                -> block hash (gen is VARINT-encoded; 0 for the first block at height)
 *   ['s', hash prefix, height[, gen]] -> transaction offsets (gen omitted when 0, VARINT-encoded otherwise)
 *   ["spk_hash_salt"]                 -> hasher salt
 *
 * hash prefix is SipHash-1-3-UJ of SHA256(scriptPubKey), truncated to 52 bits.
 * height is a 28-bit big-endian integer packed with the low 4 bits of hash prefix
 * into 10 bytes total so range seeks remain height-ordered.
 * One row is stored per distinct prefix in a block. Transaction offsets are
 * stored as VARINT-encoded deltas from zero, filling the LevelDB value.
 */

constexpr uint8_t DB_BLOCK{'b'};
constexpr uint8_t DB_TXOSCRIPTINDEX{'s'};
inline const std::string DB_SPK_HASH_SALT{"spk_hash_salt"};

constexpr uint32_t BLOCK_HEADER_SIZE{80};

constexpr uint32_t HASH_PREFIX_BITS{52};
constexpr uint32_t HEIGHT_BITS{28};
using HashPrefix = uint64_t;
constexpr HashPrefix MAX_HASH_PREFIX{(HashPrefix{1} << HASH_PREFIX_BITS) - 1};
constexpr uint32_t MAX_HEIGHT{(uint32_t{1} << HEIGHT_BITS) - 1};

inline HashPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const uint256& script_hash)
{
    return hasher.Hash(script_hash) >> (8 * sizeof(HashPrefix) - HASH_PREFIX_BITS);
}

struct BlockKey {
    uint32_t height{0};
    uint32_t gen{0};

    SERIALIZE_METHODS(BlockKey, obj)
    {
        uint8_t prefix{DB_BLOCK};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_BLOCK) {
            throw std::ios_base::failure("Invalid format for txoscriptindex block key");
        }
        READWRITE(Using<BigEndianFormatter<4>>(obj.height), VARINT(obj.gen));
    }
};

struct DBKey {
    HashPrefix hash_prefix{0};
    uint32_t height{0};
    uint32_t gen{0};

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (hash_prefix > MAX_HASH_PREFIX || height > MAX_HEIGHT) {
            throw std::ios_base::failure("Invalid format for txoscriptindex DB key");
        }
        s << DB_TXOSCRIPTINDEX
          << Using<BigEndianFormatter<6>>(hash_prefix >> 4)
          << Using<BigEndianFormatter<4>>(static_cast<uint32_t>((hash_prefix & 0xf) << HEIGHT_BITS) | height);
        if (gen != 0) s << VARINT(gen);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        uint8_t prefix;
        s >> prefix;
        if (prefix != DB_TXOSCRIPTINDEX) {
            throw std::ios_base::failure("Invalid format for txoscriptindex DB key");
        }
        uint64_t prefix_hi{0};
        uint32_t low{0};
        s >> Using<BigEndianFormatter<6>>(prefix_hi)
          >> Using<BigEndianFormatter<4>>(low);
        hash_prefix = (prefix_hi << 4) | (low >> HEIGHT_BITS);
        height = low & MAX_HEIGHT;
        if (!s.empty()) {
            s >> VARINT(gen);
            if (gen == 0 || !s.empty()) {
                throw std::ios_base::failure("Invalid format for txoscriptindex DB key");
            }
        } else {
            gen = 0;
        }
    }
};

struct DBValue {
    std::vector<uint32_t> offsets;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (offsets.empty() || offsets.size() > MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT) throw std::ios_base::failure("Invalid txoscriptindex offset count");
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
