/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_ELLSWIFT_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_ELLSWIFT_TESTS_EXHAUSTIVE_H

#include "../../../include/secp256k1_ellswift.h"
#include "main_impl.h"

static void test_exhaustive_ellswift(const secp256k1_context *ctx, const secp256k1_ge *group) {
    int i;

    /* Note that SwiftEC/ElligatorSwift are inherently curve operations, not
     * group operations, and this test only checks the curve points which are in
     * a tiny subgroup. In that sense it can't be really seen as exhaustive as
     * it doesn't (and for computational reasons obviously cannot) test the
     * entire domain ellswift operates under. */
    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        secp256k1_scalar scalar_i;
        unsigned char sec32[32];
        unsigned char ell64[64];
        secp256k1_pubkey pub_decoded;
        secp256k1_ge ge_decoded;

        /* Construct ellswift pubkey from exhaustive loop scalar i. */
        secp256k1_scalar_set_int(&scalar_i, i);
        secp256k1_scalar_get_b32(sec32, &scalar_i);
        CHECK(secp256k1_ellswift_create(ctx, ell64, sec32, NULL));

        /* Decode ellswift pubkey and check that it matches the precomputed group element. */
        secp256k1_ellswift_decode(ctx, &pub_decoded, ell64);
        secp256k1_pubkey_load(ctx, &ge_decoded, &pub_decoded);
        CHECK(secp256k1_ge_eq_var(&ge_decoded, &group[i]));
    }
}

#endif
