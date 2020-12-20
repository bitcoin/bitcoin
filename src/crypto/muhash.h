// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_MUHASH_H
#define BITCOIN_CRYPTO_MUHASH_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <serialize.h>
#include <uint256.h>

#include <stdint.h>

class Num3072
{
private:
    void FullReduce();
    bool IsOverflow() const;
    Num3072 GetInverse() const;

public:

#ifdef HAVE___INT128
    typedef unsigned __int128 double_limb_t;
    typedef uint64_t limb_t;
    static constexpr int LIMBS = 48;
    static constexpr int LIMB_SIZE = 64;
#else
    typedef uint64_t double_limb_t;
    typedef uint32_t limb_t;
    static constexpr int LIMBS = 96;
    static constexpr int LIMB_SIZE = 32;
#endif
    limb_t limbs[LIMBS];

    // Sanity check for Num3072 constants
    static_assert(LIMB_SIZE * LIMBS == 3072, "Num3072 isn't 3072 bits");
    static_assert(sizeof(double_limb_t) == sizeof(limb_t) * 2, "bad size for double_limb_t");
    static_assert(sizeof(limb_t) * 8 == LIMB_SIZE, "LIMB_SIZE is incorrect");

    // Hard coded values in MuHash3072 constructor and Finalize
    static_assert(sizeof(limb_t) == 4 || sizeof(limb_t) == 8, "bad size for limb_t");

    void Multiply(const Num3072& a);
    void Divide(const Num3072& a);
    void SetToOne();
    void Square();

    Num3072() { this->SetToOne(); };

    SERIALIZE_METHODS(Num3072, obj)
    {
        for (auto& limb : obj.limbs) {
            READWRITE(limb);
        }
    }
};

#endif // BITCOIN_CRYPTO_MUHASH_H
