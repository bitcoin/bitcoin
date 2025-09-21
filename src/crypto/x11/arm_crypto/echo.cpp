// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_ARM_AES)
#include <crypto/x11/util/util.hpp>

#include <cstddef>

#include <arm_neon.h>

namespace sapphire {
namespace {
void ALWAYS_INLINE StateRound(uint64_t W[16][2], size_t idx, uint8x16_t& key, uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3)
{
    uint8x16_t block = vreinterpretq_u8_u64(vld1q_u64(&W[idx][0]));
    block = util::aes_round(block, key);
    block = util::aes_round_nk(block);
    vst1q_u64(&W[idx][0], vreinterpretq_u64_u8(block));

    util::unpack_le(key, k0, k1, k2, k3);
    if ((k0 = (k0 + 1)) == 0) {
        if ((k1 = (k1 + 1)) == 0) {
            if ((k2 = (k2 + 1)) == 0) {
                k3 = (k3 + 1);
            }
        }
    }
}
} // anonymous namespace

namespace arm_crypto_echo {
void FullStateRound(uint64_t W[16][2], uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3)
{
    uint8x16_t key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 0, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 1, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 2, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 3, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 4, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 5, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 6, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 7, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 8, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 9, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 10, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 11, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 12, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 13, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 14, key, k0, k1, k2, k3);
    key = util::pack_le(k0, k1, k2, k3);
    StateRound(W, 15, key, k0, k1, k2, k3);
}
} // namespace arm_crypto_echo
} // namespace sapphire

#endif // ENABLE_ARM_AES
