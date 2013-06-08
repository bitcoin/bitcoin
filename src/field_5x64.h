// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_FIELD_REPR_
#define _SECP256K1_FIELD_REPR_

#include <stdint.h>

typedef struct {
    // X = sum(i=0..4, elem[i]*2^64) mod n
    uint64_t n[5];
#ifdef VERIFY
    int reduced; // n[4] == 0
    int normalized; // reduced and X < 2^256 - 0x100003D1
#endif
} secp256k1_fe_t;

#endif
