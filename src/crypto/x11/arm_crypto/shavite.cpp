// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(ENABLE_ARM_AES)
#include <crypto/x11/sph_shavite.h>
#include <crypto/x11/util/util.hpp>

#include <cstdint>
#include <cstring>

#include <arm_neon.h>

namespace sapphire {
namespace {
void CompressElement(uint32_t& l0, uint32_t& l1, uint32_t& l2, uint32_t& l3,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, const uint8x16_t* rk_words)
{
    uint8x16_t block = util::pack_le(r0, r1, r2, r3);
    block = util::Xor(block, rk_words[0]);
    block = util::Xor(util::aes_round_nk(block), rk_words[1]);
    block = util::Xor(util::aes_round_nk(block), rk_words[2]);
    block = util::Xor(util::aes_round_nk(block), rk_words[3]);
    block = util::aes_round_nk(block);
    uint32x4_t result = vreinterpretq_u32_u8(block);
    l0 ^= vgetq_lane_u32(result, 0);
    l1 ^= vgetq_lane_u32(result, 1);
    l2 ^= vgetq_lane_u32(result, 2);
    l3 ^= vgetq_lane_u32(result, 3);
}
} // anonymous namespace

namespace arm_crypto_shavite {
void Compress(sph_shavite_big_context *sc, const void *msg)
{
    uint32_t p0, p1, p2, p3, p4, p5, p6, p7;
    uint32_t p8, p9, pA, pB, pC, pD, pE, pF;

    alignas(16) uint8x16_t rk_words[448/4];
    alignas(16) uint32_t rk[448];

#if SPH_LITTLE_ENDIAN
    memcpy(rk, msg, 128);
#else
    for (size_t u{0}; u < 32; u += 4) {
        rk[u + 0] = sph_dec32le_aligned(
            (const unsigned char *)msg + (u << 2) +  0);
        rk[u + 1] = sph_dec32le_aligned(
            (const unsigned char *)msg + (u << 2) +  4);
        rk[u + 2] = sph_dec32le_aligned(
            (const unsigned char *)msg + (u << 2) +  8);
        rk[u + 3] = sph_dec32le_aligned(
            (const unsigned char *)msg + (u << 2) + 12);
    }
#endif

    size_t u{32};
    for (;;) {
        for (int s{0}; s < 4; s++) {
            uint32_t x0 = rk[u - 31];
            uint32_t x1 = rk[u - 30];
            uint32_t x2 = rk[u - 29];
            uint32_t x3 = rk[u - 32];

            uint32x4_t block = vreinterpretq_u32_u8(util::aes_round_nk(util::pack_le(x0, x1, x2, x3)));
            rk[u + 0] = vgetq_lane_u32(block, 0) ^ rk[u - 4];
            rk[u + 1] = vgetq_lane_u32(block, 1) ^ rk[u - 3];
            rk[u + 2] = vgetq_lane_u32(block, 2) ^ rk[u - 2];
            rk[u + 3] = vgetq_lane_u32(block, 3) ^ rk[u - 1];

            if (u == 32) {
                rk[32] ^= sc->count0;
                rk[33] ^= sc->count1;
                rk[34] ^= sc->count2;
                rk[35] ^= SPH_T32(~sc->count3);
            } else if (u == 440) {
                rk[440] ^= sc->count1;
                rk[441] ^= sc->count0;
                rk[442] ^= sc->count3;
                rk[443] ^= SPH_T32(~sc->count2);
            }
            u += 4;

            x0 = rk[u - 31];
            x1 = rk[u - 30];
            x2 = rk[u - 29];
            x3 = rk[u - 32];

            block = vreinterpretq_u32_u8(util::aes_round_nk(util::pack_le(x0, x1, x2, x3)));
            rk[u + 0] = vgetq_lane_u32(block, 0) ^ rk[u - 4];
            rk[u + 1] = vgetq_lane_u32(block, 1) ^ rk[u - 3];
            rk[u + 2] = vgetq_lane_u32(block, 2) ^ rk[u - 2];
            rk[u + 3] = vgetq_lane_u32(block, 3) ^ rk[u - 1];

            if (u == 164) {
                rk[164] ^= sc->count3;
                rk[165] ^= sc->count2;
                rk[166] ^= sc->count1;
                rk[167] ^= SPH_T32(~sc->count0);
            } else if (u == 316) {
                rk[316] ^= sc->count2;
                rk[317] ^= sc->count3;
                rk[318] ^= sc->count0;
                rk[319] ^= SPH_T32(~sc->count1);
            }
            u += 4;
        }
        if (u == 448)
            break;
        for (int s = 0; s < 8; s++) {
            rk[u + 0] = rk[u - 32] ^ rk[u - 7];
            rk[u + 1] = rk[u - 31] ^ rk[u - 6];
            rk[u + 2] = rk[u - 30] ^ rk[u - 5];
            rk[u + 3] = rk[u - 29] ^ rk[u - 4];
            u += 4;
            if (u == 448)
                break;
        }
    }

    for (int i{0}; i < (448/4); i++) {
        rk_words[i] = vreinterpretq_u8_u32(vld1q_u32(&rk[i*4]));
    }

    p0 = sc->h[0x0];
    p1 = sc->h[0x1];
    p2 = sc->h[0x2];
    p3 = sc->h[0x3];
    p4 = sc->h[0x4];
    p5 = sc->h[0x5];
    p6 = sc->h[0x6];
    p7 = sc->h[0x7];
    p8 = sc->h[0x8];
    p9 = sc->h[0x9];
    pA = sc->h[0xA];
    pB = sc->h[0xB];
    pC = sc->h[0xC];
    pD = sc->h[0xD];
    pE = sc->h[0xE];
    pF = sc->h[0xF];

    size_t u_words{0};
    for (size_t r{0}; r < 14; r++) {
        CompressElement(p0, p1, p2, p3, p4, p5, p6, p7, &rk_words[u_words]);
        u_words += 4;
        CompressElement(p8, p9, pA, pB, pC, pD, pE, pF, &rk_words[u_words]);
        u_words += 4;

#define WROT(a, b, c, d)   do { \
        uint32_t t = d; \
        d = c; \
        c = b; \
        b = a; \
        a = t; \
    } while (0)

        WROT(p0, p4, p8, pC);
        WROT(p1, p5, p9, pD);
        WROT(p2, p6, pA, pE);
        WROT(p3, p7, pB, pF);

#undef WROT
    }

    sc->h[0x0] ^= p0;
    sc->h[0x1] ^= p1;
    sc->h[0x2] ^= p2;
    sc->h[0x3] ^= p3;
    sc->h[0x4] ^= p4;
    sc->h[0x5] ^= p5;
    sc->h[0x6] ^= p6;
    sc->h[0x7] ^= p7;
    sc->h[0x8] ^= p8;
    sc->h[0x9] ^= p9;
    sc->h[0xA] ^= pA;
    sc->h[0xB] ^= pB;
    sc->h[0xC] ^= pC;
    sc->h[0xD] ^= pD;
    sc->h[0xE] ^= pE;
    sc->h[0xF] ^= pF;
}
} // namespace arm_crypto_shavite
} // namespace sapphire

#endif // ENABLE_ARM_AES
