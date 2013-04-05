#include <stdio.h>

#include "impl/num.h"
#include "impl/field.h"
#include "impl/group.h"
#include "impl/ecmult.h"
#include "impl/ecdsa.h"

int main() {
    secp256k1_num_start();
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
        secp256k1_num_set_rand(&r, order);
        secp256k1_num_set_rand(&s, order);
        secp256k1_num_set_rand(&m, order);
        secp256k1_ecdsa_sig_set_rs(&sig, &r, &s);
        secp256k1_gej_t pubkey; secp256k1_gej_set_xo(&pubkey, &x, 1);
        if (secp256k1_gej_is_valid(&pubkey)) {
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
    secp256k1_num_stop();
    return 0;
}
