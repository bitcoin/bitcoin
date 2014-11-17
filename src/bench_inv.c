/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
#include <stdio.h>

#include "include/secp256k1.h"

#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "group_impl.h"
#include "scalar_impl.h"

int main(void) {
    static const unsigned char init[32] = {
        0x02, 0x03, 0x05, 0x07, 0x0b, 0x0d, 0x11, 0x13,
        0x17, 0x1d, 0x1f, 0x25, 0x29, 0x2b, 0x2f, 0x35,
        0x3b, 0x3d, 0x43, 0x47, 0x49, 0x4f, 0x53, 0x59,
        0x61, 0x65, 0x67, 0x6b, 0x6d, 0x71, 0x7f, 0x83
    };
    static const unsigned char fini[32] = {
        0xba, 0x28, 0x58, 0xd8, 0xaa, 0x11, 0xd6, 0xf2,
        0xfa, 0xce, 0x50, 0xb1, 0x67, 0x19, 0xb1, 0xa6,
        0xe0, 0xaa, 0x84, 0x53, 0xf6, 0x80, 0xfc, 0x23,
        0x88, 0x3c, 0xd6, 0x74, 0x9f, 0x27, 0x09, 0x03
    };
    secp256k1_ge_start();
    secp256k1_scalar_t base, x;
    secp256k1_scalar_set_b32(&base, init, NULL);
    secp256k1_scalar_set_b32(&x, init, NULL);
    for (int i=0; i<1000000; i++) {
        secp256k1_scalar_inverse(&x, &x);
        secp256k1_scalar_add(&x, &x, &base);
    }
    unsigned char res[32];
    secp256k1_scalar_get_b32(res, &x);
    CHECK(memcmp(res, fini, 32) == 0);
    return 0;
}
