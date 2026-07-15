// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SIPHASH_H
#define BITCOIN_CRYPTO_SIPHASH_H

#include <attributes.h>
#include <uint256.h>

#include <bit>
#include <cstdint>
#include <span>

/** Shared SipHash state (v0..v3) with its round, compression, and finalization primitives.
 *  Internal building block, only meant to be composed by the hasher classes below. */
class SipHashState
{
    /** SipHash initialization constants. */
    static constexpr uint64_t C0{0x736f6d6570736575}, C1{0x646f72616e646f6d}, C2{0x6c7967656e657261}, C3{0x7465646279746573};
    /** SipHash v2 finalizer constant. */
    static constexpr uint64_t FINALIZER{0xFF};
    /** SipHash custom unpadded finalizer constant. */
    static constexpr uint64_t FINALIZER_UNPADDED{0x6465646461706e75};
    static_assert(FINALIZER_UNPADDED != FINALIZER);

    /** Construct a SipHashState with the specified values as state. */
    ALWAYS_INLINE SipHashState(uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3) noexcept : m_v0{v0}, m_v1{v1}, m_v2{v2}, m_v3{v3} {}

    /** State variables. */
    uint64_t m_v0, m_v1, m_v2, m_v3;

    /** Mutably perform one SipRound on this state. */
    ALWAYS_INLINE void SipRound() noexcept
    {
        m_v0 += m_v1; m_v1 = std::rotl(m_v1, 13); m_v1 ^= m_v0;
        m_v0 = std::rotl(m_v0, 32);
        m_v2 += m_v3; m_v3 = std::rotl(m_v3, 16); m_v3 ^= m_v2;
        m_v0 += m_v3; m_v3 = std::rotl(m_v3, 21); m_v3 ^= m_v0;
        m_v2 += m_v1; m_v1 = std::rotl(m_v1, 17); m_v1 ^= m_v2;
        m_v2 = std::rotl(m_v2, 32);
    }

public:
    /** Construct a SipHashState initialized with the specified key. */
    explicit ALWAYS_INLINE SipHashState(uint64_t k0, uint64_t k1) noexcept : SipHashState{C0 ^ k0, C1 ^ k1, C2 ^ k0, C3 ^ k1} {}
    /** Construct a copy of this state. */
    ALWAYS_INLINE SipHashState Copy() const noexcept { return {m_v0, m_v1, m_v2, m_v3}; }
    /** Mutably compress one block into this state, with 1 SipRound. */
    ALWAYS_INLINE SipHashState& Compress1(uint64_t data) noexcept
    {
        m_v3 ^= data;
        SipRound();
        m_v0 ^= data;
        return *this;
    }
    /** Mutably compress one jumbo block into this state, with 1 SipRound. */
    ALWAYS_INLINE SipHashState& Compress1Jumbo(const uint256& data) noexcept
    {
        const uint64_t d0{data.GetUint64(0)}, d1{data.GetUint64(1)}, d2{data.GetUint64(2)}, d3{data.GetUint64(3)};
        m_v3 ^= d0; m_v0 ^= d1; m_v1 ^= d2; m_v2 ^= d3;
        SipRound();
        m_v0 ^= d0; m_v1 ^= d1; m_v2 ^= d2; m_v3 ^= d3;
        return *this;
    }
    /** Mutably compress one block into this state, with 2 SipRounds. */
    ALWAYS_INLINE SipHashState& Compress2(uint64_t data) noexcept
    {
        m_v3 ^= data;
        SipRound();
        SipRound();
        m_v0 ^= data;
        return *this;
    }
    /** Mutably finalize this state with 4 SipRounds, and return the resulting hash. */
    ALWAYS_INLINE uint64_t Finalize4() noexcept
    {
        m_v2 ^= FINALIZER;
        SipRound();
        SipRound();
        SipRound();
        SipRound();
        return m_v0 ^ m_v1 ^ m_v2 ^ m_v3;
    }
    /** Mutably finalize this state with 3 SipRounds using the unpadded finalizer, and return the
     *  resulting hash. */
    ALWAYS_INLINE uint64_t Finalize3U() noexcept
    {
        m_v2 ^= FINALIZER_UNPADDED;
        SipRound();
        SipRound();
        SipRound();
        return m_v0 ^ m_v1 ^ m_v2 ^ m_v3;
    }
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
 *                 arbitrarily mixed. For hash-table use, cryptographic hash outputs must make up
 *                 all but a small bounded number of retained jumbo blocks. Note that the block
 *                 structure matters: hashing one jumbo block is not the same as hashing the same
 *                 data split up into four normal blocks. Jumbo blocks are interpreted as 4 LE64 integers
 *                 (d0,d1,d2,d3), and XOR'ed into the state before a round as
 *                 (v0,v1,v2,v3) ^= (d1,d2,d3,d0), and XOR'ed into the state after a round as
 *                 (v0,v1,v2,v3) ^= (d0,d1,d2,d3). This is a strict generalization of normal block
 *                 processing, in the sense that if d1..d3=0, it is identical to processing a normal
 *                 block d0 (a single LE64 integer). Of course, making d1..d3=0 should be impossible
 *                 in the output of a cryptographic hash function. These jumbo blocks are acceptable
 *                 because even though they may give the attacker more control within a single
 *                 round, that control is limited by the cryptographic hash in between.
 *
 * Other components are unchanged: initialization constants, SipRound, and 64-bit output.
 * This interface serves as an executable specification for fixed-width implementations.
 */
class SipHasher13UJ
{
    SipHashState m_state;

public:
    /** Construct a SipHash-1-3-UJ calculator initialized with 128-bit key (k0, k1). */
    SipHasher13UJ(uint64_t k0, uint64_t k1) noexcept : m_state{k0, k1} {}
    /** Hash a normal 64-bit value. */
    SipHasher13UJ& Write(uint64_t data) noexcept;
    /** Hash a 256-bit value as a jumbo block. For hash-table use, non-hash inputs must remain few and bounded. */
    SipHasher13UJ& WriteJumbo(const uint256& hash) noexcept;
    /** Compute the 64-bit SipHash-1-3-UJ of the data written so far. The object remains untouched. */
    uint64_t Finalize() const noexcept;
};

/**
 * Optimized SipHash-2-4 implementation for uint256.
 *
 * This class caches the initial SipHash state (v0..v3) derived from (k0, k1)
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
