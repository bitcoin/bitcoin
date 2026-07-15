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
inline constexpr uint64_t SIPHASH_FINALIZER_UNPADDED{0x6465646461706E75};
static_assert(SIPHASH_FINALIZER_UNPADDED != SIPHASH_FINALIZER);

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

ALWAYS_INLINE void ProcessNormal(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3, uint64_t data)
{
    v3 ^= data;
    SipRound(v0, v1, v2, v3);
    v0 ^= data;
}

ALWAYS_INLINE void ProcessJumbo(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3, const uint256& data)
{
    const uint64_t d0{data.GetUint64(0)}, d1{data.GetUint64(1)}, d2{data.GetUint64(2)}, d3{data.GetUint64(3)};
    v3 ^= d0; v0 ^= d1; v1 ^= d2; v2 ^= d3;
    SipRound(v0, v1, v2, v3);
    v0 ^= d0; v1 ^= d1; v2 ^= d2; v3 ^= d3;
}

ALWAYS_INLINE uint64_t Finalize13UJ(uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3)
{
    v2 ^= SIPHASH_FINALIZER_UNPADDED;
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

/** A custom weaker variant of SipHash-1-3 without padding, and supporting "jumbo" inputs.
 *
 * Compared to the traditional SipHash-2-4, this incorporates 3 changes:
 *
 * - Use SipHash-1-3: This common variant reduces the number of rounds between input blocks from 2
 *                    to 1, and the number of finalization rounds at the end from 4 to 3.
 *                    It is a standard faster choice with weaker security guarantees. It is
 *                    considered strong enough for use in hash tables and other applications with
 *                    only weak security requirements, in particular when attackers cannot directly
 *                    observe the hash function output, and cannot control k0 and k1.
 *
 * - Remove padding: Standard SipHash operates on byte-oriented inputs, which are converted into
 *                   8-byte blocks internally by adding a padding of 1-8 bytes to obtain a multiple
 *                   of 8 bytes. In this variant, the input is a sequence of blocks instead, which
 *                   are used without padding. This saves 1 round for inputs that are a multiple of
 *                   8 bytes already. The padding normally adds a commitment to the input's byte
 *                   length, but since our input is block oriented, and there is one round per
 *                   input, the function of that commitment is directly fulfilled by the number of
 *                   rounds. The empty input is permitted; it yields the result of finalizing the
 *                   initialization directly. Out of an abundance of caution the finalizing
 *                   v2 ^= 0xff is changed to v2 ^= 0x6465646461706e75 ("unpadded" in LE64), to
 *                   avoid collisions between standard SipHash-1-3 and this variant.
 *
 * - Jumbo blocks: We generalize the input to consist of a sequence of blocks which are each either
 *                 exactly 8 bytes (normal blocks) or 32 bytes (jumbo blocks), and may be
 *                 arbitrarily mixed. Jumbo blocks are only allowed if they are themselves the
 *                 output of a cryptographic hash function. Note that the block structure matters:
 *                 hashing one jumbo block is not the same as hashing the same data split up into
 *                 four normal blocks. Jumbo blocks are interpreted as 4 LE64 integers
 *                 (d0,d1,d2,d3), and XOR'ed into the state before a round as
 *                 (v0,v1,v2,v3) ^= (d1,d2,d3,d0), and XOR'ed into the state after a round as
 *                 (v0,v1,v2,v3) ^= (d0,d1,d2,d3). This is a strict generalization of normal block
 *                 processing, in the sense that if d1..d3=0, it is identical to processing a normal
 *                 block d0 (a single LE64 integer). Of course, making d1..d3=0 should be impossible
 *                 in the output of a cryptographic hash function. These jumbo blocks are acceptable
 *                 because even though they may give the attacker more control within a single
 *                 round, that control is limited by the cryptographic hash in between.
 *
 * Other components are unchanged: initialization, constants, SipRound, and 64-bit output.
 * This interface serves as an executable specification for fixed-shape implementations.
 */
class SipHasher13UJ
{
    SipHashState m_state;

public:
    /** Construct a SipHash-1-3-UJ calculator initialized with 128-bit key (k0, k1). */
    SipHasher13UJ(uint64_t k0, uint64_t k1) noexcept : m_state{k0, k1} {}
    /** Hash a normal 64-bit value. */
    SipHasher13UJ& Write(uint64_t data) noexcept;
    /** Hash a 256-bit value which *must* be the output of a cryptographic hash. */
    SipHasher13UJ& WriteJumbo(const uint256& hash) noexcept;
    /** Compute the 64-bit SipHash-1-3-UJ of the data written so far. The object remains untouched. */
    uint64_t Finalize() const noexcept;
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
