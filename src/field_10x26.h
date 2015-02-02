/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_FIELD_REPR_
#define _SECP256K1_FIELD_REPR_

#include <stdint.h>

typedef struct {
    /* X = sum(i=0..9, elem[i]*2^26) mod n */
    uint32_t n[10];
#ifdef VERIFY
    int magnitude;
    int normalized;
#endif
} secp256k1_fe_t;

/* Unpacks a constant into a overlapping multi-limbed FE element. */
#define SECP256K1_FE_CONST_INNER(d7, d6, d5, d4, d3, d2, d1, d0) { \
    (d0) & 0x3FFFFFFUL, \
    ((d0) >> 26) | ((d1) & 0xFFFFFUL) << 6, \
    ((d1) >> 20) | ((d2) & 0x3FFFUL) << 12, \
    ((d2) >> 14) | ((d3) & 0xFFUL) << 18, \
    ((d3) >> 8) | ((d4) & 0x3) << 24, \
    ((d4) >> 2) & 0x3FFFFFFUL, \
    ((d4) >> 28) | ((d5) & 0x3FFFFFUL) << 4, \
    ((d5) >> 22) | ((d6) & 0xFFFF) << 10, \
    ((d6) >> 16) | ((d7) & 0x3FF) << 16, \
    ((d7) >> 10) \
}

#ifdef VERIFY
#define SECP256K1_FE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {SECP256K1_FE_CONST_INNER((d7), (d6), (d5), (d4), (d3), (d2), (d1), (d0)), 1, 1}
#else
#define SECP256K1_FE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {SECP256K1_FE_CONST_INNER((d7), (d6), (d5), (d4), (d3), (d2), (d1), (d0))}
#endif

typedef struct {
    uint32_t n[8];
} secp256k1_fe_storage_t;

#define SECP256K1_FE_STORAGE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {{ (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }}

#endif
