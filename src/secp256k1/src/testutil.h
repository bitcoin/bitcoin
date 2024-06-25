/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_TESTUTIL_H
#define SECP256K1_TESTUTIL_H

#include "field.h"
#include "group.h"
#include "testrand.h"
#include "util.h"

static void testutil_random_fe(secp256k1_fe *x) {
    unsigned char bin[32];
    do {
        testrand256(bin);
        if (secp256k1_fe_set_b32_limit(x, bin)) {
            return;
        }
    } while(1);
}

static void testutil_random_fe_non_zero(secp256k1_fe *nz) {
    do {
        testutil_random_fe(nz);
    } while (secp256k1_fe_is_zero(nz));
}

static void testutil_random_fe_magnitude(secp256k1_fe *fe, int m) {
    secp256k1_fe zero;
    int n = testrand_int(m + 1);
    secp256k1_fe_normalize(fe);
    if (n == 0) {
        return;
    }
    secp256k1_fe_clear(&zero);
    secp256k1_fe_negate(&zero, &zero, 0);
    secp256k1_fe_mul_int_unchecked(&zero, n - 1);
    secp256k1_fe_add(fe, &zero);
#ifdef VERIFY
    CHECK(fe->magnitude == n);
#endif
}

static void testutil_random_fe_test(secp256k1_fe *x) {
    unsigned char bin[32];
    do {
        testrand256_test(bin);
        if (secp256k1_fe_set_b32_limit(x, bin)) {
            return;
        }
    } while(1);
}

static void testutil_random_fe_non_zero_test(secp256k1_fe *fe) {
    do {
        testutil_random_fe_test(fe);
    } while(secp256k1_fe_is_zero(fe));
}

static void testutil_random_ge_x_magnitude(secp256k1_ge *ge) {
    testutil_random_fe_magnitude(&ge->x, SECP256K1_GE_X_MAGNITUDE_MAX);
}

static void testutil_random_ge_y_magnitude(secp256k1_ge *ge) {
    testutil_random_fe_magnitude(&ge->y, SECP256K1_GE_Y_MAGNITUDE_MAX);
}

static void testutil_random_gej_x_magnitude(secp256k1_gej *gej) {
    testutil_random_fe_magnitude(&gej->x, SECP256K1_GEJ_X_MAGNITUDE_MAX);
}

static void testutil_random_gej_y_magnitude(secp256k1_gej *gej) {
    testutil_random_fe_magnitude(&gej->y, SECP256K1_GEJ_Y_MAGNITUDE_MAX);
}

static void testutil_random_gej_z_magnitude(secp256k1_gej *gej) {
    testutil_random_fe_magnitude(&gej->z, SECP256K1_GEJ_Z_MAGNITUDE_MAX);
}

static void testutil_random_ge_test(secp256k1_ge *ge) {
    secp256k1_fe fe;
    do {
        testutil_random_fe_test(&fe);
        if (secp256k1_ge_set_xo_var(ge, &fe, testrand_bits(1))) {
            secp256k1_fe_normalize(&ge->y);
            break;
        }
    } while(1);
    ge->infinity = 0;
}

static void testutil_random_ge_jacobian_test(secp256k1_gej *gej, const secp256k1_ge *ge) {
    secp256k1_fe z2, z3;
    testutil_random_fe_non_zero_test(&gej->z);
    secp256k1_fe_sqr(&z2, &gej->z);
    secp256k1_fe_mul(&z3, &z2, &gej->z);
    secp256k1_fe_mul(&gej->x, &ge->x, &z2);
    secp256k1_fe_mul(&gej->y, &ge->y, &z3);
    gej->infinity = ge->infinity;
}

static void testutil_random_gej_test(secp256k1_gej *gej) {
    secp256k1_ge ge;
    testutil_random_ge_test(&ge);
    testutil_random_ge_jacobian_test(gej, &ge);
}

static void testutil_random_scalar_order_test(secp256k1_scalar *num) {
    do {
        unsigned char b32[32];
        int overflow = 0;
        testrand256_test(b32);
        secp256k1_scalar_set_b32(num, b32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(num)) {
            continue;
        }
        break;
    } while(1);
}

static void testutil_random_scalar_order(secp256k1_scalar *num) {
    do {
        unsigned char b32[32];
        int overflow = 0;
        testrand256(b32);
        secp256k1_scalar_set_b32(num, b32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(num)) {
            continue;
        }
        break;
    } while(1);
}

static void testutil_random_scalar_order_b32(unsigned char *b32) {
    secp256k1_scalar num;
    testutil_random_scalar_order(&num);
    secp256k1_scalar_get_b32(b32, &num);
}

#endif /* SECP256K1_TESTUTIL_H */
