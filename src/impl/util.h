// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_UTIL_IMPL_H_
#define _SECP256K1_UTIL_IMPL_H_

#include <stdint.h>
#include <string.h>

#include "../util.h"

static inline uint32_t secp256k1_rand32(void) {
    static uint32_t Rz = 11, Rw = 11;
    Rz = 36969 * (Rz & 0xFFFF) + (Rz >> 16);
    Rw = 18000 * (Rw & 0xFFFF) + (Rw >> 16);
    return (Rw << 16) + (Rw >> 16) + Rz;
}

static void secp256k1_rand256(unsigned char *b32) {
    for (int i=0; i<8; i++) {
        uint32_t r = secp256k1_rand32();
        b32[i*4 + 0] = (r >>  0) & 0xFF;
        b32[i*4 + 1] = (r >>  8) & 0xFF;
        b32[i*4 + 2] = (r >> 16) & 0xFF;
        b32[i*4 + 3] = (r >> 24) & 0xFF;
    }
}

static void secp256k1_rand256_test(unsigned char *b32) {
    int bits=0;
    memset(b32, 0, 32);
    while (bits < 256) {
        uint32_t ent = secp256k1_rand32();
        int now = 1 + ((ent % 64)*((ent >> 6) % 32)+16)/31;
        uint32_t val = 1 & (ent >> 11);
        while (now > 0 && bits < 256) {
            b32[bits / 8] |= val << (bits % 8);
            now--;
            bits++;
        }
    }
}

#endif
