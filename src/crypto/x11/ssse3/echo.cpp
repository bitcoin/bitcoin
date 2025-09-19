// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_SSSE3)
#include <attributes.h>
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <tmmintrin.h>

namespace sapphire {
namespace {
__m128i ALWAYS_INLINE gf8_mul2(const __m128i& x)
{
    // (x << 1)
    const __m128i lhs = _mm_add_epi8(x, x);
    // (x & 0x80) ? 0xff : 0x00
    const __m128i msb_set = _mm_and_si128(x, _mm_set1_epi8(0x80));
    const __m128i mask = _mm_cmpeq_epi8(msb_set, _mm_set1_epi8(0x80));
    // Replace 0xff with 0x1b
    const __m128i rhs = _mm_and_si128(mask, _mm_set1_epi8(0x1b));
    // (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00)
    return util::Xor(lhs, rhs);
}

void ALWAYS_INLINE MixColumn(uint64_t W[16][2], int ia, int ib, int ic, int id)
{
    const __m128i a = _mm_load_si128((const __m128i*)&W[ia][0]);
    const __m128i b = _mm_load_si128((const __m128i*)&W[ib][0]);
    const __m128i c = _mm_load_si128((const __m128i*)&W[ic][0]);
    const __m128i d = _mm_load_si128((const __m128i*)&W[id][0]);

    const __m128i ab = util::Xor(a, b);
    const __m128i bc = util::Xor(b, c);
    const __m128i cd = util::Xor(c, d);

    const __m128i abx = gf8_mul2(ab);
    const __m128i bcx = gf8_mul2(bc);
    const __m128i cdx = gf8_mul2(cd);

    // W[ia] = abx ^ bc ^ d
    _mm_store_si128((__m128i*)&W[ia][0], util::Xor(util::Xor(abx, bc), d));
    // W[ib] = bcx ^ a ^ cd
    _mm_store_si128((__m128i*)&W[ib][0], util::Xor(util::Xor(bcx, a), cd));
    // W[ic] = cdx ^ ab ^ d
    _mm_store_si128((__m128i*)&W[ic][0], util::Xor(util::Xor(cdx, ab), d));
    // W[id] = abx ^ bcx ^ cdx ^ ab ^ c
    _mm_store_si128((__m128i*)&W[id][0], util::Xor(util::Xor(util::Xor(util::Xor(abx, bcx), cdx), ab), c));
}

void ALWAYS_INLINE ShiftRow1(__m128i& Wa, __m128i& Wb, __m128i& Wc, __m128i& Wd)
{
    __m128i tmp = Wa;
    Wa = Wb;
    Wb = Wc;
    Wc = Wd;
    Wd = tmp;
}

void ALWAYS_INLINE ShiftRow2(__m128i& Wa, __m128i& Wb, __m128i& Wc, __m128i& Wd)
{
    __m128i tmp1 = Wa;
    __m128i tmp2 = Wb;
    Wa = Wc;
    Wb = Wd;
    Wc = tmp1;
    Wd = tmp2;
}

void ALWAYS_INLINE ShiftRow3(__m128i& Wa, __m128i& Wb, __m128i& Wc, __m128i& Wd)
{
    __m128i tmp = Wd;
    Wd = Wc;
    Wc = Wb;
    Wb = Wa;
    Wa = tmp;
}
} // anonymous namespace

namespace ssse3_echo {
void MixColumns(uint64_t W[16][2])
{
    MixColumn(W, 0, 1, 2, 3);
    MixColumn(W, 4, 5, 6, 7);
    MixColumn(W, 8, 9, 10, 11);
    MixColumn(W, 12, 13, 14, 15);
}

void ShiftRows(uint64_t W[16][2])
{
    alignas(16) __m128i w[16];
    w[0] = _mm_load_si128((const __m128i*)&W[0][0]);
    w[1] = _mm_load_si128((const __m128i*)&W[1][0]);
    w[2] = _mm_load_si128((const __m128i*)&W[2][0]);
    w[3] = _mm_load_si128((const __m128i*)&W[3][0]);
    w[4] = _mm_load_si128((const __m128i*)&W[4][0]);
    w[5] = _mm_load_si128((const __m128i*)&W[5][0]);
    w[6] = _mm_load_si128((const __m128i*)&W[6][0]);
    w[7] = _mm_load_si128((const __m128i*)&W[7][0]);
    w[8] = _mm_load_si128((const __m128i*)&W[8][0]);
    w[9] = _mm_load_si128((const __m128i*)&W[9][0]);
    w[10] = _mm_load_si128((const __m128i*)&W[10][0]);
    w[11] = _mm_load_si128((const __m128i*)&W[11][0]);
    w[12] = _mm_load_si128((const __m128i*)&W[12][0]);
    w[13] = _mm_load_si128((const __m128i*)&W[13][0]);
    w[14] = _mm_load_si128((const __m128i*)&W[14][0]);
    w[15] = _mm_load_si128((const __m128i*)&W[15][0]);

    ShiftRow1(w[1], w[5], w[9], w[13]);
    ShiftRow2(w[2], w[6], w[10], w[14]);
    ShiftRow3(w[3], w[7], w[11], w[15]);

    _mm_store_si128((__m128i*)&W[0][0], w[0]);
    _mm_store_si128((__m128i*)&W[1][0], w[1]);
    _mm_store_si128((__m128i*)&W[2][0], w[2]);
    _mm_store_si128((__m128i*)&W[3][0], w[3]);
    _mm_store_si128((__m128i*)&W[4][0], w[4]);
    _mm_store_si128((__m128i*)&W[5][0], w[5]);
    _mm_store_si128((__m128i*)&W[6][0], w[6]);
    _mm_store_si128((__m128i*)&W[7][0], w[7]);
    _mm_store_si128((__m128i*)&W[8][0], w[8]);
    _mm_store_si128((__m128i*)&W[9][0], w[9]);
    _mm_store_si128((__m128i*)&W[10][0], w[10]);
    _mm_store_si128((__m128i*)&W[11][0], w[11]);
    _mm_store_si128((__m128i*)&W[12][0], w[12]);
    _mm_store_si128((__m128i*)&W[13][0], w[13]);
    _mm_store_si128((__m128i*)&W[14][0], w[14]);
    _mm_store_si128((__m128i*)&W[15][0], w[15]);
}
} // namespace ssse3_echo
} // namespace sapphire

#endif // ENABLE_SSSE3
