// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_ARM_NEON)
#include <attributes.h>
#include <crypto/x11/util/util.hpp>

#include <cstdint>

#include <arm_neon.h>

namespace sapphire {
namespace {
uint8x16_t ALWAYS_INLINE gf8_mul2(const uint8x16_t& x)
{
    // (x << 1)
    const uint8x16_t lhs = vshlq_n_u8(x, 1);
    // (x & 0x80) ? 0xff : 0x00
    const uint8x16_t msb_set = vandq_u8(x, vmovq_n_u8(0x80));
    const uint8x16_t mask = vceqq_u8(msb_set, vmovq_n_u8(0x80));
    // Replace 0xff with 0x1b
    const uint8x16_t rhs = vandq_u8(mask, vmovq_n_u8(0x1b));
    // (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00))
    return util::Xor(lhs, rhs);
}

void ALWAYS_INLINE MixColumn(uint8x16_t& Wa, uint8x16_t& Wb, uint8x16_t& Wc, uint8x16_t& Wd)
{
    const uint8x16_t a = Wa;
    const uint8x16_t b = Wb;
    const uint8x16_t c = Wc;
    const uint8x16_t d = Wd;

    const uint8x16_t ab = util::Xor(a, b);
    const uint8x16_t bc = util::Xor(b, c);
    const uint8x16_t cd = util::Xor(c, d);

    const uint8x16_t abx = gf8_mul2(ab);
    const uint8x16_t bcx = gf8_mul2(bc);
    const uint8x16_t cdx = gf8_mul2(cd);

    // Wa = abx ^ bc ^ d
    Wa = util::Xor(util::Xor(abx, bc), d);
    // Wb = bcx ^ a ^ cd
    Wb = util::Xor(util::Xor(bcx, a), cd);
    // Wc = cdx ^ ab ^ d
    Wc = util::Xor(util::Xor(cdx, ab), d);
    // Wd = abx ^ bcx ^ cdx ^ ab ^ c
    Wd = util::Xor(util::Xor(util::Xor(util::Xor(abx, bcx), cdx), ab), c);
}

void ALWAYS_INLINE ShiftRow1(uint8x16_t& Wa, uint8x16_t& Wb, uint8x16_t& Wc, uint8x16_t& Wd)
{
    uint8x16_t tmp = Wa;
    Wa = Wb;
    Wb = Wc;
    Wc = Wd;
    Wd = tmp;
}

void ALWAYS_INLINE ShiftRow2(uint8x16_t& Wa, uint8x16_t& Wb, uint8x16_t& Wc, uint8x16_t& Wd)
{
    uint8x16_t tmp1 = Wa;
    uint8x16_t tmp2 = Wb;
    Wa = Wc;
    Wb = Wd;
    Wc = tmp1;
    Wd = tmp2;
}

void ALWAYS_INLINE ShiftRow3(uint8x16_t& Wa, uint8x16_t& Wb, uint8x16_t& Wc, uint8x16_t& Wd)
{
    uint8x16_t tmp = Wd;
    Wd = Wc;
    Wc = Wb;
    Wb = Wa;
    Wa = tmp;
}
} // anonymous namespace

namespace arm_neon_echo {
void ShiftAndMix(uint64_t W[16][2])
{
    alignas(16) uint8x16_t w[16];
    w[0] = vreinterpretq_u8_u64(vld1q_u64(&W[0][0]));
    w[1] = vreinterpretq_u8_u64(vld1q_u64(&W[1][0]));
    w[2] = vreinterpretq_u8_u64(vld1q_u64(&W[2][0]));
    w[3] = vreinterpretq_u8_u64(vld1q_u64(&W[3][0]));
    w[4] = vreinterpretq_u8_u64(vld1q_u64(&W[4][0]));
    w[5] = vreinterpretq_u8_u64(vld1q_u64(&W[5][0]));
    w[6] = vreinterpretq_u8_u64(vld1q_u64(&W[6][0]));
    w[7] = vreinterpretq_u8_u64(vld1q_u64(&W[7][0]));
    w[8] = vreinterpretq_u8_u64(vld1q_u64(&W[8][0]));
    w[9] = vreinterpretq_u8_u64(vld1q_u64(&W[9][0]));
    w[10] = vreinterpretq_u8_u64(vld1q_u64(&W[10][0]));
    w[11] = vreinterpretq_u8_u64(vld1q_u64(&W[11][0]));
    w[12] = vreinterpretq_u8_u64(vld1q_u64(&W[12][0]));
    w[13] = vreinterpretq_u8_u64(vld1q_u64(&W[13][0]));
    w[14] = vreinterpretq_u8_u64(vld1q_u64(&W[14][0]));
    w[15] = vreinterpretq_u8_u64(vld1q_u64(&W[15][0]));

    ShiftRow1(w[1], w[5], w[9], w[13]);
    ShiftRow2(w[2], w[6], w[10], w[14]);
    ShiftRow3(w[3], w[7], w[11], w[15]);

    MixColumn(w[0], w[1], w[2], w[3]);
    MixColumn(w[4], w[5], w[6], w[7]);
    MixColumn(w[8], w[9], w[10], w[11]);
    MixColumn(w[12], w[13], w[14], w[15]);

    vst1q_u64(&W[0][0], vreinterpretq_u64_u8(w[0]));
    vst1q_u64(&W[1][0], vreinterpretq_u64_u8(w[1]));
    vst1q_u64(&W[2][0], vreinterpretq_u64_u8(w[2]));
    vst1q_u64(&W[3][0], vreinterpretq_u64_u8(w[3]));
    vst1q_u64(&W[4][0], vreinterpretq_u64_u8(w[4]));
    vst1q_u64(&W[5][0], vreinterpretq_u64_u8(w[5]));
    vst1q_u64(&W[6][0], vreinterpretq_u64_u8(w[6]));
    vst1q_u64(&W[7][0], vreinterpretq_u64_u8(w[7]));
    vst1q_u64(&W[8][0], vreinterpretq_u64_u8(w[8]));
    vst1q_u64(&W[9][0], vreinterpretq_u64_u8(w[9]));
    vst1q_u64(&W[10][0], vreinterpretq_u64_u8(w[10]));
    vst1q_u64(&W[11][0], vreinterpretq_u64_u8(w[11]));
    vst1q_u64(&W[12][0], vreinterpretq_u64_u8(w[12]));
    vst1q_u64(&W[13][0], vreinterpretq_u64_u8(w[13]));
    vst1q_u64(&W[14][0], vreinterpretq_u64_u8(w[14]));
    vst1q_u64(&W[15][0], vreinterpretq_u64_u8(w[15]));
}
} // namespace arm_neon_echo
} // namespace sapphire

#endif // ENABLE_ARM_NEON
