/*-
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

#include <stdlib.h>
#include <stdint.h>
#include <xmmintrin.h>

#include "scrypt_mine.h"
#include "pbkdf2.h"

#include "util.h"
#include "net.h"

extern bool fShutdown;
extern bool fGenerateBitcoins;

extern CBlockIndex* pindexBest;
extern uint32_t nTransactionsUpdated;


#if defined(__x86_64__)

#define SCRYPT_3WAY
#define SCRYPT_BUFFER_SIZE (3 * 131072 + 63)

extern "C" int scrypt_best_throughput();
extern "C" void scrypt_core(uint32_t *X, uint32_t *V);
extern "C" void scrypt_core_2way(uint32_t *X, uint32_t *Y, uint32_t *V);
extern "C" void scrypt_core_3way(uint32_t *X, uint32_t *Y, uint32_t *Z, uint32_t *V);

#elif defined(__i386__)

#define SCRYPT_BUFFER_SIZE (131072 + 63)

extern  "C" void scrypt_core(uint32_t *X, uint32_t *V);

#endif

void *scrypt_buffer_alloc() {
    return malloc(SCRYPT_BUFFER_SIZE);
}

void scrypt_buffer_free(void *scratchpad)
{
    free(scratchpad);
}

/* cpu and memory intensive function to transform a 80 byte buffer into a 32 byte output
   scratchpad size needs to be at least 63 + (128 * r * p) + (256 * r + 64) + (128 * r * N) bytes
   r = 1, p = 1, N = 1024
 */

static void scrypt(const void* input, size_t inputlen, uint32_t *res, void *scratchpad)
{
    uint32_t *V;
    uint32_t X[32];
    V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    PBKDF2_SHA256((const uint8_t*)input, inputlen, (const uint8_t*)input, sizeof(block_header), 1, (uint8_t *)X, 128);

    scrypt_core(X, V);

    PBKDF2_SHA256((const uint8_t*)input, inputlen, (uint8_t *)X, 128, 1, (uint8_t*)res, 32);
}

void scrypt_hash(const void* input, size_t inputlen, uint32_t *res, void *scratchpad)
{
    return scrypt(input, inputlen, res, scratchpad);
}

#ifdef SCRYPT_3WAY
static void scrypt_2way(const void *input1, const void *input2, size_t input1len, size_t input2len, uint32_t *res1, uint32_t *res2, void *scratchpad)
{
    uint32_t *V;
    uint32_t X[32], Y[32];
    V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    PBKDF2_SHA256((const uint8_t*)input1, input1len, (const uint8_t*)input1, input1len, 1, (uint8_t *)X, 128);
    PBKDF2_SHA256((const uint8_t*)input2, input2len, (const uint8_t*)input2, input2len, 1, (uint8_t *)Y, 128);

    scrypt_core_2way(X, Y, V);

    PBKDF2_SHA256((const uint8_t*)input1, input1len, (uint8_t *)X, 128, 1, (uint8_t*)res1, 32);
    PBKDF2_SHA256((const uint8_t*)input2, input2len, (uint8_t *)Y, 128, 1, (uint8_t*)res2, 32);
}

static void scrypt_3way(const void *input1, const void *input2, const void *input3,
   size_t input1len, size_t input2len, size_t input3len, uint32_t *res1, uint32_t *res2, uint32_t *res3,
   void *scratchpad)
{
    uint32_t *V;
    uint32_t X[32], Y[32], Z[32];
    V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    PBKDF2_SHA256((const uint8_t*)input1, input1len, (const uint8_t*)input1, input1len, 1, (uint8_t *)X, 128);
    PBKDF2_SHA256((const uint8_t*)input2, input2len, (const uint8_t*)input2, input2len, 1, (uint8_t *)Y, 128);
    PBKDF2_SHA256((const uint8_t*)input3, input3len, (const uint8_t*)input3, input3len, 1, (uint8_t *)Z, 128);

    scrypt_core_3way(X, Y, Z, V);

    PBKDF2_SHA256((const uint8_t*)input1, input1len, (uint8_t *)X, 128, 1, (uint8_t*)res1, 32);
    PBKDF2_SHA256((const uint8_t*)input2, input2len, (uint8_t *)Y, 128, 1, (uint8_t*)res2, 32);
    PBKDF2_SHA256((const uint8_t*)input3, input3len, (uint8_t *)Z, 128, 1, (uint8_t*)res3, 32);
}
#endif

unsigned int scanhash_scrypt(block_header *pdata, void *scratchbuf,
    uint32_t max_nonce, uint32_t &hash_count,
    void *result, block_header *res_header)
{
    hash_count = 0;
    block_header data = *pdata;
    uint32_t hash[8];
    unsigned char *hashc = (unsigned char *) &hash;

#ifdef SCRYPT_3WAY
    block_header data2 = *pdata;
    uint32_t hash2[8];
    unsigned char *hashc2 = (unsigned char *) &hash2;

    block_header data3 = *pdata;
    uint32_t hash3[8];
    unsigned char *hashc3 = (unsigned char *) &hash3;

    int throughput = scrypt_best_throughput();
#endif

    uint32_t n = 0;

    while (true) {

        data.nonce = n++;

#ifdef SCRYPT_3WAY
        if (throughput >= 2 && n < max_nonce) {
            data2.nonce = n++;
            if(throughput >= 3)
            {
                data3.nonce = n++;
                scrypt_3way(&data, &data2, &data3, 80, 80, 80, hash, hash2, hash3, scratchbuf);
                hash_count += 3;

                if (hashc3[31] == 0 && hashc3[30] == 0) {
                    memcpy(result, hash3, 32);
                    *res_header = data3;

                    return data3.nonce;
                }
            }
            else
            {
                scrypt_2way(&data, &data2, 80, 80, hash, hash2, scratchbuf);
                hash_count += 2;
            }

            if (hashc2[31] == 0 && hashc2[30] == 0) {
                memcpy(result, hash2, 32);

                return data2.nonce;
            }
        } else {
            scrypt(&data, 80, hash, scratchbuf);
            hash_count += 1;
        }
#else
        scrypt(&data, 80, hash, scratchbuf);
        hash_count += 1;
#endif
        if (hashc[31] == 0 && hashc[30] == 0) {
            memcpy(result, hash, 32);

            return data.nonce;
        }

        if (n >= max_nonce) {
            hash_count = 0xffff + 1;
            break;
        }
    }

    return (unsigned int) -1;
}
