// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_NUM_REPR_
#define _SECP256K1_NUM_REPR_

#include <openssl/bn.h>

typedef struct {
    BIGNUM bn;
} secp256k1_num_t;

#endif
