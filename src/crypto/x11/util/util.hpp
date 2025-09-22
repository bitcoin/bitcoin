// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_X11_UTIL_UTIL_HPP
#define BITCOIN_CRYPTO_X11_UTIL_UTIL_HPP

#include <cstdint>

#if !defined(DISABLE_OPTIMIZED_SHA256)
#include <attributes.h>

#if defined(ENABLE_SSSE3) || (defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI))
#include <immintrin.h>
#endif // ENABLE_SSSE3 || (ENABLE_SSE41 && ENABLE_X86_AESNI)
#endif // !DISABLE_OPTIMIZED_SHA256

namespace sapphire {
namespace util {
constexpr inline uint32_t pack_le(uint8_t b3, uint8_t b2, uint8_t b1, uint8_t b0) noexcept
{
    return (static_cast<uint32_t>(b3) << 24) |
           (static_cast<uint32_t>(b2) << 16) |
           (static_cast<uint32_t>(b1) <<  8) |
           (static_cast<uint32_t>(b0));
}

#if !defined(DISABLE_OPTIMIZED_SHA256)
#if defined(ENABLE_SSSE3) || (defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI))
__m128i ALWAYS_INLINE Xor(const __m128i& x, const __m128i& y) { return _mm_xor_si128(x, y); }

#if defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI)
__m128i ALWAYS_INLINE aes_round(const __m128i& input, const __m128i& key) { return _mm_aesenc_si128(input, key); }

__m128i ALWAYS_INLINE pack_le(const uint32_t& w0, const uint32_t& w1, const uint32_t& w2, const uint32_t& w3)
{
    return _mm_set_epi32(w3, w2, w1, w0);
}

void ALWAYS_INLINE unpack_le(const __m128i& i, uint32_t& w0, uint32_t& w1, uint32_t& w2, uint32_t& w3)
{
    w0 = _mm_extract_epi32(i, 0);
    w1 = _mm_extract_epi32(i, 1);
    w2 = _mm_extract_epi32(i, 2);
    w3 = _mm_extract_epi32(i, 3);
}
#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
#endif // ENABLE_SSSE3 || (ENABLE_SSE41 && ENABLE_X86_AESNI)
#endif // !DISABLE_OPTIMIZED_SHA256
} // namespace util
} // namespace sapphire

#endif // BITCOIN_CRYPTO_X11_UTIL_UTIL_HPP
