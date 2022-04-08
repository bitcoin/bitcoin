// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Based on https://github.com/noloader/SHA-Intrinsics/blob/master/sha256-arm.c,
// Written and placed in public domain by Jeffrey Walton.
// Based on code from ARM, and by Johannes Schneiders, Skip Hovsmith and
// Barry O'Rourke for the mbedTLS project.
// Variant specialized for 64-byte inputs added by Pieter Wuille.

#ifdef ENABLE_ARM_SHANI

#include <array>
#include <cstdint>
#include <cstddef>
#include <arm_acle.h>
#include <arm_neon.h>

namespace {
alignas(uint32x4_t) static constexpr std::array<uint32_t, 64> K =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};
}

namespace sha256_arm_shani {
void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks)
{
    uint32x4_t STATE0, STATE1, ABEF_SAVE, CDGH_SAVE;
    uint32x4_t MSG0, MSG1, MSG2, MSG3;
    uint32x4_t TMP0, TMP2;

    // Load state
    STATE0 = vld1q_u32(&s[0]);
    STATE1 = vld1q_u32(&s[4]);

    while (blocks--)
    {
        // Save state
        ABEF_SAVE = STATE0;
        CDGH_SAVE = STATE1;

        // Load and convert input chunk to Big Endian
        MSG0 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(chunk + 0)));
        MSG1 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(chunk + 16)));
        MSG2 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(chunk + 32)));
        MSG3 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(chunk + 48)));
        chunk += 64;

        // Original implementation preloaded message and constant addition which was 1-3% slower.
        // Now included as first step in quad round code saving one Q Neon register
        // "TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0]));"

        // Rounds 1-4
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0]));
        TMP2 = STATE0;
        MSG0 = vsha256su0q_u32(MSG0, MSG1);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

        // Rounds 5-8
        TMP0 = vaddq_u32(MSG1, vld1q_u32(&K[4]));
        TMP2 = STATE0;
        MSG1 = vsha256su0q_u32(MSG1, MSG2);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

        // Rounds 9-12
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[8]));
        TMP2 = STATE0;
        MSG2 = vsha256su0q_u32(MSG2, MSG3);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

        // Rounds 13-16
        TMP0 = vaddq_u32(MSG3, vld1q_u32(&K[12]));
        TMP2 = STATE0;
        MSG3 = vsha256su0q_u32(MSG3, MSG0);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

        // Rounds 17-20
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[16]));
        TMP2 = STATE0;
        MSG0 = vsha256su0q_u32(MSG0, MSG1);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

        // Rounds 21-24
        TMP0 = vaddq_u32(MSG1, vld1q_u32(&K[20]));
        TMP2 = STATE0;
        MSG1 = vsha256su0q_u32(MSG1, MSG2);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

        // Rounds 25-28
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[24]));
        TMP2 = STATE0;
        MSG2 = vsha256su0q_u32(MSG2, MSG3);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

        // Rounds 29-32
        TMP0 = vaddq_u32(MSG3, vld1q_u32(&K[28]));
        TMP2 = STATE0;
        MSG3 = vsha256su0q_u32(MSG3, MSG0);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

        // Rounds 33-36
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[32]));
        TMP2 = STATE0;
        MSG0 = vsha256su0q_u32(MSG0, MSG1);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

        // Rounds 37-40
        TMP0 = vaddq_u32(MSG1, vld1q_u32(&K[36]));
        TMP2 = STATE0;
        MSG1 = vsha256su0q_u32(MSG1, MSG2);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

        // Rounds 41-44
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[40]));
        TMP2 = STATE0;
        MSG2 = vsha256su0q_u32(MSG2, MSG3);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

        // Rounds 45-48
        TMP0 = vaddq_u32(MSG3, vld1q_u32(&K[44]));
        TMP2 = STATE0;
        MSG3 = vsha256su0q_u32(MSG3, MSG0);
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
        MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

        // Rounds 49-52
        TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[48]));
        TMP2 = STATE0;
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

        // Rounds 53-56
        TMP0 = vaddq_u32(MSG1, vld1q_u32(&K[52]));
        TMP2 = STATE0;
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

        // Rounds 57-60
        TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[56]));
        TMP2 = STATE0;
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

        // Rounds 61-64
        TMP0 = vaddq_u32(MSG3, vld1q_u32(&K[60]));
        TMP2 = STATE0;
        STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
        STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

        // Update state
        STATE0 = vaddq_u32(STATE0, ABEF_SAVE);
        STATE1 = vaddq_u32(STATE1, CDGH_SAVE);
    }

    // Save final state
    vst1q_u32(&s[0], STATE0);
    vst1q_u32(&s[4], STATE1);
}
}

namespace sha256d64_arm_shani {
void Transform_2way(unsigned char* output, const unsigned char* input)
{
    /* Initial state. */
    alignas(uint32x4_t) static constexpr std::array<uint32_t, 8> INIT = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    /* Precomputed message schedule for the 2nd transform. */
    alignas(uint32x4_t) static constexpr std::array<uint32_t, 64> MIDS = {
        0xc28a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf374,
        0x649b69c1, 0xf0fe4786, 0x0fe1edc6, 0x240cf254,
        0x4fe9346f, 0x6cc984be, 0x61b9411e, 0x16f988fa,
        0xf2c65152, 0xa88e5a6d, 0xb019fc65, 0xb9d99ec7,
        0x9a1231c3, 0xe70eeaa0, 0xfdb1232b, 0xc7353eb0,
        0x3069bad5, 0xcb976d5f, 0x5a0f118f, 0xdc1eeefd,
        0x0a35b689, 0xde0b7a04, 0x58f4ca9d, 0xe15d5b16,
        0x007f3e86, 0x37088980, 0xa507ea32, 0x6fab9537,
        0x17406110, 0x0d8cd6f1, 0xcdaa3b6d, 0xc0bbbe37,
        0x83613bda, 0xdb48a363, 0x0b02e931, 0x6fd15ca7,
        0x521afaca, 0x31338431, 0x6ed41a95, 0x6d437890,
        0xc39c91f2, 0x9eccabbd, 0xb5c9a0e6, 0x532fb63c,
        0xd2c741c6, 0x07237ea3, 0xa4954b68, 0x4c191d76
    };

    /* A few precomputed message schedule values for the 3rd transform. */
    alignas(uint32x4_t) static constexpr std::array<uint32_t, 12> FINS = {
        0x5807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x80000000, 0x00000000, 0x00000000, 0x00000000,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf274
    };

    /* Padding processed in the 3rd transform (byteswapped). */
    alignas(uint32x4_t) static constexpr std::array<uint32_t, 8> FINAL = {0x80000000, 0, 0, 0, 0, 0, 0, 0x100};

    uint32x4_t STATE0A, STATE0B, STATE1A, STATE1B, ABEF_SAVEA, ABEF_SAVEB, CDGH_SAVEA, CDGH_SAVEB;
    uint32x4_t MSG0A, MSG0B, MSG1A, MSG1B, MSG2A, MSG2B, MSG3A, MSG3B;
    uint32x4_t TMP0A, TMP0B, TMP2A, TMP2B, TMP;

    // Transform 1: Load state
    STATE0A = vld1q_u32(&INIT[0]);
    STATE0B = STATE0A;
    STATE1A = vld1q_u32(&INIT[4]);
    STATE1B = STATE1A;

    // Transform 1: Load and convert input chunk to Big Endian
    MSG0A = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 0)));
    MSG1A = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 16)));
    MSG2A = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 32)));
    MSG3A = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 48)));
    MSG0B = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 64)));
    MSG1B = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 80)));
    MSG2B = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 96)));
    MSG3B = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(input + 112)));

    // Transform 1: Rounds 1-4
    TMP = vld1q_u32(&K[0]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG0A = vsha256su0q_u32(MSG0A, MSG1A);
    MSG0B = vsha256su0q_u32(MSG0B, MSG1B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG0A = vsha256su1q_u32(MSG0A, MSG2A, MSG3A);
    MSG0B = vsha256su1q_u32(MSG0B, MSG2B, MSG3B);

    // Transform 1: Rounds 5-8
    TMP = vld1q_u32(&K[4]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG1A = vsha256su0q_u32(MSG1A, MSG2A);
    MSG1B = vsha256su0q_u32(MSG1B, MSG2B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG1A = vsha256su1q_u32(MSG1A, MSG3A, MSG0A);
    MSG1B = vsha256su1q_u32(MSG1B, MSG3B, MSG0B);

    // Transform 1: Rounds 9-12
    TMP = vld1q_u32(&K[8]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG2A = vsha256su0q_u32(MSG2A, MSG3A);
    MSG2B = vsha256su0q_u32(MSG2B, MSG3B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG2A = vsha256su1q_u32(MSG2A, MSG0A, MSG1A);
    MSG2B = vsha256su1q_u32(MSG2B, MSG0B, MSG1B);

    // Transform 1: Rounds 13-16
    TMP = vld1q_u32(&K[12]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG3A = vsha256su0q_u32(MSG3A, MSG0A);
    MSG3B = vsha256su0q_u32(MSG3B, MSG0B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG3A = vsha256su1q_u32(MSG3A, MSG1A, MSG2A);
    MSG3B = vsha256su1q_u32(MSG3B, MSG1B, MSG2B);

    // Transform 1: Rounds 17-20
    TMP = vld1q_u32(&K[16]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG0A = vsha256su0q_u32(MSG0A, MSG1A);
    MSG0B = vsha256su0q_u32(MSG0B, MSG1B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG0A = vsha256su1q_u32(MSG0A, MSG2A, MSG3A);
    MSG0B = vsha256su1q_u32(MSG0B, MSG2B, MSG3B);

    // Transform 1: Rounds 21-24
    TMP = vld1q_u32(&K[20]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG1A = vsha256su0q_u32(MSG1A, MSG2A);
    MSG1B = vsha256su0q_u32(MSG1B, MSG2B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG1A = vsha256su1q_u32(MSG1A, MSG3A, MSG0A);
    MSG1B = vsha256su1q_u32(MSG1B, MSG3B, MSG0B);

    // Transform 1: Rounds 25-28
    TMP = vld1q_u32(&K[24]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG2A = vsha256su0q_u32(MSG2A, MSG3A);
    MSG2B = vsha256su0q_u32(MSG2B, MSG3B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG2A = vsha256su1q_u32(MSG2A, MSG0A, MSG1A);
    MSG2B = vsha256su1q_u32(MSG2B, MSG0B, MSG1B);

    // Transform 1: Rounds 29-32
    TMP = vld1q_u32(&K[28]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG3A = vsha256su0q_u32(MSG3A, MSG0A);
    MSG3B = vsha256su0q_u32(MSG3B, MSG0B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG3A = vsha256su1q_u32(MSG3A, MSG1A, MSG2A);
    MSG3B = vsha256su1q_u32(MSG3B, MSG1B, MSG2B);

    // Transform 1: Rounds 33-36
    TMP = vld1q_u32(&K[32]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG0A = vsha256su0q_u32(MSG0A, MSG1A);
    MSG0B = vsha256su0q_u32(MSG0B, MSG1B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG0A = vsha256su1q_u32(MSG0A, MSG2A, MSG3A);
    MSG0B = vsha256su1q_u32(MSG0B, MSG2B, MSG3B);

    // Transform 1: Rounds 37-40
    TMP = vld1q_u32(&K[36]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG1A = vsha256su0q_u32(MSG1A, MSG2A);
    MSG1B = vsha256su0q_u32(MSG1B, MSG2B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG1A = vsha256su1q_u32(MSG1A, MSG3A, MSG0A);
    MSG1B = vsha256su1q_u32(MSG1B, MSG3B, MSG0B);

    // Transform 1: Rounds 41-44
    TMP = vld1q_u32(&K[40]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG2A = vsha256su0q_u32(MSG2A, MSG3A);
    MSG2B = vsha256su0q_u32(MSG2B, MSG3B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG2A = vsha256su1q_u32(MSG2A, MSG0A, MSG1A);
    MSG2B = vsha256su1q_u32(MSG2B, MSG0B, MSG1B);

    // Transform 1: Rounds 45-48
    TMP = vld1q_u32(&K[44]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG3A = vsha256su0q_u32(MSG3A, MSG0A);
    MSG3B = vsha256su0q_u32(MSG3B, MSG0B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG3A = vsha256su1q_u32(MSG3A, MSG1A, MSG2A);
    MSG3B = vsha256su1q_u32(MSG3B, MSG1B, MSG2B);

    // Transform 1: Rounds 49-52
    TMP = vld1q_u32(&K[48]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 1: Rounds 53-56
    TMP = vld1q_u32(&K[52]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 1: Rounds 57-60
    TMP = vld1q_u32(&K[56]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 1: Rounds 61-64
    TMP = vld1q_u32(&K[60]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 1: Update state
    TMP = vld1q_u32(&INIT[0]);
    STATE0A = vaddq_u32(STATE0A, TMP);
    STATE0B = vaddq_u32(STATE0B, TMP);
    TMP = vld1q_u32(&INIT[4]);
    STATE1A = vaddq_u32(STATE1A, TMP);
    STATE1B = vaddq_u32(STATE1B, TMP);

    // Transform 2: Save state
    ABEF_SAVEA = STATE0A;
    ABEF_SAVEB = STATE0B;
    CDGH_SAVEA = STATE1A;
    CDGH_SAVEB = STATE1B;

    // Transform 2: Rounds 1-4
    TMP = vld1q_u32(&MIDS[0]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 5-8
    TMP = vld1q_u32(&MIDS[4]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 9-12
    TMP = vld1q_u32(&MIDS[8]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 13-16
    TMP = vld1q_u32(&MIDS[12]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 17-20
    TMP = vld1q_u32(&MIDS[16]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 21-24
    TMP = vld1q_u32(&MIDS[20]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 25-28
    TMP = vld1q_u32(&MIDS[24]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 29-32
    TMP = vld1q_u32(&MIDS[28]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 33-36
    TMP = vld1q_u32(&MIDS[32]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 37-40
    TMP = vld1q_u32(&MIDS[36]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 41-44
    TMP = vld1q_u32(&MIDS[40]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 45-48
    TMP = vld1q_u32(&MIDS[44]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 49-52
    TMP = vld1q_u32(&MIDS[48]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 53-56
    TMP = vld1q_u32(&MIDS[52]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 57-60
    TMP = vld1q_u32(&MIDS[56]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Rounds 61-64
    TMP = vld1q_u32(&MIDS[60]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);

    // Transform 2: Update state
    STATE0A = vaddq_u32(STATE0A, ABEF_SAVEA);
    STATE0B = vaddq_u32(STATE0B, ABEF_SAVEB);
    STATE1A = vaddq_u32(STATE1A, CDGH_SAVEA);
    STATE1B = vaddq_u32(STATE1B, CDGH_SAVEB);

    // Transform 3: Pad previous output
    MSG0A = STATE0A;
    MSG0B = STATE0B;
    MSG1A = STATE1A;
    MSG1B = STATE1B;
    MSG2A = vld1q_u32(&FINAL[0]);
    MSG2B = MSG2A;
    MSG3A = vld1q_u32(&FINAL[4]);
    MSG3B = MSG3A;

    // Transform 3: Load state
    STATE0A = vld1q_u32(&INIT[0]);
    STATE0B = STATE0A;
    STATE1A = vld1q_u32(&INIT[4]);
    STATE1B = STATE1A;

    // Transform 3: Rounds 1-4
    TMP = vld1q_u32(&K[0]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG0A = vsha256su0q_u32(MSG0A, MSG1A);
    MSG0B = vsha256su0q_u32(MSG0B, MSG1B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG0A = vsha256su1q_u32(MSG0A, MSG2A, MSG3A);
    MSG0B = vsha256su1q_u32(MSG0B, MSG2B, MSG3B);

    // Transform 3: Rounds 5-8
    TMP = vld1q_u32(&K[4]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG1A = vsha256su0q_u32(MSG1A, MSG2A);
    MSG1B = vsha256su0q_u32(MSG1B, MSG2B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG1A = vsha256su1q_u32(MSG1A, MSG3A, MSG0A);
    MSG1B = vsha256su1q_u32(MSG1B, MSG3B, MSG0B);

    // Transform 3: Rounds 9-12
    TMP = vld1q_u32(&FINS[0]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG2A = vld1q_u32(&FINS[4]);
    MSG2B = MSG2A;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);
    MSG2A = vsha256su1q_u32(MSG2A, MSG0A, MSG1A);
    MSG2B = vsha256su1q_u32(MSG2B, MSG0B, MSG1B);

    // Transform 3: Rounds 13-16
    TMP = vld1q_u32(&FINS[8]);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG3A = vsha256su0q_u32(MSG3A, MSG0A);
    MSG3B = vsha256su0q_u32(MSG3B, MSG0B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP);
    MSG3A = vsha256su1q_u32(MSG3A, MSG1A, MSG2A);
    MSG3B = vsha256su1q_u32(MSG3B, MSG1B, MSG2B);

    // Transform 3: Rounds 17-20
    TMP = vld1q_u32(&K[16]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG0A = vsha256su0q_u32(MSG0A, MSG1A);
    MSG0B = vsha256su0q_u32(MSG0B, MSG1B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG0A = vsha256su1q_u32(MSG0A, MSG2A, MSG3A);
    MSG0B = vsha256su1q_u32(MSG0B, MSG2B, MSG3B);

    // Transform 3: Rounds 21-24
    TMP = vld1q_u32(&K[20]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG1A = vsha256su0q_u32(MSG1A, MSG2A);
    MSG1B = vsha256su0q_u32(MSG1B, MSG2B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG1A = vsha256su1q_u32(MSG1A, MSG3A, MSG0A);
    MSG1B = vsha256su1q_u32(MSG1B, MSG3B, MSG0B);

    // Transform 3: Rounds 25-28
    TMP = vld1q_u32(&K[24]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG2A = vsha256su0q_u32(MSG2A, MSG3A);
    MSG2B = vsha256su0q_u32(MSG2B, MSG3B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG2A = vsha256su1q_u32(MSG2A, MSG0A, MSG1A);
    MSG2B = vsha256su1q_u32(MSG2B, MSG0B, MSG1B);

    // Transform 3: Rounds 29-32
    TMP = vld1q_u32(&K[28]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG3A = vsha256su0q_u32(MSG3A, MSG0A);
    MSG3B = vsha256su0q_u32(MSG3B, MSG0B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG3A = vsha256su1q_u32(MSG3A, MSG1A, MSG2A);
    MSG3B = vsha256su1q_u32(MSG3B, MSG1B, MSG2B);

    // Transform 3: Rounds 33-36
    TMP = vld1q_u32(&K[32]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG0A = vsha256su0q_u32(MSG0A, MSG1A);
    MSG0B = vsha256su0q_u32(MSG0B, MSG1B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG0A = vsha256su1q_u32(MSG0A, MSG2A, MSG3A);
    MSG0B = vsha256su1q_u32(MSG0B, MSG2B, MSG3B);

    // Transform 3: Rounds 37-40
    TMP = vld1q_u32(&K[36]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG1A = vsha256su0q_u32(MSG1A, MSG2A);
    MSG1B = vsha256su0q_u32(MSG1B, MSG2B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG1A = vsha256su1q_u32(MSG1A, MSG3A, MSG0A);
    MSG1B = vsha256su1q_u32(MSG1B, MSG3B, MSG0B);

    // Transform 3: Rounds 41-44
    TMP = vld1q_u32(&K[40]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG2A = vsha256su0q_u32(MSG2A, MSG3A);
    MSG2B = vsha256su0q_u32(MSG2B, MSG3B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG2A = vsha256su1q_u32(MSG2A, MSG0A, MSG1A);
    MSG2B = vsha256su1q_u32(MSG2B, MSG0B, MSG1B);

    // Transform 3: Rounds 45-48
    TMP = vld1q_u32(&K[44]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    MSG3A = vsha256su0q_u32(MSG3A, MSG0A);
    MSG3B = vsha256su0q_u32(MSG3B, MSG0B);
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);
    MSG3A = vsha256su1q_u32(MSG3A, MSG1A, MSG2A);
    MSG3B = vsha256su1q_u32(MSG3B, MSG1B, MSG2B);

    // Transform 3: Rounds 49-52
    TMP = vld1q_u32(&K[48]);
    TMP0A = vaddq_u32(MSG0A, TMP);
    TMP0B = vaddq_u32(MSG0B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 3: Rounds 53-56
    TMP = vld1q_u32(&K[52]);
    TMP0A = vaddq_u32(MSG1A, TMP);
    TMP0B = vaddq_u32(MSG1B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 3: Rounds 57-60
    TMP = vld1q_u32(&K[56]);
    TMP0A = vaddq_u32(MSG2A, TMP);
    TMP0B = vaddq_u32(MSG2B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 3: Rounds 61-64
    TMP = vld1q_u32(&K[60]);
    TMP0A = vaddq_u32(MSG3A, TMP);
    TMP0B = vaddq_u32(MSG3B, TMP);
    TMP2A = STATE0A;
    TMP2B = STATE0B;
    STATE0A = vsha256hq_u32(STATE0A, STATE1A, TMP0A);
    STATE0B = vsha256hq_u32(STATE0B, STATE1B, TMP0B);
    STATE1A = vsha256h2q_u32(STATE1A, TMP2A, TMP0A);
    STATE1B = vsha256h2q_u32(STATE1B, TMP2B, TMP0B);

    // Transform 3: Update state
    TMP = vld1q_u32(&INIT[0]);
    STATE0A = vaddq_u32(STATE0A, TMP);
    STATE0B = vaddq_u32(STATE0B, TMP);
    TMP = vld1q_u32(&INIT[4]);
    STATE1A = vaddq_u32(STATE1A, TMP);
    STATE1B = vaddq_u32(STATE1B, TMP);

    // Store result
    vst1q_u8(output, vrev32q_u8(vreinterpretq_u8_u32(STATE0A)));
    vst1q_u8(output + 16, vrev32q_u8(vreinterpretq_u8_u32(STATE1A)));
    vst1q_u8(output + 32, vrev32q_u8(vreinterpretq_u8_u32(STATE0B)));
    vst1q_u8(output + 48, vrev32q_u8(vreinterpretq_u8_u32(STATE1B)));
}
}

#endif
