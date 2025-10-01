// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SIPHASH_H
#define BITCOIN_CRYPTO_SIPHASH_H

#include <cstdint>
#include <span>

class uint256;

/** SipHash-2-4 */
class CSipHasher
{
private:
    uint64_t v[4];
    uint64_t tmp;
    uint8_t count; // Only the low 8 bits of the input size matter.

public:
    static constexpr uint64_t C0{0x736f6d6570736575ULL};
    static constexpr uint64_t C1{0x646f72616e646f6dULL};
    static constexpr uint64_t C2{0x6c7967656e657261ULL};
    static constexpr uint64_t C3{0x7465646279746573ULL};

    /** Construct a SipHash calculator initialized with 128-bit key (k0, k1) */
    CSipHasher(uint64_t k0, uint64_t k1);
    /** Hash a 64-bit integer worth of data
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
    uint64_t v[4];

public:
    explicit PresaltedSipHasher(uint64_t k0, uint64_t k1) noexcept {
        v[0] = CSipHasher::C0 ^ k0;
        v[1] = CSipHasher::C1 ^ k1;
        v[2] = CSipHasher::C2 ^ k0;
        v[3] = CSipHasher::C3 ^ k1;
    }

    uint64_t operator()(const uint256& val) const noexcept;
    uint64_t operator()(const uint256& val, uint32_t extra) const noexcept;
};

#endif // BITCOIN_CRYPTO_SIPHASH_H
