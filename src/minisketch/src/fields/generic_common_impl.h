/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _MINISKETCH_FIELDS_GENERIC_COMMON_IMPL_H_
#define _MINISKETCH_FIELDS_GENERIC_COMMON_IMPL_H_ 1

#include <stdint.h>

#include "../int_utils.h"
#include "../lintrans.h"

namespace {

/** Generic implementation for fields whose elements can be represented by an integer type. */
template<typename I, int B, uint32_t MOD, typename F, typename T, const F* SQR, const F* QRT> class Field
{
    typedef BitsInt<I, B> O;
    typedef LFSR<O, MOD> L;

public:
    typedef I Elem;
    constexpr int Bits() const { return B; }

    constexpr inline Elem Mul2(Elem val) const { return L::Call(val); }

    class Multiplier
    {
        T table;
    public:
        explicit Multiplier(const Field&, Elem a) { table.template Build<L::Call>(a); }
        constexpr inline Elem operator()(Elem a) const { return table.template Map<O>(a); }
    };

    Elem Mul(Elem a, Elem b) const { return GFMul<I, B, L, O>(a, b); }

    /** Compute the square of a. */
    inline constexpr Elem Sqr(Elem a) const { return SQR->template Map<O>(a); }

    /** Compute x such that x^2 + x = a (undefined result if no solution exists). */
    inline constexpr Elem Qrt(Elem a) const { return QRT->template Map<O>(a); }

    /** Compute the inverse of x1. */
    Elem Inv(Elem a) const { return InvExtGCD<I, O, B, MOD>(a); }

    /** Generate a random field element. */
    Elem FromSeed(uint64_t seed) const {
        uint64_t k0 = 0x496e744669656c64ull; // "IntField"
        uint64_t k1 = seed;
        uint64_t count = ((uint64_t)B) << 32;
        Elem ret;
        do {
            ret = O::Mask(I(SipHash(k0, k1, count++)));
        } while(ret == 0);
        return ret;
    }

    Elem Deserialize(BitReader& in) const { return in.template Read<B, I>(); }

    void Serialize(BitWriter& out, Elem val) const { out.template Write<B, I>(val); }

    constexpr Elem FromUint64(uint64_t x) const { return O::Mask(I(x)); }
    constexpr uint64_t ToUint64(Elem val) const { return uint64_t(val); }
};

}

#endif
