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
