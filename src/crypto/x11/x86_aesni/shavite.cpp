// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI)
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <immintrin.h>
#include <wmmintrin.h>

namespace sapphire {
namespace x86_aesni_shavite {
void CompressElement(uint32_t& l0, uint32_t& l1, uint32_t& l2, uint32_t& l3,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, const uint32_t* rk)
{
    // Load block + XOR with round key 1
    __m128i block = util::pack_le(r0, r1, r2, r3);
    block = util::Xor(block, _mm_load_si128((const __m128i*)&rk[0]));
    // AES round + XOR with round key 2
    block = util::aes_round(block, _mm_setzero_si128());
    block = util::Xor(block, _mm_load_si128((const __m128i*)&rk[4]));
    // AES round + XOR with round key 3
    block = util::aes_round(block, _mm_setzero_si128());
    block = util::Xor(block, _mm_load_si128((const __m128i*)&rk[8]));
    // AES Round + XOR with round key 4
    block = util::aes_round(block, _mm_setzero_si128());
    block = util::Xor(block, _mm_load_si128((const __m128i*)&rk[12]));
    // AES round
    block = util::aes_round(block, _mm_setzero_si128());
    // Unpack + XOR with l values
    l0 ^= _mm_extract_epi32(block, 0);
    l1 ^= _mm_extract_epi32(block, 1);
    l2 ^= _mm_extract_epi32(block, 2);
    l3 ^= _mm_extract_epi32(block, 3);
}
} // namespace x86_aesni_shavite
} // namespace sapphire

#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
