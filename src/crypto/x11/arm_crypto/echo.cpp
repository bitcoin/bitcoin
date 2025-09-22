// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_ARM_AES)
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <arm_neon.h>

namespace sapphire {
namespace arm_crypto_echo {
void FullStateRound(uint64_t W[16][2], uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3)
{
    uint8x16_t key = util::pack_le(k0, k1, k2, k3);
    for (int n = 0; n < 16; n++) {
        uint8x16_t block = vreinterpretq_u8_u64(vld1q_u64(&W[n][0]));
        block = util::aes_round(block, key);
        block = util::aes_round_nk(block);
        vst1q_u64(&W[n][0], vreinterpretq_u64_u8(block));

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
} // namespace arm_crypto_echo
} // namespace sapphire

#endif // ENABLE_ARM_AES
