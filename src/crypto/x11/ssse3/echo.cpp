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
} // anonymous namespace

namespace ssse3_echo {
void MixColumns(uint64_t W[16][2])
{
    MixColumn(W, 0, 1, 2, 3);
    MixColumn(W, 4, 5, 6, 7);
    MixColumn(W, 8, 9, 10, 11);
    MixColumn(W, 12, 13, 14, 15);
}
} // namespace ssse3_echo
} // namespace sapphire

#endif // ENABLE_SSSE3
