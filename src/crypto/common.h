// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_COMMON_H
#define BITCOIN_CRYPTO_COMMON_H

#include <stdint.h>

#ifdef WIN32
uint32_t static inline ReadLE32(const unsigned char *ptr) { return *((uint32_t*)ptr); }
uint64_t static inline ReadLE64(const unsigned char *ptr) { return *((uint64_t*)ptr); }

void static inline WriteLE32(unsigned char *ptr, uint32_t x) { *((uint32_t*)ptr) = x; }
void static inline WriteLE64(unsigned char *ptr, uint64_t x) { *((uint64_t*)ptr) = x; }

uint32_t static inline ReadBE32(const unsigned char *ptr) {
    return ((uint32_t)ptr[0] << 24 | (uint32_t)ptr[1] << 16 | (uint32_t)ptr[2] << 8 | (uint32_t)ptr[3]);
}

uint64_t static inline ReadBE64(const unsigned char *ptr) {
    return ((uint64_t)ptr[0] << 56 | (uint64_t)ptr[1] << 48 | (uint64_t)ptr[2] << 40 | (uint64_t)ptr[3] << 32 |
            (uint64_t)ptr[4] << 24 | (uint64_t)ptr[5] << 16 | (uint64_t)ptr[6] << 8 | (uint64_t)ptr[7]);
}

void static inline WriteBE32(unsigned char *ptr, uint32_t x) {
    ptr[0] = x >> 24; ptr[1] = x >> 16; ptr[2] = x >> 8; ptr[3] = x;
}

void static inline WriteBE64(unsigned char *ptr, uint64_t x) {
    ptr[0] = x >> 56; ptr[1] = x >> 48; ptr[2] = x >> 40; ptr[3] = x >> 32;
    ptr[4] = x >> 24; ptr[5] = x >> 16; ptr[6] = x >> 8; ptr[7] = x;
}
#else
#  include <endian.h>
uint32_t static inline ReadLE32(const unsigned char *ptr) { return le32toh(*((uint32_t*)ptr)); }
uint64_t static inline ReadLE64(const unsigned char *ptr) { return le64toh(*((uint64_t*)ptr)); }
void static inline WriteLE32(unsigned char *ptr, uint32_t x) { *((uint32_t*)ptr) = htole32(x); }
void static inline WriteLE64(unsigned char *ptr, uint64_t x) { *((uint64_t*)ptr) = htole64(x); }

uint32_t static inline ReadBE32(const unsigned char *ptr) { return be32toh(*((uint32_t*)ptr)); }
uint64_t static inline ReadBE64(const unsigned char *ptr) { return be64toh(*((uint64_t*)ptr)); }
void static inline WriteBE32(unsigned char *ptr, uint32_t x) { *((uint32_t*)ptr) = htobe32(x); }
void static inline WriteBE64(unsigned char *ptr, uint64_t x) { *((uint64_t*)ptr) = htobe64(x); }
#endif
#endif
