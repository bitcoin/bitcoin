 /*********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

/* Constant time, unoptimized, concise, plain C, AES implementation
 * Based On:
 *   Emilia Kasper and Peter Schwabe, Faster and Timing-Attack Resistant AES-GCM
 *   http://www.iacr.org/archive/ches2009/57470001/57470001.pdf
 * But using 8 16-bit integers representing a single AES state rather than 8 128-bit
 * integers representing 8 AES states.
 */

#include "ctaes.h"

/* Slice variable slice_i contains the i'th bit of the 16 state variables in this order:
 *  0  1  2  3
 *  4  5  6  7
 *  8  9 10 11
 * 12 13 14 15
 */

/** Convert a byte to sliced form, storing it corresponding to given row and column in s */
static void LoadByte(AES_state* s, unsigned char byte, int r, int c) {
    int i;
    for (i = 0; i < 8; i++) {
        s->slice[i] |= (byte & 1) << (r * 4 + c);
        byte >>= 1;
    }
}

/** Load 16 bytes of data into 8 sliced integers */
static void LoadBytes(AES_state *s, const unsigned char* data16) {
    int c;
    for (c = 0; c < 4; c++) {
        int r;
        for (r = 0; r < 4; r++) {
            LoadByte(s, *(data16++), r, c);
        }
    }
}

/** Convert 8 sliced integers into 16 bytes of data */
static void SaveBytes(unsigned char* data16, const AES_state *s) {
    int c;
    for (c = 0; c < 4; c++) {
        int r;
        for (r = 0; r < 4; r++) {
            int b;
            uint8_t v = 0;
            for (b = 0; b < 8; b++) {
                v |= ((s->slice[b] >> (r * 4 + c)) & 1) << b;
            }
            *(data16++) = v;
        }
    }
}

/* S-box implementation based on the gate logic from:
 *   Joan Boyar and Rene Peralta, A depth-16 circuit for the AES S-box.
 *   https://eprint.iacr.org/2011/332.pdf
*/
static void SubBytes(AES_state *s, int inv) {
    /* Load the bit slices */
    uint16_t U0 = s->slice[7], U1 = s->slice[6], U2 = s->slice[5], U3 = s->slice[4];
    uint16_t U4 = s->slice[3], U5 = s->slice[2], U6 = s->slice[1], U7 = s->slice[0];

    uint16_t T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16;
    uint16_t T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, D;
    uint16_t M1, M6, M11, M13, M15, M20, M21, M22, M23, M25, M37, M38, M39, M40;
    uint16_t M41, M42, M43, M44, M45, M46, M47, M48, M49, M50, M51, M52, M53, M54;
    uint16_t M55, M56, M57, M58, M59, M60, M61, M62, M63;

    if (inv) {
        uint16_t R5, R13, R17, R18, R19;
        /* Undo linear postprocessing */
        T23 = U0 ^ U3;
        T22 = ~(U1 ^ U3);
        T2 = ~(U0 ^ U1);
        T1 = U3 ^ U4;
        T24 = ~(U4 ^ U7);
        R5 = U6 ^ U7;
        T8 = ~(U1 ^ T23);
        T19 = T22 ^ R5;
        T9 = ~(U7 ^ T1);
        T10 = T2 ^ T24;
        T13 = T2 ^ R5;
        T3 = T1 ^ R5;
        T25 = ~(U2 ^ T1);
        R13 = U1 ^ U6;
        T17 = ~(U2 ^ T19);
        T20 = T24 ^ R13;
        T4 = U4 ^ T8;
        R17 = ~(U2 ^ U5);
        R18 = ~(U5 ^ U6);
        R19 = ~(U2 ^ U4);
        D = U0 ^ R17;
        T6 = T22 ^ R17;
        T16 = R13 ^ R19;
        T27 = T1 ^ R18;
        T15 = T10 ^ T27;
        T14 = T10 ^ R18;
        T26 = T3 ^ T16;
    } else {
        /* Linear preprocessing. */
        T1 = U0 ^ U3;
        T2 = U0 ^ U5;
        T3 = U0 ^ U6;
        T4 = U3 ^ U5;
        T5 = U4 ^ U6;
        T6 = T1 ^ T5;
        T7 = U1 ^ U2;
        T8 = U7 ^ T6;
        T9 = U7 ^ T7;
        T10 = T6 ^ T7;
        T11 = U1 ^ U5;
        T12 = U2 ^ U5;
        T13 = T3 ^ T4;
        T14 = T6 ^ T11;
        T15 = T5 ^ T11;
        T16 = T5 ^ T12;
        T17 = T9 ^ T16;
        T18 = U3 ^ U7;
        T19 = T7 ^ T18;
        T20 = T1 ^ T19;
        T21 = U6 ^ U7;
        T22 = T7 ^ T21;
        T23 = T2 ^ T22;
        T24 = T2 ^ T10;
        T25 = T20 ^ T17;
        T26 = T3 ^ T16;
        T27 = T1 ^ T12;
        D = U7;
    }

    /* Non-linear transformation (identical to the code in SubBytes) */
    M1 = T13 & T6;
    M6 = T3 & T16;
    M11 = T1 & T15;
    M13 = (T4 & T27) ^ M11;
    M15 = (T2 & T10) ^ M11;
    M20 = T14 ^ M1 ^ (T23 & T8) ^ M13;
    M21 = (T19 & D) ^ M1 ^ T24 ^ M15;
    M22 = T26 ^ M6 ^ (T22 & T9) ^ M13;
    M23 = (T20 & T17) ^ M6 ^ M15 ^ T25;
    M25 = M22 & M20;
    M37 = M21 ^ ((M20 ^ M21) & (M23 ^ M25));
    M38 = M20 ^ M25 ^ (M21 | (M20 & M23));
    M39 = M23 ^ ((M22 ^ M23) & (M21 ^ M25));
    M40 = M22 ^ M25 ^ (M23 | (M21 & M22));
    M41 = M38 ^ M40;
    M42 = M37 ^ M39;
    M43 = M37 ^ M38;
    M44 = M39 ^ M40;
    M45 = M42 ^ M41;
    M46 = M44 & T6;
    M47 = M40 & T8;
    M48 = M39 & D;
    M49 = M43 & T16;
    M50 = M38 & T9;
    M51 = M37 & T17;
    M52 = M42 & T15;
    M53 = M45 & T27;
    M54 = M41 & T10;
    M55 = M44 & T13;
    M56 = M40 & T23;
    M57 = M39 & T19;
    M58 = M43 & T3;
    M59 = M38 & T22;
    M60 = M37 & T20;
    M61 = M42 & T1;
    M62 = M45 & T4;
    M63 = M41 & T2;

    if (inv){
        /* Undo linear preprocessing */
        uint16_t P0 = M52 ^ M61;
        uint16_t P1 = M58 ^ M59;
        uint16_t P2 = M54 ^ M62;
        uint16_t P3 = M47 ^ M50;
        uint16_t P4 = M48 ^ M56;
        uint16_t P5 = M46 ^ M51;
        uint16_t P6 = M49 ^ M60;
        uint16_t P7 = P0 ^ P1;
        uint16_t P8 = M50 ^ M53;
        uint16_t P9 = M55 ^ M63;
        uint16_t P10 = M57 ^ P4;
        uint16_t P11 = P0 ^ P3;
        uint16_t P12 = M46 ^ M48;
        uint16_t P13 = M49 ^ M51;
        uint16_t P14 = M49 ^ M62;
        uint16_t P15 = M54 ^ M59;
        uint16_t P16 = M57 ^ M61;
        uint16_t P17 = M58 ^ P2;
        uint16_t P18 = M63 ^ P5;
        uint16_t P19 = P2 ^ P3;
        uint16_t P20 = P4 ^ P6;
        uint16_t P22 = P2 ^ P7;
        uint16_t P23 = P7 ^ P8;
        uint16_t P24 = P5 ^ P7;
        uint16_t P25 = P6 ^ P10;
        uint16_t P26 = P9 ^ P11;
        uint16_t P27 = P10 ^ P18;
        uint16_t P28 = P11 ^ P25;
        uint16_t P29 = P15 ^ P20;
        s->slice[7] = P13 ^ P22;
        s->slice[6] = P26 ^ P29;
        s->slice[5] = P17 ^ P28;
        s->slice[4] = P12 ^ P22;
        s->slice[3] = P23 ^ P27;
        s->slice[2] = P19 ^ P24;
        s->slice[1] = P14 ^ P23;
        s->slice[0] = P9 ^ P16;
    } else {
        /* Linear postprocessing */
        uint16_t L0 = M61 ^ M62;
        uint16_t L1 = M50 ^ M56;
        uint16_t L2 = M46 ^ M48;
        uint16_t L3 = M47 ^ M55;
        uint16_t L4 = M54 ^ M58;
        uint16_t L5 = M49 ^ M61;
        uint16_t L6 = M62 ^ L5;
        uint16_t L7 = M46 ^ L3;
        uint16_t L8 = M51 ^ M59;
        uint16_t L9 = M52 ^ M53;
        uint16_t L10 = M53 ^ L4;
        uint16_t L11 = M60 ^ L2;
        uint16_t L12 = M48 ^ M51;
        uint16_t L13 = M50 ^ L0;
        uint16_t L14 = M52 ^ M61;
        uint16_t L15 = M55 ^ L1;
        uint16_t L16 = M56 ^ L0;
        uint16_t L17 = M57 ^ L1;
        uint16_t L18 = M58 ^ L8;
        uint16_t L19 = M63 ^ L4;
        uint16_t L20 = L0 ^ L1;
        uint16_t L21 = L1 ^ L7;
        uint16_t L22 = L3 ^ L12;
        uint16_t L23 = L18 ^ L2;
        uint16_t L24 = L15 ^ L9;
        uint16_t L25 = L6 ^ L10;
        uint16_t L26 = L7 ^ L9;
        uint16_t L27 = L8 ^ L10;
        uint16_t L28 = L11 ^ L14;
        uint16_t L29 = L11 ^ L17;
        s->slice[7] = L6 ^ L24;
        s->slice[6] = ~(L16 ^ L26);
        s->slice[5] = ~(L19 ^ L28);
        s->slice[4] = L6 ^ L21;
        s->slice[3] = L20 ^ L22;
        s->slice[2] = L25 ^ L29;
        s->slice[1] = ~(L13 ^ L27);
        s->slice[0] = ~(L6 ^ L23);
    }
}

#define BIT_RANGE(from,to) (((1 << ((to) - (from))) - 1) << (from))

#define BIT_RANGE_LEFT(x,from,to,shift) (((x) & BIT_RANGE((from), (to))) << (shift))
#define BIT_RANGE_RIGHT(x,from,to,shift) (((x) & BIT_RANGE((from), (to))) >> (shift))

static void ShiftRows(AES_state* s) {
    int i;
    for (i = 0; i < 8; i++) {
        uint16_t v = s->slice[i];
        s->slice[i] =
            (v & BIT_RANGE(0, 4)) |
            BIT_RANGE_LEFT(v, 4, 5, 3) | BIT_RANGE_RIGHT(v, 5, 8, 1) |
            BIT_RANGE_LEFT(v, 8, 10, 2) | BIT_RANGE_RIGHT(v, 10, 12, 2) |
            BIT_RANGE_LEFT(v, 12, 15, 1) | BIT_RANGE_RIGHT(v, 15, 16, 3);
    }
}

static void InvShiftRows(AES_state* s) {
    int i;
    for (i = 0; i < 8; i++) {
        uint16_t v = s->slice[i];
        s->slice[i] =
            (v & BIT_RANGE(0, 4)) |
            BIT_RANGE_LEFT(v, 4, 7, 1) | BIT_RANGE_RIGHT(v, 7, 8, 3) |
            BIT_RANGE_LEFT(v, 8, 10, 2) | BIT_RANGE_RIGHT(v, 10, 12, 2) |
            BIT_RANGE_LEFT(v, 12, 13, 3) | BIT_RANGE_RIGHT(v, 13, 16, 1);
    }
}

#define ROT(x,b) (((x) >> ((b) * 4)) | ((x) << ((4-(b)) * 4)))

static void MixColumns(AES_state* s, int inv) {
    /* The MixColumns transform treats the bytes of the columns of the state as
     * coefficients of a 3rd degree polynomial over GF(2^8) and multiplies them
     * by the fixed polynomial a(x) = {03}x^3 + {01}x^2 + {01}x + {02}, modulo
     * x^4 + {01}.
     *
     * In the inverse transform, we multiply by the inverse of a(x),
     * a^-1(x) = {0b}x^3 + {0d}x^2 + {09}x + {0e}. This is equal to
     * a(x) * ({04}x^2 + {05}), so we can reuse the forward transform's code
     * (found in OpenSSL's bsaes-x86_64.pl, attributed to Jussi Kivilinna)
     *
     * In the bitsliced representation, a multiplication of every column by x
     * mod x^4 + 1 is simply a right rotation.
     */

    /* Shared for both directions is a multiplication by a(x), which can be
     * rewritten as (x^3 + x^2 + x) + {02}*(x^3 + {01}).
     *
     * First compute s into the s? variables, (x^3 + {01}) * s into the s?_01
     * variables and (x^3 + x^2 + x)*s into the s?_123 variables.
     */
    uint16_t s0 = s->slice[0], s1 = s->slice[1], s2 = s->slice[2], s3 = s->slice[3];
    uint16_t s4 = s->slice[4], s5 = s->slice[5], s6 = s->slice[6], s7 = s->slice[7];
    uint16_t s0_01 = s0 ^ ROT(s0, 1), s0_123 = ROT(s0_01, 1) ^ ROT(s0, 3);
    uint16_t s1_01 = s1 ^ ROT(s1, 1), s1_123 = ROT(s1_01, 1) ^ ROT(s1, 3);
    uint16_t s2_01 = s2 ^ ROT(s2, 1), s2_123 = ROT(s2_01, 1) ^ ROT(s2, 3);
    uint16_t s3_01 = s3 ^ ROT(s3, 1), s3_123 = ROT(s3_01, 1) ^ ROT(s3, 3);
    uint16_t s4_01 = s4 ^ ROT(s4, 1), s4_123 = ROT(s4_01, 1) ^ ROT(s4, 3);
    uint16_t s5_01 = s5 ^ ROT(s5, 1), s5_123 = ROT(s5_01, 1) ^ ROT(s5, 3);
    uint16_t s6_01 = s6 ^ ROT(s6, 1), s6_123 = ROT(s6_01, 1) ^ ROT(s6, 3);
    uint16_t s7_01 = s7 ^ ROT(s7, 1), s7_123 = ROT(s7_01, 1) ^ ROT(s7, 3);
    /* Now compute s = s?_123 + {02} * s?_01. */
    s->slice[0] = s7_01 ^ s0_123;
    s->slice[1] = s7_01 ^ s0_01 ^ s1_123;
    s->slice[2] = s1_01 ^ s2_123;
    s->slice[3] = s7_01 ^ s2_01 ^ s3_123;
    s->slice[4] = s7_01 ^ s3_01 ^ s4_123;
    s->slice[5] = s4_01 ^ s5_123;
    s->slice[6] = s5_01 ^ s6_123;
    s->slice[7] = s6_01 ^ s7_123;
    if (inv) {
        /* In the reverse direction, we further need to multiply by
         * {04}x^2 + {05}, which can be written as {04} * (x^2 + {01}) + {01}.
         *
         * First compute (x^2 + {01}) * s into the t?_02 variables: */
        uint16_t t0_02 = s->slice[0] ^ ROT(s->slice[0], 2);
        uint16_t t1_02 = s->slice[1] ^ ROT(s->slice[1], 2);
        uint16_t t2_02 = s->slice[2] ^ ROT(s->slice[2], 2);
        uint16_t t3_02 = s->slice[3] ^ ROT(s->slice[3], 2);
        uint16_t t4_02 = s->slice[4] ^ ROT(s->slice[4], 2);
        uint16_t t5_02 = s->slice[5] ^ ROT(s->slice[5], 2);
        uint16_t t6_02 = s->slice[6] ^ ROT(s->slice[6], 2);
        uint16_t t7_02 = s->slice[7] ^ ROT(s->slice[7], 2);
        /* And then update s += {04} * t?_02 */
        s->slice[0] ^= t6_02;
        s->slice[1] ^= t6_02 ^ t7_02;
        s->slice[2] ^= t0_02 ^ t7_02;
        s->slice[3] ^= t1_02 ^ t6_02;
        s->slice[4] ^= t2_02 ^ t6_02 ^ t7_02;
        s->slice[5] ^= t3_02 ^ t7_02;
        s->slice[6] ^= t4_02;
        s->slice[7] ^= t5_02;
    }
}

static void AddRoundKey(AES_state* s, const AES_state* round) {
    int b;
    for (b = 0; b < 8; b++) {
        s->slice[b] ^= round->slice[b];
    }
}

/** column_0(s) = column_c(a) */
static void GetOneColumn(AES_state* s, const AES_state* a, int c) {
    int b;
    for (b = 0; b < 8; b++) {
        s->slice[b] = (a->slice[b] >> c) & 0x1111;
    }
}

/** column_c1(r) |= (column_0(s) ^= column_c2(a)) */
static void KeySetupColumnMix(AES_state* s, AES_state* r, const AES_state* a, int c1, int c2) {
    int b;
    for (b = 0; b < 8; b++) {
        r->slice[b] |= ((s->slice[b] ^= ((a->slice[b] >> c2) & 0x1111)) & 0x1111) << c1;
    }
}

/** Rotate the rows in s one position upwards, and xor in r */
static void KeySetupTransform(AES_state* s, const AES_state* r) {
    int b;
    for (b = 0; b < 8; b++) {
        s->slice[b] = ((s->slice[b] >> 4) | (s->slice[b] << 12)) ^ r->slice[b];
    }
}

/* Multiply the cells in s by x, as polynomials over GF(2) mod x^8 + x^4 + x^3 + x + 1 */
static void MultX(AES_state* s) {
    uint16_t top = s->slice[7];
    s->slice[7] = s->slice[6];
    s->slice[6] = s->slice[5];
    s->slice[5] = s->slice[4];
    s->slice[4] = s->slice[3] ^ top;
    s->slice[3] = s->slice[2] ^ top;
    s->slice[2] = s->slice[1];
    s->slice[1] = s->slice[0] ^ top;
    s->slice[0] = top;
}

/** Expand the cipher key into the key schedule.
 *
 *  state must be a pointer to an array of size nrounds + 1.
 *  key must be a pointer to 4 * nkeywords bytes.
 *
 *  AES128 uses nkeywords = 4, nrounds = 10
 *  AES192 uses nkeywords = 6, nrounds = 12
 *  AES256 uses nkeywords = 8, nrounds = 14
 */
static void AES_setup(AES_state* rounds, const uint8_t* key, int nkeywords, int nrounds)
{
    int i;

    /* The one-byte round constant */
    AES_state rcon = {{1,0,0,0,0,0,0,0}};
    /* The number of the word being generated, modulo nkeywords */
    int pos = 0;
    /* The column representing the word currently being processed */
    AES_state column;

    for (i = 0; i < nrounds + 1; i++) {
        int b;
        for (b = 0; b < 8; b++) {
            rounds[i].slice[b] = 0;
        }
    }

    /* The first nkeywords round columns are just taken from the key directly. */
    for (i = 0; i < nkeywords; i++) {
        int r;
        for (r = 0; r < 4; r++) {
            LoadByte(&rounds[i >> 2], *(key++), r, i & 3);
        }
    }

    GetOneColumn(&column, &rounds[(nkeywords - 1) >> 2], (nkeywords - 1) & 3);

    for (i = nkeywords; i < 4 * (nrounds + 1); i++) {
        /* Transform column */
        if (pos == 0) {
            SubBytes(&column, 0);
            KeySetupTransform(&column, &rcon);
            MultX(&rcon);
        } else if (nkeywords > 6 && pos == 4) {
            SubBytes(&column, 0);
        }
        if (++pos == nkeywords) pos = 0;
        KeySetupColumnMix(&column, &rounds[i >> 2], &rounds[(i - nkeywords) >> 2], i & 3, (i - nkeywords) & 3);
    }
}

static void AES_encrypt(const AES_state* rounds, int nrounds, unsigned char* cipher16, const unsigned char* plain16) {
    AES_state s = {{0}};
    int round;

    LoadBytes(&s, plain16);
    AddRoundKey(&s, rounds++);

    for (round = 1; round < nrounds; round++) {
        SubBytes(&s, 0);
        ShiftRows(&s);
        MixColumns(&s, 0);
        AddRoundKey(&s, rounds++);
    }

    SubBytes(&s, 0);
    ShiftRows(&s);
    AddRoundKey(&s, rounds);

    SaveBytes(cipher16, &s);
}

static void AES_decrypt(const AES_state* rounds, int nrounds, unsigned char* plain16, const unsigned char* cipher16) {
    /* Most AES decryption implementations use the alternate scheme
     * (the Equivalent Inverse Cipher), which looks more like encryption, but
     * needs different round constants. We can't reuse any code here anyway, so
     * don't bother. */
    AES_state s = {{0}};
    int round;

    rounds += nrounds;

    LoadBytes(&s, cipher16);
    AddRoundKey(&s, rounds--);

    for (round = 1; round < nrounds; round++) {
        InvShiftRows(&s);
        SubBytes(&s, 1);
        AddRoundKey(&s, rounds--);
        MixColumns(&s, 1);
    }

    InvShiftRows(&s);
    SubBytes(&s, 1);
    AddRoundKey(&s, rounds);

    SaveBytes(plain16, &s);
}

void AES128_init(AES128_ctx* ctx, const unsigned char* key16) {
    AES_setup(ctx->rk, key16, 4, 10);
}

void AES128_encrypt(const AES128_ctx* ctx, size_t blocks, unsigned char* cipher16, const unsigned char* plain16) {
    while (blocks--) {
        AES_encrypt(ctx->rk, 10, cipher16, plain16);
        cipher16 += 16;
        plain16 += 16;
    }
}

void AES128_decrypt(const AES128_ctx* ctx, size_t blocks, unsigned char* plain16, const unsigned char* cipher16) {
    while (blocks--) {
        AES_decrypt(ctx->rk, 10, plain16, cipher16);
        cipher16 += 16;
        plain16 += 16;
    }
}

void AES192_init(AES192_ctx* ctx, const unsigned char* key24) {
    AES_setup(ctx->rk, key24, 6, 12);
}

void AES192_encrypt(const AES192_ctx* ctx, size_t blocks, unsigned char* cipher16, const unsigned char* plain16) {
    while (blocks--) {
        AES_encrypt(ctx->rk, 12, cipher16, plain16);
        cipher16 += 16;
        plain16 += 16;
    }

}

void AES192_decrypt(const AES192_ctx* ctx, size_t blocks, unsigned char* plain16, const unsigned char* cipher16) {
    while (blocks--) {
        AES_decrypt(ctx->rk, 12, plain16, cipher16);
        cipher16 += 16;
        plain16 += 16;
    }
}

void AES256_init(AES256_ctx* ctx, const unsigned char* key32) {
    AES_setup(ctx->rk, key32, 8, 14);
}

void AES256_encrypt(const AES256_ctx* ctx, size_t blocks, unsigned char* cipher16, const unsigned char* plain16) {
    while (blocks--) {
        AES_encrypt(ctx->rk, 14, cipher16, plain16);
        cipher16 += 16;
        plain16 += 16;
    }
}

void AES256_decrypt(const AES256_ctx* ctx, size_t blocks, unsigned char* plain16, const unsigned char* cipher16) {
    while (blocks--) {
        AES_decrypt(ctx->rk, 14, plain16, cipher16);
        cipher16 += 16;
        plain16 += 16;
    }
}
