// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_ARM_AES)
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <arm_neon.h>

namespace sapphire {
namespace arm_crypto_shavite {
void CompressElement(uint32_t& l0, uint32_t& l1, uint32_t& l2, uint32_t& l3,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, const uint32_t* rk)
{
    // Pack block + XOR with round key 1
    uint8x16_t block = util::pack_le(r0, r1, r2, r3);
    block = util::Xor(block, vreinterpretq_u8_u32(vld1q_u32(&rk[0])));
    // AES round + XOR with round key 2
    block = util::Xor(util::aes_round_nk(block), vreinterpretq_u8_u32(vld1q_u32(&rk[4])));
    // AES round + XOR with round key 3
    block = util::Xor(util::aes_round_nk(block), vreinterpretq_u8_u32(vld1q_u32(&rk[8])));
    // AES Round + XOR with round key 4
    block = util::Xor(util::aes_round_nk(block), vreinterpretq_u8_u32(vld1q_u32(&rk[12])));
    // AES round
    block = util::aes_round_nk(block);
    // Unpack + XOR with l values
    uint32x4_t result = vreinterpretq_u32_u8(block);
    l0 ^= vgetq_lane_u32(result, 0);
    l1 ^= vgetq_lane_u32(result, 1);
    l2 ^= vgetq_lane_u32(result, 2);
    l3 ^= vgetq_lane_u32(result, 3);
}
} // namespace arm_crypto_shavite
} // namespace sapphire

#endif // ENABLE_ARM_AES
