// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_MUHASH_H
#define BITCOIN_CRYPTO_MUHASH_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <serialize.h>
#include <stdint.h>

struct Num3072 {
#ifdef HAVE___INT128
    typedef unsigned __int128 double_limb_type;
    typedef uint64_t limb_type;
    static constexpr int LIMBS = 48;
    static constexpr int LIMB_SIZE = 64;
#else
    typedef uint64_t double_limb_type;
    typedef uint32_t limb_type;
    static constexpr int LIMBS = 96;
    static constexpr int LIMB_SIZE = 32;
#endif
    limb_type limbs[LIMBS];
};

/** A class representing MuHash sets
 *
 * MuHash is a hashing algorithm that supports adding set elements in any
 * order but also deleting in any order. As a result, it can maintain a
 * running sum for a set of data as a whole, and add/subtract when data
 * is added to or removed from it. A downside of MuHash is that computing
 * an inverse is relatively expensive. This can be solved by representing
 * the running value as a fraction, and multiplying added elements into
 * the numerator and removed elements into the denominator. Only when the
 * final hash is desired, a single modular inverse and multiplication is
 * needed to combine the two.
 *
 * As the update operations are also associative, H(a)+H(b)+H(c)+H(d) can
 * in fact be computed as (H(a)+H(b)) + (H(c)+H(d)). This implies that
 * all of this is perfectly parallellizable: each thread can process an
 * arbitrary subset of the update operations, allowing them to be
 * efficiently combined later.
 */
class MuHash3072
{
private:
    Num3072 data;

public:
    /* The empty set. */
    MuHash3072() noexcept;

    /* A singleton with a single 32-byte key in it. */
    explicit MuHash3072(const unsigned char* key32) noexcept;

    /* Multiply (resulting in a hash for the union of the sets) */
    MuHash3072& operator*=(const MuHash3072& add) noexcept;

    /* Divide (resulting in a hash for the difference of the sets) */
    MuHash3072& operator/=(const MuHash3072& sub) noexcept;

    /* Finalize into a 384-byte hash. Does not change this object's value. */
    void Finalize(unsigned char* hash384) noexcept;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        for(int i = 0; i<data.LIMBS; i++) {
            READWRITE(data.limbs[i]);
        }
    }
};

#endif // BITCOIN_CRYPTO_MUHASH_H
