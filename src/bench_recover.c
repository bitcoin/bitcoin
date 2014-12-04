/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>

#include "include/secp256k1.h"
#include "util.h"

int main(void) {
    secp256k1_start(SECP256K1_START_VERIFY);

    unsigned char msg[32];
    unsigned char sig[64];

    for (int i = 0; i < 32; i++) msg[i] = 1 + i;
    for (int i = 0; i < 64; i++) sig[i] = 65 + i;

    unsigned char pubkey[33];
    for (int i=0; i<1000000; i++) {
        int pubkeylen = 33;
        CHECK(secp256k1_ecdsa_recover_compact(msg, 32, sig, pubkey, &pubkeylen, 1, i % 2));
        for (int j = 0; j < 32; j++) {
            sig[j + 32] = msg[j];    /* Move former message to S. */
            msg[j] = sig[j];         /* Move former R to message. */
            sig[j] = pubkey[j + 1];  /* Move recovered pubkey X coordinate to R (which must be a valid X coordinate). */
        }
    }

    static const unsigned char fini[33] = {
        0x02,
        0x52, 0x63, 0xae, 0x9a, 0x9d, 0x47, 0x1f, 0x1a,
        0xb2, 0x36, 0x65, 0x89, 0x11, 0xe7, 0xcc, 0x86,
        0xa3, 0xab, 0x97, 0xb6, 0xf1, 0xaf, 0xfd, 0x8f,
        0x9b, 0x38, 0xb6, 0x18, 0x55, 0xe5, 0xc2, 0x43
    };
    CHECK(memcmp(fini, pubkey, 33) == 0);

    secp256k1_stop();
    return 0;
}
