// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI)
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <immintrin.h>
#include <wmmintrin.h>

namespace sapphire {
namespace x86_aesni_aes {
void Round(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3,
           uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3)
{
    __m128i block = util::aes_round(util::pack_le(x0, x1, x2, x3), util::pack_le(k0, k1, k2, k3));
    util::unpack_le(block, y0, y1, y2, y3);
}

void RoundKeyless(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3)
{
    __m128i block = util::aes_round(util::pack_le(x0, x1, x2, x3), _mm_setzero_si128());
    util::unpack_le(block, y0, y1, y2, y3);
}
} // namespace x86_aesni_aes
} // namespace sapphire

#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
