// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>

#include "impl/num.h"
#include "impl/field.h"
#include "impl/group.h"
#include "impl/ecmult.h"
#include "impl/ecdsa.h"
#include "impl/util.h"

void random_num_order(secp256k1_num_t *num) {
    do {
        unsigned char b32[32];
        secp256k1_rand256(b32);
        secp256k1_num_set_bin(num, b32, 32);
        if (secp256k1_num_is_zero(num))
            continue;
        if (secp256k1_num_cmp(num, &secp256k1_ge_consts->order) >= 0)
            continue;
        break;
    } while(1);
}

int main() {
    secp256k1_fe_start();
    secp256k1_ge_start();
    secp256k1_ecmult_start();

    secp256k1_fe_t x;
    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    secp256k1_num_t r, s, m;
    secp256k1_num_init(&r);
    secp256k1_num_init(&s);
    secp256k1_num_init(&m);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    secp256k1_fe_set_hex(&x, "a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f", 64);
    int cnt = 0;
    int good = 0;
    for (int i=0; i<1000000; i++) {
        random_num_order(&r);
        random_num_order(&s);
        random_num_order(&m);
        secp256k1_ecdsa_sig_set_rs(&sig, &r, &s);
        secp256k1_ge_t pubkey; secp256k1_ge_set_xo(&pubkey, &x, 1);
        if (secp256k1_ge_is_valid(&pubkey)) {
            cnt++;
            good += secp256k1_ecdsa_sig_verify(&sig, &pubkey, &m);
        }
     }
    printf("%i/%i\n", good, cnt);
    secp256k1_num_free(&r);
    secp256k1_num_free(&s);
    secp256k1_num_free(&m);
    secp256k1_ecdsa_sig_free(&sig);

    secp256k1_ecmult_stop();
    secp256k1_ge_stop();
    secp256k1_fe_stop();
    return 0;
}
