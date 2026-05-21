/**********************************************************************
 * Copyright (c) 2018 Pieter Wuille, Greg Maxwell, Gleb Naumenko      *
 * Distributed under the MIT software license, see the accompanying   *
 * file LICENSE or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _MINISKETCH_FIELDS_CLMUL_COMMON_IMPL_H_
#define _MINISKETCH_FIELDS_CLMUL_COMMON_IMPL_H_ 1

#include <stdint.h>
#include <immintrin.h>

#include "../int_utils.h"
#include "../lintrans.h"

namespace {

// The memory sanitizer in clang < 11 cannot reason through _mm_clmulepi64_si128 calls.
// Disable memory sanitization in the functions using them for those compilers.
#if defined(__clang__) && (__clang_major__ < 11)
#  if defined(__has_feature)
#    if __has_feature(memory_sanitizer)
#      define NO_SANITIZE_MEMORY __attribute__((no_sanitize("memory")))
#    endif
#  endif
#endif
#ifndef NO_SANITIZE_MEMORY
#  define NO_SANITIZE_MEMORY
#endif

template<typename I, int BITS, I MOD> NO_SANITIZE_MEMORY I MulWithClMulReduce(I a, I b)
{
    static constexpr I MASK = Mask<BITS, I>();

    const __m128i MOD128 = _mm_cvtsi64_si128(MOD);
    __m128i product = _mm_clmulepi64_si128(_mm_cvtsi64_si128((uint64_t)a), _mm_cvtsi64_si128((uint64_t)b), 0x00);
    if (BITS <= 32) {
        __m128i high1 = _mm_srli_epi64(product, BITS);
        __m128i red1 = _mm_clmulepi64_si128(high1, MOD128, 0x00);
        __m128i high2 = _mm_srli_epi64(red1, BITS);
        __m128i red2 = _mm_clmulepi64_si128(high2, MOD128, 0x00);
        return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
    } else if (BITS == 64) {
        __m128i red1 = _mm_clmulepi64_si128(product, MOD128, 0x01);
        __m128i red2 = _mm_clmulepi64_si128(red1, MOD128, 0x01);
        return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2));
    } else if ((BITS % 8) == 0) {
        __m128i high1 = _mm_srli_si128(product, BITS / 8);
        __m128i red1 = _mm_clmulepi64_si128(high1, MOD128, 0x00);
        __m128i high2 = _mm_srli_si128(red1, BITS / 8);
        __m128i red2 = _mm_clmulepi64_si128(high2, MOD128, 0x00);
        return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
    } else {
        __m128i high1 = _mm_or_si128(_mm_srli_epi64(product, BITS), _mm_srli_si128(_mm_slli_epi64(product, 64 - BITS), 8));
        __m128i red1 = _mm_clmulepi64_si128(high1, MOD128, 0x00);
        if ((uint64_t(MOD) >> (66 - BITS)) == 0) {
            __m128i high2 = _mm_srli_epi64(red1, BITS);
            __m128i red2 = _mm_clmulepi64_si128(high2, MOD128, 0x00);
            return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
        } else {
            __m128i high2 = _mm_or_si128(_mm_srli_epi64(red1, BITS), _mm_srli_si128(_mm_slli_epi64(red1, 64 - BITS), 8));
            __m128i red2 = _mm_clmulepi64_si128(high2, MOD128, 0x00);
            return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
        }
    }
}

template<typename I, int BITS, int POS> NO_SANITIZE_MEMORY I MulTrinomial(I a, I b)
{
    static constexpr I MASK = Mask<BITS, I>();

    __m128i product = _mm_clmulepi64_si128(_mm_cvtsi64_si128((uint64_t)a), _mm_cvtsi64_si128((uint64_t)b), 0x00);
    if (BITS <= 32) {
        __m128i high1 = _mm_srli_epi64(product, BITS);
        __m128i red1 = _mm_xor_si128(high1, _mm_slli_epi64(high1, POS));
        if (POS == 1) {
            return _mm_cvtsi128_si64(_mm_xor_si128(product, red1)) & MASK;
        } else {
            __m128i high2 = _mm_srli_epi64(red1, BITS);
            __m128i red2 = _mm_xor_si128(high2, _mm_slli_epi64(high2, POS));
            return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
        }
    } else {
        __m128i high1 = _mm_or_si128(_mm_srli_epi64(product, BITS), _mm_srli_si128(_mm_slli_epi64(product, 64 - BITS), 8));
        if (BITS + POS <= 66) {
            __m128i red1 = _mm_xor_si128(high1, _mm_slli_epi64(high1, POS));
            if (POS == 1) {
                return _mm_cvtsi128_si64(_mm_xor_si128(product, red1)) & MASK;
            } else if (BITS + POS <= 66) {
                __m128i high2 = _mm_srli_epi64(red1, BITS);
                __m128i red2 = _mm_xor_si128(high2, _mm_slli_epi64(high2, POS));
                return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
            }
        } else {
            const __m128i MOD128 = _mm_cvtsi64_si128(1 + (((uint64_t)1) << POS));
            __m128i red1 = _mm_clmulepi64_si128(high1, MOD128, 0x00);
            __m128i high2 = _mm_or_si128(_mm_srli_epi64(red1, BITS), _mm_srli_si128(_mm_slli_epi64(red1, 64 - BITS), 8));
            __m128i red2 = _mm_xor_si128(high2, _mm_slli_epi64(high2, POS));
            return _mm_cvtsi128_si64(_mm_xor_si128(_mm_xor_si128(product, red1), red2)) & MASK;
        }
    }
}

/** Implementation of fields that use the SSE clmul intrinsic for multiplication. */
template<typename I, int B, I MOD, I (*MUL)(I, I), typename F, const F* SQR, const F* SQR2, const F* SQR4, const F* SQR8, const F* SQR16, const F* QRT, typename T, const T* LOAD, const T* SAVE> struct GenField
{
    typedef BitsInt<I, B> O;
    typedef LFSR<O, MOD> L;

    static inline constexpr I Sqr1(I a) { return SQR->template Map<O>(a); }
    static inline constexpr I Sqr2(I a) { return SQR2->template Map<O>(a); }
    static inline constexpr I Sqr4(I a) { return SQR4->template Map<O>(a); }
    static inline constexpr I Sqr8(I a) { return SQR8->template Map<O>(a); }
    static inline constexpr I Sqr16(I a) { return SQR16->template Map<O>(a); }

public:
    typedef I Elem;

    inline constexpr int Bits() const { return B; }

    inline constexpr Elem Mul2(Elem val) const { return L::Call(val); }

    inline Elem Mul(Elem a, Elem b) const { return MUL(a, b); }

    class Multiplier
    {
        Elem m_val;
    public:
        inline constexpr explicit Multiplier(const GenField&, Elem a) : m_val(a) {}
        constexpr Elem operator()(Elem a) const { return MUL(m_val, a); }
    };

    /** Compute the square of a. */
    inline constexpr Elem Sqr(Elem val) const { return SQR->template Map<O>(val); }

    /** Compute x such that x^2 + x = a (undefined result if no solution exists). */
    inline constexpr Elem Qrt(Elem val) const { return QRT->template Map<O>(val); }

    /** Compute the inverse of x1. */
    inline Elem Inv(Elem val) const { return InvLadder<I, O, B, MUL, Sqr1, Sqr2, Sqr4, Sqr8, Sqr16>(val); }

    /** Generate a random field element. */
    Elem FromSeed(uint64_t seed) const {
        uint64_t k0 = 0x434c4d554c466c64ull; // "CLMULFld"
        uint64_t k1 = seed;
        uint64_t count = ((uint64_t)B) << 32;
        I ret;
        do {
            ret = O::Mask(I(SipHash(k0, k1, count++)));
        } while(ret == 0);
        return LOAD->template Map<O>(ret);
    }

    Elem Deserialize(BitReader& in) const { return LOAD->template Map<O>(in.Read<B, I>()); }

    void Serialize(BitWriter& out, Elem val) const { out.Write<B, I>(SAVE->template Map<O>(val)); }

    constexpr Elem FromUint64(uint64_t x) const { return LOAD->template Map<O>(O::Mask(I(x))); }
    constexpr uint64_t ToUint64(Elem val) const { return uint64_t(SAVE->template Map<O>(val)); }
};

template<typename I, int B, I MOD, typename F, const F* SQR, const F* SQR2, const F* SQR4, const F* SQR8, const F* SQR16, const F* QRT, typename T, const T* LOAD, const T* SAVE>
using Field = GenField<I, B, MOD, MulWithClMulReduce<I, B, MOD>, F, SQR, SQR2, SQR4, SQR8, SQR16, QRT, T, LOAD, SAVE>;

template<typename I, int B, int POS, typename F, const F* SQR, const F* SQR2, const F* SQR4, const F* SQR8, const F* SQR16, const F* QRT, typename T, const T* LOAD, const T* SAVE>
using FieldTri = GenField<I, B, I(1) + (I(1) << POS), MulTrinomial<I, B, POS>, F, SQR, SQR2, SQR4, SQR8, SQR16, QRT, T, LOAD, SAVE>;

}

#endif
