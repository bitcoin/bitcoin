// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SIPHASH_H
#define BITCOIN_CRYPTO_SIPHASH_H

#include <attributes.h>
#include <uint256.h>

#include <array>
#include <bit>
#include <cstdint>
#include <span>

namespace siphash_detail {

inline constexpr uint64_t SIPHASH_FINALIZER{0xFF};

ALWAYS_INLINE void SipRound(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3)
{
    v0 += v1; v1 = std::rotl(v1, 13); v1 ^= v0;
    v0 = std::rotl(v0, 32);
    v2 += v3; v3 = std::rotl(v3, 16); v3 ^= v2;
    v0 += v3; v3 = std::rotl(v3, 21); v3 ^= v0;
    v2 += v1; v1 = std::rotl(v1, 17); v1 ^= v2;
    v2 = std::rotl(v2, 32);
}

ALWAYS_INLINE void ProcessNormal24(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3, uint64_t data)
{
    v3 ^= data;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= data;
}

ALWAYS_INLINE uint64_t Finalize24(uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3, uint64_t data)
{
    ProcessNormal24(v0, v1, v2, v3, data);
    v2 ^= SIPHASH_FINALIZER;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

} // namespace siphash_detail

/** Shared SipHash internal state v[0..3], initialized from (k0, k1). */
class SipHashState
{
    static constexpr uint64_t C0{0x736f6d6570736575ULL}, C1{0x646f72616e646f6dULL}, C2{0x6c7967656e657261ULL}, C3{0x7465646279746573ULL};

public:
    explicit SipHashState(uint64_t k0, uint64_t k1) noexcept : v{C0 ^ k0, C1 ^ k1, C2 ^ k0, C3 ^ k1} {}

    std::array<uint64_t, 4> v{};
};

/** General SipHash-2-4 implementation. */
class CSipHasher
{
    SipHashState m_state;
    uint64_t m_tmp{0};
    uint8_t m_count{0}; //!< Only the low 8 bits of the input size matter.

public:
    /** Construct a SipHash calculator initialized with 128-bit key (k0, k1). */
    CSipHasher(uint64_t k0, uint64_t k1);
    /** Hash a 64-bit integer worth of data.
     *  It is treated as if this was the little-endian interpretation of 8 bytes.
     *  This function can only be used when a multiple of 8 bytes have been written so far.
     */
    CSipHasher& Write(uint64_t data);
    /** Hash arbitrary bytes. */
    CSipHasher& Write(std::span<const unsigned char> data);
    /** Compute the 64-bit SipHash-2-4 of the data written so far. The object remains untouched. */
    uint64_t Finalize() const;
};

/**
 * Optimized SipHash-2-4 implementation for uint256.
 *
 * This class caches the initial SipHash v[0..3] state derived from (k0, k1)
 * and implements a specialized hashing path for uint256 values, with or
 * without an extra 32-bit word. The internal state is immutable, so
 * PresaltedSipHasher instances can be reused for multiple hashes with the
 * same key.
 */
class PresaltedSipHasher
{
    const SipHashState m_state;

public:
    explicit PresaltedSipHasher(uint64_t k0, uint64_t k1) noexcept : m_state{k0, k1} {}

    /** Equivalent to CSipHasher(k0, k1).Write(val).Finalize(). */
    uint64_t operator()(const uint256& val) const noexcept;

    /**
     * Equivalent to CSipHasher(k0, k1).Write(val).Write(extra).Finalize(),
     * with `extra` encoded as 4 little-endian bytes.
     */
    uint64_t operator()(const uint256& val, uint32_t extra) const noexcept;
};

#endif // BITCOIN_CRYPTO_SIPHASH_H
