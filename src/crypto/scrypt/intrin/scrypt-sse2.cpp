/*
 * Copyright 2009 Colin Percival, 2011 ArtForz, 2012-2013 pooler
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

#include <emmintrin.h>
#include "scrypt.h"

static inline uint32_t le32dec(const void *pp)
{
    const uint8_t *p = (uint8_t const *)pp;
    return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
    ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}

static inline void le32enc(void *pp, uint32_t x)
{
    uint8_t *p = (uint8_t *)pp;
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

static inline void xor_salsa8_sse2(__m128i B[4], const __m128i Bx[4])
{
    __m128i X0 = B[0] = _mm_xor_si128(B[0], Bx[0]);
    __m128i X1 = B[1] = _mm_xor_si128(B[1], Bx[1]);
    __m128i X2 = B[2] = _mm_xor_si128(B[2], Bx[2]);
    __m128i X3 = B[3] = _mm_xor_si128(B[3], Bx[3]);

    for (uint32_t i = 0; i < 8; i += 2) {
        /* Operate on "columns". */
        __m128i T = _mm_add_epi32(X0, X3);
        X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 7));
        X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 25));
        T = _mm_add_epi32(X1, X0);
        X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
        X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
        T = _mm_add_epi32(X2, X1);
        X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 13));
        X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 19));
        T = _mm_add_epi32(X3, X2);
        X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
        X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

        /* Rearrange data. */
        X1 = _mm_shuffle_epi32(X1, 0x93);
        X2 = _mm_shuffle_epi32(X2, 0x4E);
        X3 = _mm_shuffle_epi32(X3, 0x39);

        /* Operate on "rows". */
        T = _mm_add_epi32(X0, X1);
        X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 7));
        X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 25));
        T = _mm_add_epi32(X3, X0);
        X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
        X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
        T = _mm_add_epi32(X2, X3);
        X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 13));
        X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 19));
        T = _mm_add_epi32(X1, X2);
        X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
        X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

        /* Rearrange data. */
        X1 = _mm_shuffle_epi32(X1, 0x39);
        X2 = _mm_shuffle_epi32(X2, 0x4E);
        X3 = _mm_shuffle_epi32(X3, 0x93);
    }

    B[0] = _mm_add_epi32(B[0], X0);
    B[1] = _mm_add_epi32(B[1], X1);
    B[2] = _mm_add_epi32(B[2], X2);
    B[3] = _mm_add_epi32(B[3], X3);
}

uint256 scrypt_blockhash(const uint8_t* input)
{
    uint8_t scratchpad[SCRYPT_BUFFER_SIZE];
    __m128i *V = (__m128i *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    uint8_t B[128];
    void *const tmp = const_cast<uint8_t*>(input);
    PKCS5_PBKDF2_HMAC(static_cast<const char*>(tmp), 80, input, 80, 1, EVP_sha256(), 128, B);

    union {
        __m128i i128[8];
        uint32_t u32[32];
    } X;
    uint32_t i, k;
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 16; i++) {
            X.u32[k * 16 + i] = le32dec(&B[(k * 16 + (i * 5 % 16)) * 4]);
        }
    }

    for (i = 0; i < 1024; i++) {
        for (k = 0; k < 8; k++)
            V[i * 8 + k] = X.i128[k];
        xor_salsa8_sse2(&X.i128[0], &X.i128[4]);
        xor_salsa8_sse2(&X.i128[4], &X.i128[0]);
    }
    for (i = 0; i < 1024; i++) {
        uint32_t j = 8 * (X.u32[16] & 1023);
        for (k = 0; k < 8; k++)
            X.i128[k] = _mm_xor_si128(X.i128[k], V[j + k]);
        xor_salsa8_sse2(&X.i128[0], &X.i128[4]);
        xor_salsa8_sse2(&X.i128[4], &X.i128[0]);
    }

    for (k = 0; k < 2; k++) {
        for (i = 0; i < 16; i++) {
            le32enc(&B[(k * 16 + (i * 5 % 16)) * 4], X.u32[k * 16 + i]);
        }
    }

    uint256 result = 0;
    PKCS5_PBKDF2_HMAC(static_cast<const char*>(tmp), 80, B, 128, 1, EVP_sha256(), 32, (unsigned char*)&result);

    return result;
}
