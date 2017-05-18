// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_MUHASH_H
#define BITCOIN_CRYPTO_MUHASH_H

#if defined(HAVE_CONFIG_H)
#include "bitcoin-config.h"
#endif

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

/** A class representing MuHash sets */
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
};

#endif // BITCOIN_CRYPTO_MUHASH_H
