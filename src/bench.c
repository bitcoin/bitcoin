// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>

#include "include/secp256k1.h"
#include "util_impl.h"

int main() {
    secp256k1_start(SECP256K1_START_VERIFY);

    int good = 0;
    unsigned char pubkey[33] = {0x02,0x1f,0x98,0xb7,0x3c,0xbd,0xd4,0x06,0xf3,0x49,0xa9,0x6c,0x2d,0xcb,0x7a,0xf7,0x01,0xe0,0xbd,0x07,0xdf,0xe9,0x17,0xae,0x0e,0x43,0x85,0x63,0xf0,0xff,0x7b,0xab,0x2f};
    for (int i=0; i<1000000; i++) {
        unsigned char msg[32];
        secp256k1_rand256(msg);
        unsigned char sig[72] = {0x30, 0x44, 0x02, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        secp256k1_rand256(sig + 4);
        secp256k1_rand256(sig + 38);
        good += secp256k1_ecdsa_verify(msg, 32, sig, 72, pubkey, 33);
    }
    printf("%i\n", good);

    secp256k1_stop();
    return 0;
}
