/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_ECDH_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_ECDH_TESTS_EXHAUSTIVE_H

#include "../../../include/secp256k1_ecdh.h"
#include "main_impl.h"

static void test_exhaustive_ecdh(const secp256k1_context *ctx, const secp256k1_ge *group) {
    int i, j;
    unsigned char seckeys[EXHAUSTIVE_TEST_ORDER - 1][32];
    secp256k1_pubkey pubkeys[EXHAUSTIVE_TEST_ORDER - 1];

    /* Construct key pairs (32-byte secret key, public key object) for the entire group. */
    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        secp256k1_scalar scalar;
        secp256k1_scalar_set_int(&scalar, i);
        secp256k1_scalar_get_b32(seckeys[i - 1], &scalar);
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i - 1], seckeys[i - 1]));
    }

    /* Loop over key combinations. */
    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        for (j = 1; j < EXHAUSTIVE_TEST_ORDER; j++) {
            unsigned char ecdh_result_ij[32];
            unsigned char ecdh_result_ji[32];

            /* Calculate ECDH(i*G, j) and ECDH(j*G, i) using API function and verify that the results match. */
            CHECK(secp256k1_ecdh(ctx, ecdh_result_ij, &pubkeys[i - 1], seckeys[j - 1], NULL, NULL));
            CHECK(secp256k1_ecdh(ctx, ecdh_result_ji, &pubkeys[j - 1], seckeys[i - 1], NULL, NULL));
            CHECK(secp256k1_memcmp_var(ecdh_result_ij, ecdh_result_ji, 32) == 0);

            /* Recalculate the expected ECDH result manually by invoking the default ECDH hash
             * function on the precomputed group element (group[i * j]) coordinates, and verify
             * that it matches the previously calculated public API results. */
            {
                secp256k1_ge ecdh_ge_expected = group[(i * j) % EXHAUSTIVE_TEST_ORDER];
                unsigned char ecdh_result_expected[32];
                unsigned char x[32];
                unsigned char y[32];

                secp256k1_fe_normalize_var(&ecdh_ge_expected.x);
                secp256k1_fe_normalize_var(&ecdh_ge_expected.y);
                secp256k1_fe_get_b32(x, &ecdh_ge_expected.x);
                secp256k1_fe_get_b32(y, &ecdh_ge_expected.y);
                CHECK(secp256k1_ecdh_hash_function_default(ecdh_result_expected, x, y, NULL));
                CHECK(secp256k1_memcmp_var(ecdh_result_ij, ecdh_result_expected, 32) == 0);
            }
        }
    }
}

#endif
