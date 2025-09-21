// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI)
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <immintrin.h>
#include <wmmintrin.h>

namespace sapphire {
namespace x86_aesni_echo {
void FullStateRound(uint64_t W[16][2], uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3)
{
    __m128i key = util::pack_le(k0, k1, k2, k3);
    for (int n = 0; n < 16; n++) {
        __m128i block = _mm_load_si128((const __m128i*)&W[n][0]);
        block = util::aes_round(block, key);
        block = util::aes_round(block, _mm_setzero_si128());
        _mm_store_si128((__m128i*)&W[n][0], block);

        util::unpack_le(key, k0, k1, k2, k3);
        if ((k0 = (k0 + 1)) == 0) {
            if ((k1 = (k1 + 1)) == 0) {
                if ((k2 = (k2 + 1)) == 0) {
                    k3 = (k3 + 1);
                }
            }
        }
        key = util::pack_le(k0, k1, k2, k3);
    }
    util::unpack_le(key, k0, k1, k2, k3);
}
} // namespace x86_aesni_echo
} // namespace sapphire

#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
