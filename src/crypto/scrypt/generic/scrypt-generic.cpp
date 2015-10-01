/*
 * Copyright 2009 Colin Percival, 2011 ArtForz, 2011 pooler, 2013 Balthazar
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

#include "scrypt.h"

#ifdef _MSC_VER
#define INLINE __inline
#else
#define INLINE inline
#endif

// Generic scrypt_core implementation

static INLINE void xor_salsa8(uint32_t B[16], const uint32_t Bx[16])
{
    uint32_t x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
    int8_t i;

    x00 = (B[0] ^= Bx[0]);
    x01 = (B[1] ^= Bx[1]);
    x02 = (B[2] ^= Bx[2]);
    x03 = (B[3] ^= Bx[3]);
    x04 = (B[4] ^= Bx[4]);
    x05 = (B[5] ^= Bx[5]);
    x06 = (B[6] ^= Bx[6]);
    x07 = (B[7] ^= Bx[7]);
    x08 = (B[8] ^= Bx[8]);
    x09 = (B[9] ^= Bx[9]);
    x10 = (B[10] ^= Bx[10]);
    x11 = (B[11] ^= Bx[11]);
    x12 = (B[12] ^= Bx[12]);
    x13 = (B[13] ^= Bx[13]);
    x14 = (B[14] ^= Bx[14]);
    x15 = (B[15] ^= Bx[15]);
    for (i = 0; i < 8; i += 2) {
#define R(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
        /* Operate on columns. */
        x04 ^= R(x00+x12, 7); x09 ^= R(x05+x01, 7);
        x14 ^= R(x10+x06, 7); x03 ^= R(x15+x11, 7);

        x08 ^= R(x04+x00, 9); x13 ^= R(x09+x05, 9);
        x02 ^= R(x14+x10, 9); x07 ^= R(x03+x15, 9);

        x12 ^= R(x08+x04,13); x01 ^= R(x13+x09,13);
        x06 ^= R(x02+x14,13); x11 ^= R(x07+x03,13);

        x00 ^= R(x12+x08,18); x05 ^= R(x01+x13,18);
        x10 ^= R(x06+x02,18); x15 ^= R(x11+x07,18);

        /* Operate on rows. */
        x01 ^= R(x00+x03, 7); x06 ^= R(x05+x04, 7);
        x11 ^= R(x10+x09, 7); x12 ^= R(x15+x14, 7);

        x02 ^= R(x01+x00, 9); x07 ^= R(x06+x05, 9);
        x08 ^= R(x11+x10, 9); x13 ^= R(x12+x15, 9);

        x03 ^= R(x02+x01,13); x04 ^= R(x07+x06,13);
        x09 ^= R(x08+x11,13); x14 ^= R(x13+x12,13);

        x00 ^= R(x03+x02,18); x05 ^= R(x04+x07,18);
        x10 ^= R(x09+x08,18); x15 ^= R(x14+x13,18);
#undef R
    }
    B[0] += x00;
    B[1] += x01;
    B[2] += x02;
    B[3] += x03;
    B[4] += x04;
    B[5] += x05;
    B[6] += x06;
    B[7] += x07;
    B[8] += x08;
    B[9] += x09;
    B[10] += x10;
    B[11] += x11;
    B[12] += x12;
    B[13] += x13;
    B[14] += x14;
    B[15] += x15;
}

/* cpu and memory intensive function to transform a 80 byte buffer into a 32 byte output
   scratchpad size needs to be at least 63 + (128 * r * p) + (256 * r + 64) + (128 * r * N) bytes
   r = 1, p = 1, N = 1024
 */
uint256 scrypt_blockhash(const uint8_t* input)
{
    uint8_t scratchpad[SCRYPT_BUFFER_SIZE];
    uint32_t X[32];
    uint256 result = 0;

    uint32_t *V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    PKCS5_PBKDF2_HMAC((const char*)input, 80, input, 80, 1, EVP_sha256(), 128, (unsigned char *)X);

    for (uint16_t i = 0; i < 1024; i++) {
        memcpy(&V[i * 32], X, 128);
        xor_salsa8(&X[0], &X[16]);
        xor_salsa8(&X[16], &X[0]);
    }
    for (uint16_t i = 0; i < 1024; i++) {
        uint16_t j = 32 * (X[16] & 1023);
        for (uint16_t k = 0; k < 32; k++)
            X[k] ^= V[j + k];
        xor_salsa8(&X[0], &X[16]);
        xor_salsa8(&X[16], &X[0]);
    }

    PKCS5_PBKDF2_HMAC((const char*)input, 80, (const unsigned char*)X, 128, 1, EVP_sha256(), 32, (unsigned char*)&result);

    return result;
}
