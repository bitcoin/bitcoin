// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "util_impl.h"
#include "secp256k1.c"

#ifdef ENABLE_OPENSSL_TESTS
#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/obj_mac.h"
#endif

static int count = 100;

/***** NUM TESTS *****/

void random_num_negate(secp256k1_num_t *num) {
    if (secp256k1_rand32() & 1)
        secp256k1_num_negate(num);
}

void random_num_order_test(secp256k1_num_t *num) {
    do {
        unsigned char b32[32];
        secp256k1_rand256_test(b32);
        secp256k1_num_set_bin(num, b32, 32);
        if (secp256k1_num_is_zero(num))
            continue;
        if (secp256k1_num_cmp(num, &secp256k1_ge_consts->order) >= 0)
            continue;
        break;
    } while(1);
}

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

void test_num_copy_inc_cmp() {
    secp256k1_num_t n1,n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order(&n1);
    secp256k1_num_copy(&n2, &n1);
    CHECK(secp256k1_num_eq(&n1, &n2));
    CHECK(secp256k1_num_eq(&n2, &n1));
    secp256k1_num_inc(&n2);
    CHECK(!secp256k1_num_eq(&n1, &n2));
    CHECK(!secp256k1_num_eq(&n2, &n1));
    secp256k1_num_free(&n1);
    secp256k1_num_free(&n2);
}


void test_num_get_set_hex() {
    secp256k1_num_t n1,n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1);
    char c[64];
    secp256k1_num_get_hex(c, 64, &n1);
    secp256k1_num_set_hex(&n2, c, 64);
    CHECK(secp256k1_num_eq(&n1, &n2));
    for (int i=0; i<64; i++) {
        // check whether the lower 4 bits correspond to the last hex character
        int low1 = secp256k1_num_shift(&n1, 4);
        int lowh = c[63];
        int low2 = (lowh>>6)*9+(lowh-'0')&15;
        CHECK(low1 == low2);
        // shift bits off the hex representation, and compare
        memmove(c+1, c, 63);
        c[0] = '0';
        secp256k1_num_set_hex(&n2, c, 64);
        CHECK(secp256k1_num_eq(&n1, &n2));
    }
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void test_num_get_set_bin() {
    secp256k1_num_t n1,n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1);
    unsigned char c[32];
    secp256k1_num_get_bin(c, 32, &n1);
    secp256k1_num_set_bin(&n2, c, 32);
    CHECK(secp256k1_num_eq(&n1, &n2));
    for (int i=0; i<32; i++) {
        // check whether the lower 8 bits correspond to the last byte
        int low1 = secp256k1_num_shift(&n1, 8);
        int low2 = c[31];
        CHECK(low1 == low2);
        // shift bits off the byte representation, and compare
        memmove(c+1, c, 31);
        c[0] = 0;
        secp256k1_num_set_bin(&n2, c, 32);
        CHECK(secp256k1_num_eq(&n1, &n2));
    }
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void run_num_int() {
    secp256k1_num_t n1;
    secp256k1_num_init(&n1);
    for (int i=-255; i<256; i++) {
        unsigned char c1[3] = {};
        c1[2] = abs(i);
        unsigned char c2[3] = {0x11,0x22,0x33};
        secp256k1_num_set_int(&n1, i);
        secp256k1_num_get_bin(c2, 3, &n1);
        CHECK(memcmp(c1, c2, 3) == 0);
    }
    secp256k1_num_free(&n1);
}

void test_num_negate() {
    secp256k1_num_t n1;
    secp256k1_num_t n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1); // n1 = R
    random_num_negate(&n1);
    secp256k1_num_copy(&n2, &n1); // n2 = R
    secp256k1_num_sub(&n1, &n2, &n1); // n1 = n2-n1 = 0
    CHECK(secp256k1_num_is_zero(&n1));
    secp256k1_num_copy(&n1, &n2); // n1 = R
    secp256k1_num_negate(&n1); // n1 = -R
    CHECK(!secp256k1_num_is_zero(&n1));
    secp256k1_num_add(&n1, &n2, &n1); // n1 = n2+n1 = 0
    CHECK(secp256k1_num_is_zero(&n1));
    secp256k1_num_copy(&n1, &n2); // n1 = R
    secp256k1_num_negate(&n1); // n1 = -R
    CHECK(secp256k1_num_is_neg(&n1) != secp256k1_num_is_neg(&n2));
    secp256k1_num_negate(&n1); // n1 = R
    CHECK(secp256k1_num_eq(&n1, &n2));
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void test_num_add_sub() {
    int r = secp256k1_rand32();
    secp256k1_num_t n1;
    secp256k1_num_t n2;
    secp256k1_num_init(&n1);
    secp256k1_num_init(&n2);
    random_num_order_test(&n1); // n1 = R1
    if (r & 1) {
        random_num_negate(&n1);
    }
    random_num_order_test(&n2); // n2 = R2
    if (r & 2) {
        random_num_negate(&n2);
    }
    secp256k1_num_t n1p2, n2p1, n1m2, n2m1;
    secp256k1_num_init(&n1p2);
    secp256k1_num_init(&n2p1);
    secp256k1_num_init(&n1m2);
    secp256k1_num_init(&n2m1);
    secp256k1_num_add(&n1p2, &n1, &n2); // n1p2 = R1 + R2
    secp256k1_num_add(&n2p1, &n2, &n1); // n2p1 = R2 + R1
    secp256k1_num_sub(&n1m2, &n1, &n2); // n1m2 = R1 - R2
    secp256k1_num_sub(&n2m1, &n2, &n1); // n2m1 = R2 - R1
    CHECK(secp256k1_num_eq(&n1p2, &n2p1));
    CHECK(!secp256k1_num_eq(&n1p2, &n1m2));
    secp256k1_num_negate(&n2m1); // n2m1 = -R2 + R1
    CHECK(secp256k1_num_eq(&n2m1, &n1m2));
    CHECK(!secp256k1_num_eq(&n2m1, &n1));
    secp256k1_num_add(&n2m1, &n2m1, &n2); // n2m1 = -R2 + R1 + R2 = R1
    CHECK(secp256k1_num_eq(&n2m1, &n1));
    CHECK(!secp256k1_num_eq(&n2p1, &n1));
    secp256k1_num_sub(&n2p1, &n2p1, &n2); // n2p1 = R2 + R1 - R2 = R1
    CHECK(secp256k1_num_eq(&n2p1, &n1));
    secp256k1_num_free(&n2m1);
    secp256k1_num_free(&n1m2);
    secp256k1_num_free(&n2p1);
    secp256k1_num_free(&n1p2);
    secp256k1_num_free(&n2);
    secp256k1_num_free(&n1);
}

void run_num_smalltests() {
    for (int i=0; i<100*count; i++) {
        test_num_copy_inc_cmp();
        test_num_get_set_hex();
        test_num_get_set_bin();
        test_num_negate();
        test_num_add_sub();
    }
    run_num_int();
}

/***** FIELD TESTS *****/

void random_fe(secp256k1_fe_t *x) {
    unsigned char bin[32];
    secp256k1_rand256(bin);
    secp256k1_fe_set_b32(x, bin);
}

void random_fe_non_zero(secp256k1_fe_t *nz) {
    int tries = 10;
    while (--tries >= 0) {
        random_fe(nz);
        secp256k1_fe_normalize(nz);
        if (!secp256k1_fe_is_zero(nz))
            break;
    }
    // Infinitesimal probability of spurious failure here
    CHECK(tries >= 0);
}

void random_fe_non_square(secp256k1_fe_t *ns) {
    random_fe_non_zero(ns);
    secp256k1_fe_t r;
    if (secp256k1_fe_sqrt(&r, ns)) {
        secp256k1_fe_negate(ns, ns, 1);
    }
}

int check_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b) {
    secp256k1_fe_t an = *a; secp256k1_fe_normalize(&an);
    secp256k1_fe_t bn = *b; secp256k1_fe_normalize(&bn);
    return secp256k1_fe_equal(&an, &bn);
}

int check_fe_inverse(const secp256k1_fe_t *a, const secp256k1_fe_t *ai) {
    secp256k1_fe_t x; secp256k1_fe_mul(&x, a, ai);
    secp256k1_fe_t one; secp256k1_fe_set_int(&one, 1);
    return check_fe_equal(&x, &one);
}

void run_field_inv() {
    secp256k1_fe_t x, xi, xii;
    for (int i=0; i<10*count; i++) {
        random_fe_non_zero(&x);
        secp256k1_fe_inv(&xi, &x);
        CHECK(check_fe_inverse(&x, &xi));
        secp256k1_fe_inv(&xii, &xi);
        CHECK(check_fe_equal(&x, &xii));
    }
}

void run_field_inv_var() {
    secp256k1_fe_t x, xi, xii;
    for (int i=0; i<10*count; i++) {
        random_fe_non_zero(&x);
        secp256k1_fe_inv_var(&xi, &x);
        CHECK(check_fe_inverse(&x, &xi));
        secp256k1_fe_inv_var(&xii, &xi);
        CHECK(check_fe_equal(&x, &xii));
    }
}

void run_field_inv_all() {
    secp256k1_fe_t x[16], xi[16], xii[16];
    // Check it's safe to call for 0 elements
    secp256k1_fe_inv_all(0, xi, x);
    for (int i=0; i<count; i++) {
        size_t len = (secp256k1_rand32() & 15) + 1;
        for (int j=0; j<len; j++)
            random_fe_non_zero(&x[j]);
        secp256k1_fe_inv_all(len, xi, x);
        for (int j=0; j<len; j++)
            CHECK(check_fe_inverse(&x[j], &xi[j]));
        secp256k1_fe_inv_all(len, xii, xi);
        for (int j=0; j<len; j++)
            CHECK(check_fe_equal(&x[j], &xii[j]));
    }
}

void run_field_inv_all_var() {
    secp256k1_fe_t x[16], xi[16], xii[16];
    // Check it's safe to call for 0 elements
    secp256k1_fe_inv_all_var(0, xi, x);
    for (int i=0; i<count; i++) {
        size_t len = (secp256k1_rand32() & 15) + 1;
        for (int j=0; j<len; j++)
            random_fe_non_zero(&x[j]);
        secp256k1_fe_inv_all_var(len, xi, x);
        for (int j=0; j<len; j++)
            CHECK(check_fe_inverse(&x[j], &xi[j]));
        secp256k1_fe_inv_all_var(len, xii, xi);
        for (int j=0; j<len; j++)
            CHECK(check_fe_equal(&x[j], &xii[j]));
    }
}

void run_sqr() {
    secp256k1_fe_t x, s;

    {
        secp256k1_fe_set_int(&x, 1);
        secp256k1_fe_negate(&x, &x, 1);

        for (int i=1; i<=512; ++i) {
            secp256k1_fe_mul_int(&x, 2);
            secp256k1_fe_normalize(&x);
            secp256k1_fe_sqr(&s, &x);
        }
    }
}

void test_sqrt(const secp256k1_fe_t *a, const secp256k1_fe_t *k) {
    secp256k1_fe_t r1, r2;
    int v = secp256k1_fe_sqrt(&r1, a);
    CHECK((v == 0) == (k == NULL));

    if (k != NULL) {
        // Check that the returned root is +/- the given known answer
        secp256k1_fe_negate(&r2, &r1, 1);
        secp256k1_fe_add(&r1, k); secp256k1_fe_add(&r2, k);
        secp256k1_fe_normalize(&r1); secp256k1_fe_normalize(&r2);
        CHECK(secp256k1_fe_is_zero(&r1) || secp256k1_fe_is_zero(&r2));
    }
}

void run_sqrt() {
    secp256k1_fe_t ns, x, s, t;

    // Check sqrt(0) is 0
    secp256k1_fe_set_int(&x, 0);
    secp256k1_fe_sqr(&s, &x);
    test_sqrt(&s, &x);

    // Check sqrt of small squares (and their negatives)
    for (int i=1; i<=100; i++) {
        secp256k1_fe_set_int(&x, i);
        secp256k1_fe_sqr(&s, &x);
        test_sqrt(&s, &x);
        secp256k1_fe_negate(&t, &s, 1);
        test_sqrt(&t, NULL);
    }

    // Consistency checks for large random values
    for (int i=0; i<10; i++) {
        random_fe_non_square(&ns);
        for (int j=0; j<count; j++) {
            random_fe(&x);
            secp256k1_fe_sqr(&s, &x);
            test_sqrt(&s, &x);
            secp256k1_fe_negate(&t, &s, 1);
            test_sqrt(&t, NULL);
            secp256k1_fe_mul(&t, &s, &ns);
            test_sqrt(&t, NULL);
        }
    }
}

/***** ECMULT TESTS *****/

void run_ecmult_chain() {
    // random starting point A (on the curve)
    secp256k1_fe_t ax; secp256k1_fe_set_hex(&ax, "8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004", 64);
    secp256k1_fe_t ay; secp256k1_fe_set_hex(&ay, "a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f", 64);
    secp256k1_gej_t a; secp256k1_gej_set_xy(&a, &ax, &ay);
    // two random initial factors xn and gn
    secp256k1_num_t xn;
    secp256k1_num_init(&xn);
    secp256k1_num_set_hex(&xn, "84cc5452f7fde1edb4d38a8ce9b1b84ccef31f146e569be9705d357a42985407", 64);
    secp256k1_num_t gn;
    secp256k1_num_init(&gn);
    secp256k1_num_set_hex(&gn, "a1e58d22553dcd42b23980625d4c57a96e9323d42b3152e5ca2c3990edc7c9de", 64);
    // two small multipliers to be applied to xn and gn in every iteration:
    secp256k1_num_t xf;
    secp256k1_num_init(&xf);
    secp256k1_num_set_hex(&xf, "1337", 4);
    secp256k1_num_t gf;
    secp256k1_num_init(&gf);
    secp256k1_num_set_hex(&gf, "7113", 4);
    // accumulators with the resulting coefficients to A and G
    secp256k1_num_t ae;
    secp256k1_num_init(&ae);
    secp256k1_num_set_int(&ae, 1);
    secp256k1_num_t ge;
    secp256k1_num_init(&ge);
    secp256k1_num_set_int(&ge, 0);
    // the point being computed
    secp256k1_gej_t x = a;
    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    for (int i=0; i<200*count; i++) {
        // in each iteration, compute X = xn*X + gn*G;
        secp256k1_ecmult(&x, &x, &xn, &gn);
        // also compute ae and ge: the actual accumulated factors for A and G
        // if X was (ae*A+ge*G), xn*X + gn*G results in (xn*ae*A + (xn*ge+gn)*G)
        secp256k1_num_mod_mul(&ae, &ae, &xn, order);
        secp256k1_num_mod_mul(&ge, &ge, &xn, order);
        secp256k1_num_add(&ge, &ge, &gn);
        secp256k1_num_mod(&ge, order);
        // modify xn and gn
        secp256k1_num_mod_mul(&xn, &xn, &xf, order);
        secp256k1_num_mod_mul(&gn, &gn, &gf, order);

        // verify
        if (i == 19999) {
            char res[132]; int resl = 132;
            secp256k1_gej_get_hex(res, &resl, &x);
            CHECK(strcmp(res, "(D6E96687F9B10D092A6F35439D86CEBEA4535D0D409F53586440BD74B933E830,B95CBCA2C77DA786539BE8FD53354D2D3B4F566AE658045407ED6015EE1B2A88)") == 0);
        }
    }
    // redo the computation, but directly with the resulting ae and ge coefficients:
    secp256k1_gej_t x2; secp256k1_ecmult(&x2, &a, &ae, &ge);
    char res[132]; int resl = 132;
    char res2[132]; int resl2 = 132;
    secp256k1_gej_get_hex(res, &resl, &x);
    secp256k1_gej_get_hex(res2, &resl2, &x2);
    CHECK(strcmp(res, res2) == 0);
    CHECK(strlen(res) == 131);
    secp256k1_num_free(&xn);
    secp256k1_num_free(&gn);
    secp256k1_num_free(&xf);
    secp256k1_num_free(&gf);
    secp256k1_num_free(&ae);
    secp256k1_num_free(&ge);
}

void test_point_times_order(const secp256k1_gej_t *point) {
    // multiplying a point by the order results in O
    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    secp256k1_num_t zero;
    secp256k1_num_init(&zero);
    secp256k1_num_set_int(&zero, 0);
    secp256k1_gej_t res;
    secp256k1_ecmult(&res, point, order, order); // calc res = order * point + order * G;
    CHECK(secp256k1_gej_is_infinity(&res));
    secp256k1_num_free(&zero);
}

void run_point_times_order() {
    secp256k1_fe_t x; secp256k1_fe_set_hex(&x, "02", 2);
    for (int i=0; i<500; i++) {
        secp256k1_ge_t p;
        if (secp256k1_ge_set_xo(&p, &x, 1)) {
            CHECK(secp256k1_ge_is_valid(&p));
            secp256k1_gej_t j;
            secp256k1_gej_set_ge(&j, &p);
            CHECK(secp256k1_gej_is_valid(&j));
            test_point_times_order(&j);
        }
        secp256k1_fe_sqr(&x, &x);
    }
    char c[65]; int cl=65;
    secp256k1_fe_get_hex(c, &cl, &x);
    CHECK(strcmp(c, "7603CB59B0EF6C63FE6084792A0C378CDB3233A80F8A9A09A877DEAD31B38C45") == 0);
}

void test_wnaf(const secp256k1_num_t *number, int w) {
    secp256k1_num_t x, two, t;
    secp256k1_num_init(&x);
    secp256k1_num_init(&two);
    secp256k1_num_init(&t);
    secp256k1_num_set_int(&x, 0);
    secp256k1_num_set_int(&two, 2);
    int wnaf[257];
    int bits = secp256k1_ecmult_wnaf(wnaf, number, w);
    int zeroes = -1;
    for (int i=bits-1; i>=0; i--) {
        secp256k1_num_mul(&x, &x, &two);
        int v = wnaf[i];
        if (v) {
            CHECK(zeroes == -1 || zeroes >= w-1); // check that distance between non-zero elements is at least w-1
            zeroes=0;
            CHECK((v & 1) == 1); // check non-zero elements are odd
            CHECK(v <= (1 << (w-1)) - 1); // check range below
            CHECK(v >= -(1 << (w-1)) - 1); // check range above
        } else {
            CHECK(zeroes != -1); // check that no unnecessary zero padding exists
            zeroes++;
        }
        secp256k1_num_set_int(&t, v);
        secp256k1_num_add(&x, &x, &t);
    }
    CHECK(secp256k1_num_eq(&x, number)); // check that wnaf represents number
    secp256k1_num_free(&x);
    secp256k1_num_free(&two);
    secp256k1_num_free(&t);
}

void run_wnaf() {
    secp256k1_num_t n;
    secp256k1_num_init(&n);
    for (int i=0; i<count; i++) {
        random_num_order(&n);
        if (i % 1)
            secp256k1_num_negate(&n);
        test_wnaf(&n, 4+(i%10));
    }
    secp256k1_num_free(&n);
}

void random_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *key, const secp256k1_num_t *msg, int *recid) {
    secp256k1_num_t nonce;
    secp256k1_num_init(&nonce);
    do {
        random_num_order_test(&nonce);
    } while(!secp256k1_ecdsa_sig_sign(sig, key, msg, &nonce, recid));
    secp256k1_num_free(&nonce);
}

void test_ecdsa_sign_verify() {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;
    secp256k1_num_t msg, key;
    secp256k1_num_init(&msg);
    random_num_order_test(&msg);
    secp256k1_num_init(&key);
    random_num_order_test(&key);
    secp256k1_gej_t pubj; secp256k1_ecmult_gen(&pubj, &key);
    secp256k1_ge_t pub; secp256k1_ge_set_gej(&pub, &pubj);
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    random_sign(&sig, &key, &msg, NULL);
    CHECK(secp256k1_ecdsa_sig_verify(&sig, &pub, &msg));
    secp256k1_num_inc(&msg);
    CHECK(!secp256k1_ecdsa_sig_verify(&sig, &pub, &msg));
    secp256k1_ecdsa_sig_free(&sig);
    secp256k1_num_free(&msg);
    secp256k1_num_free(&key);
}

void run_ecdsa_sign_verify() {
    for (int i=0; i<10*count; i++) {
        test_ecdsa_sign_verify();
    }
}

void test_ecdsa_end_to_end() {
    unsigned char privkey[32];
    unsigned char message[32];

    // Generate a random key and message.
    {
        secp256k1_num_t msg, key;
        secp256k1_num_init(&msg);
        random_num_order_test(&msg);
        secp256k1_num_init(&key);
        random_num_order_test(&key);
        secp256k1_num_get_bin(privkey, 32, &key);
        secp256k1_num_get_bin(message, 32, &msg);
        secp256k1_num_free(&msg);
        secp256k1_num_free(&key);
    }

    // Construct and verify corresponding public key.
    CHECK(secp256k1_ecdsa_seckey_verify(privkey) == 1);
    char pubkey[65]; int pubkeylen = 65;
    CHECK(secp256k1_ecdsa_pubkey_create(pubkey, &pubkeylen, privkey, secp256k1_rand32() % 2) == 1);
    CHECK(secp256k1_ecdsa_pubkey_verify(pubkey, pubkeylen));

    // Verify private key import and export.
    unsigned char seckey[300]; int seckeylen = 300;
    CHECK(secp256k1_ecdsa_privkey_export(privkey, seckey, &seckeylen, secp256k1_rand32() % 2) == 1);
    unsigned char privkey2[32];
    CHECK(secp256k1_ecdsa_privkey_import(privkey2, seckey, seckeylen) == 1);
    CHECK(memcmp(privkey, privkey2, 32) == 0);

    // Optionally tweak the keys using addition.
    if (secp256k1_rand32() % 3 == 0) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        int ret1 = secp256k1_ecdsa_privkey_tweak_add(privkey, rnd);
        int ret2 = secp256k1_ecdsa_pubkey_tweak_add(pubkey, pubkeylen, rnd);
        CHECK(ret1 == ret2);
        if (ret1 == 0) return;
        char pubkey2[65]; int pubkeylen2 = 65;
        CHECK(secp256k1_ecdsa_pubkey_create(pubkey2, &pubkeylen2, privkey, pubkeylen == 33) == 1);
        CHECK(memcmp(pubkey, pubkey2, pubkeylen) == 0);
    }

    // Optionally tweak the keys using multiplication.
    if (secp256k1_rand32() % 3 == 0) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        int ret1 = secp256k1_ecdsa_privkey_tweak_mul(privkey, rnd);
        int ret2 = secp256k1_ecdsa_pubkey_tweak_mul(pubkey, pubkeylen, rnd);
        CHECK(ret1 == ret2);
        if (ret1 == 0) return;
        char pubkey2[65]; int pubkeylen2 = 65;
        CHECK(secp256k1_ecdsa_pubkey_create(pubkey2, &pubkeylen2, privkey, pubkeylen == 33) == 1);
        CHECK(memcmp(pubkey, pubkey2, pubkeylen) == 0);
    }

    // Sign.
    unsigned char signature[72]; unsigned int signaturelen = 72;
    while(1) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        if (secp256k1_ecdsa_sign(message, 32, signature, &signaturelen, privkey, rnd) == 1) {
            break;
        }
    }
    // Verify.
    CHECK(secp256k1_ecdsa_verify(message, 32, signature, signaturelen, pubkey, pubkeylen) == 1);
    // Destroy signature and verify again.
    signature[signaturelen - 1 - secp256k1_rand32() % 20] += 1 + (secp256k1_rand32() % 255);
    CHECK(secp256k1_ecdsa_verify(message, 32, signature, signaturelen, pubkey, pubkeylen) != 1);

    // Compact sign.
    unsigned char csignature[64]; unsigned int recid = 0;
    while(1) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        if (secp256k1_ecdsa_sign_compact(message, 32, csignature, privkey, rnd, &recid) == 1) {
            break;
        }
    }
    // Recover.
    unsigned char recpubkey[65]; unsigned recpubkeylen = 0;
    CHECK(secp256k1_ecdsa_recover_compact(message, 32, csignature, recpubkey, &recpubkeylen, pubkeylen == 33, recid) == 1);
    CHECK(recpubkeylen == pubkeylen);
    CHECK(memcmp(pubkey, recpubkey, pubkeylen) == 0);
    // Destroy signature and verify again.
    csignature[secp256k1_rand32() % 64] += 1 + (secp256k1_rand32() % 255);
    CHECK(secp256k1_ecdsa_recover_compact(message, 32, csignature, recpubkey, &recpubkeylen, pubkeylen == 33, recid) != 1 ||
          memcmp(pubkey, recpubkey, pubkeylen) != 0);
    CHECK(recpubkeylen == pubkeylen);
}

void run_ecdsa_end_to_end() {
    for (int i=0; i<64*count; i++) {
        test_ecdsa_end_to_end();
    }
}


#ifdef ENABLE_OPENSSL_TESTS
EC_KEY *get_openssl_key(const secp256k1_num_t *key) {
    unsigned char privkey[300];
    int privkeylen;
    int compr = secp256k1_rand32() & 1;
    const unsigned char* pbegin = privkey;
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    CHECK(secp256k1_ecdsa_privkey_serialize(privkey, &privkeylen, key, compr));
    CHECK(d2i_ECPrivateKey(&ec_key, &pbegin, privkeylen));
    CHECK(EC_KEY_check_key(ec_key));
    return ec_key;
}

void test_ecdsa_openssl() {
    const secp256k1_ge_consts_t *c = secp256k1_ge_consts;
    secp256k1_num_t key, msg;
    secp256k1_num_init(&msg);
    unsigned char message[32];
    secp256k1_rand256_test(message);
    secp256k1_num_set_bin(&msg, message, 32);
    secp256k1_num_init(&key);
    random_num_order_test(&key);
    secp256k1_gej_t qj;
    secp256k1_ecmult_gen(&qj, &key);
    secp256k1_ge_t q;
    secp256k1_ge_set_gej(&q, &qj);
    EC_KEY *ec_key = get_openssl_key(&key);
    CHECK(ec_key);
    unsigned char signature[80];
    int sigsize = 80;
    CHECK(ECDSA_sign(0, message, sizeof(message), signature, &sigsize, ec_key));
    secp256k1_ecdsa_sig_t sig;
    secp256k1_ecdsa_sig_init(&sig);
    CHECK(secp256k1_ecdsa_sig_parse(&sig, signature, sigsize));
    CHECK(secp256k1_ecdsa_sig_verify(&sig, &q, &msg));
    secp256k1_num_inc(&sig.r);
    CHECK(!secp256k1_ecdsa_sig_verify(&sig, &q, &msg));

    random_sign(&sig, &key, &msg, NULL);
    sigsize = 80;
    CHECK(secp256k1_ecdsa_sig_serialize(signature, &sigsize, &sig));
    CHECK(ECDSA_verify(0, message, sizeof(message), signature, sigsize, ec_key) == 1);

    secp256k1_ecdsa_sig_free(&sig);
    EC_KEY_free(ec_key);
    secp256k1_num_free(&key);
    secp256k1_num_free(&msg);
}

void run_ecdsa_openssl() {
    for (int i=0; i<10*count; i++) {
        test_ecdsa_openssl();
    }
}
#endif

int main(int argc, char **argv) {
    if (argc > 1)
        count = strtol(argv[1], NULL, 0)*47;

    printf("test count = %i\n", count);

    // initialize
    secp256k1_start();

    // num tests
    run_num_smalltests();

    // field tests
    run_field_inv();
    run_field_inv_var();
    run_field_inv_all();
    run_field_inv_all_var();
    run_sqr();
    run_sqrt();

    // ecmult tests
    run_wnaf();
    run_point_times_order();
    run_ecmult_chain();

    // ecdsa tests
    run_ecdsa_sign_verify();
    run_ecdsa_end_to_end();
#ifdef ENABLE_OPENSSL_TESTS
    run_ecdsa_openssl();
#endif

    // shutdown
    secp256k1_stop();
    return 0;
}
