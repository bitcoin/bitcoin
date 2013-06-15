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

#if defined (__x86_64__) || defined (__i386__)
#define SCRYPT_BUFFER_SIZE (131072 + 63)
extern "C" void scrypt_core(uint32_t *X, uint32_t *V);
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
