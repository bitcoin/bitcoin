/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "secp256k1.c"
#include "testrand_impl.h"

#ifdef ENABLE_OPENSSL_TESTS
#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/obj_mac.h"
#endif

static int count = 64;

/***** NUM TESTS *****/

void random_num_negate(secp256k1_num_t *num) {
    if (secp256k1_rand32() & 1)
        secp256k1_num_negate(num);
}

void random_field_element_test(secp256k1_fe_t *fe) {
    do {
        unsigned char b32[32];
        secp256k1_rand256_test(b32);
        secp256k1_num_t num;
        secp256k1_num_set_bin(&num, b32, 32);
        if (secp256k1_num_cmp(&num, &secp256k1_fe_consts->p) >= 0)
            continue;
        secp256k1_fe_set_b32(fe, b32);
        break;
    } while(1);
}

void random_field_element_magnitude(secp256k1_fe_t *fe) {
    secp256k1_fe_normalize(fe);
    int n = secp256k1_rand32() % 4;
    for (int i = 0; i < n; i++) {
        secp256k1_fe_negate(fe, fe, 1 + 2*i);
        secp256k1_fe_negate(fe, fe, 2 + 2*i);
    }
}

void random_group_element_test(secp256k1_ge_t *ge) {
    secp256k1_fe_t fe;
    do {
        random_field_element_test(&fe);
        if (secp256k1_ge_set_xo(ge, &fe, secp256k1_rand32() & 1))
            break;
    } while(1);
}

void random_group_element_jacobian_test(secp256k1_gej_t *gej, const secp256k1_ge_t *ge) {
    do {
        random_field_element_test(&gej->z);
        if (!secp256k1_fe_is_zero(&gej->z)) {
            break;
        }
    } while(1);
    secp256k1_fe_t z2; secp256k1_fe_sqr(&z2, &gej->z);
    secp256k1_fe_t z3; secp256k1_fe_mul(&z3, &z2, &gej->z);
    secp256k1_fe_mul(&gej->x, &ge->x, &z2);
    secp256k1_fe_mul(&gej->y, &ge->y, &z3);
    gej->infinity = ge->infinity;
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

void random_scalar_order_test(secp256k1_scalar_t *num) {
    do {
        unsigned char b32[32];
        secp256k1_rand256_test(b32);
        int overflow = 0;
        secp256k1_scalar_set_b32(num, b32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(num))
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

void test_num_copy_inc_cmp(void) {
    secp256k1_num_t n1,n2;
    random_num_order(&n1);
    secp256k1_num_copy(&n2, &n1);
    CHECK(secp256k1_num_eq(&n1, &n2));
    CHECK(secp256k1_num_eq(&n2, &n1));
    secp256k1_num_inc(&n2);
    CHECK(!secp256k1_num_eq(&n1, &n2));
    CHECK(!secp256k1_num_eq(&n2, &n1));
}


void test_num_get_set_hex(void) {
    secp256k1_num_t n1,n2;
    random_num_order_test(&n1);
    char c[64];
    secp256k1_num_get_hex(c, 64, &n1);
    secp256k1_num_set_hex(&n2, c, 64);
    CHECK(secp256k1_num_eq(&n1, &n2));
    for (int i=0; i<64; i++) {
        /* check whether the lower 4 bits correspond to the last hex character */
        int low1 = secp256k1_num_shift(&n1, 4);
        int lowh = c[63];
        int low2 = ((lowh>>6)*9+(lowh-'0'))&15;
        CHECK(low1 == low2);
        /* shift bits off the hex representation, and compare */
        memmove(c+1, c, 63);
        c[0] = '0';
        secp256k1_num_set_hex(&n2, c, 64);
        CHECK(secp256k1_num_eq(&n1, &n2));
    }
}

void test_num_get_set_bin(void) {
    secp256k1_num_t n1,n2;
    random_num_order_test(&n1);
    unsigned char c[32];
    secp256k1_num_get_bin(c, 32, &n1);
    secp256k1_num_set_bin(&n2, c, 32);
    CHECK(secp256k1_num_eq(&n1, &n2));
    for (int i=0; i<32; i++) {
        /* check whether the lower 8 bits correspond to the last byte */
        int low1 = secp256k1_num_shift(&n1, 8);
        int low2 = c[31];
        CHECK(low1 == low2);
        /* shift bits off the byte representation, and compare */
        memmove(c+1, c, 31);
        c[0] = 0;
        secp256k1_num_set_bin(&n2, c, 32);
        CHECK(secp256k1_num_eq(&n1, &n2));
    }
}

void run_num_int(void) {
    secp256k1_num_t n1;
    for (int i=-255; i<256; i++) {
        unsigned char c1[3] = {};
        c1[2] = abs(i);
        unsigned char c2[3] = {0x11,0x22,0x33};
        secp256k1_num_set_int(&n1, i);
        secp256k1_num_get_bin(c2, 3, &n1);
        CHECK(memcmp(c1, c2, 3) == 0);
    }
}

void test_num_negate(void) {
    secp256k1_num_t n1;
    secp256k1_num_t n2;
    random_num_order_test(&n1); /* n1 = R */
    random_num_negate(&n1);
    secp256k1_num_copy(&n2, &n1); /* n2 = R */
    secp256k1_num_sub(&n1, &n2, &n1); /* n1 = n2-n1 = 0 */
    CHECK(secp256k1_num_is_zero(&n1));
    secp256k1_num_copy(&n1, &n2); /* n1 = R */
    secp256k1_num_negate(&n1); /* n1 = -R */
    CHECK(!secp256k1_num_is_zero(&n1));
    secp256k1_num_add(&n1, &n2, &n1); /* n1 = n2+n1 = 0 */
    CHECK(secp256k1_num_is_zero(&n1));
    secp256k1_num_copy(&n1, &n2); /* n1 = R */
    secp256k1_num_negate(&n1); /* n1 = -R */
    CHECK(secp256k1_num_is_neg(&n1) != secp256k1_num_is_neg(&n2));
    secp256k1_num_negate(&n1); /* n1 = R */
    CHECK(secp256k1_num_eq(&n1, &n2));
}

void test_num_add_sub(void) {
    int r = secp256k1_rand32();
    secp256k1_num_t n1;
    secp256k1_num_t n2;
    random_num_order_test(&n1); /* n1 = R1 */
    if (r & 1) {
        random_num_negate(&n1);
    }
    random_num_order_test(&n2); /* n2 = R2 */
    if (r & 2) {
        random_num_negate(&n2);
    }
    secp256k1_num_t n1p2, n2p1, n1m2, n2m1;
    secp256k1_num_add(&n1p2, &n1, &n2); /* n1p2 = R1 + R2 */
    secp256k1_num_add(&n2p1, &n2, &n1); /* n2p1 = R2 + R1 */
    secp256k1_num_sub(&n1m2, &n1, &n2); /* n1m2 = R1 - R2 */
    secp256k1_num_sub(&n2m1, &n2, &n1); /* n2m1 = R2 - R1 */
    CHECK(secp256k1_num_eq(&n1p2, &n2p1));
    CHECK(!secp256k1_num_eq(&n1p2, &n1m2));
    secp256k1_num_negate(&n2m1); /* n2m1 = -R2 + R1 */
    CHECK(secp256k1_num_eq(&n2m1, &n1m2));
    CHECK(!secp256k1_num_eq(&n2m1, &n1));
    secp256k1_num_add(&n2m1, &n2m1, &n2); /* n2m1 = -R2 + R1 + R2 = R1 */
    CHECK(secp256k1_num_eq(&n2m1, &n1));
    CHECK(!secp256k1_num_eq(&n2p1, &n1));
    secp256k1_num_sub(&n2p1, &n2p1, &n2); /* n2p1 = R2 + R1 - R2 = R1 */
    CHECK(secp256k1_num_eq(&n2p1, &n1));
}

void run_num_smalltests(void) {
    for (int i=0; i<100*count; i++) {
        test_num_copy_inc_cmp();
        test_num_get_set_hex();
        test_num_get_set_bin();
        test_num_negate();
        test_num_add_sub();
    }
    run_num_int();
}

/***** SCALAR TESTS *****/

int secp256k1_scalar_eq(const secp256k1_scalar_t *s1, const secp256k1_scalar_t *s2) {
    secp256k1_scalar_t t;
    secp256k1_scalar_negate(&t, s2);
    secp256k1_scalar_add(&t, &t, s1);
    int ret = secp256k1_scalar_is_zero(&t);
    return ret;
}

void scalar_test(void) {
    unsigned char c[32];

    /* Set 's' to a random scalar, with value 'snum'. */
    secp256k1_rand256_test(c);
    secp256k1_scalar_t s;
    secp256k1_scalar_set_b32(&s, c, NULL);
    secp256k1_num_t snum;
    secp256k1_num_set_bin(&snum, c, 32);
    secp256k1_num_mod(&snum, &secp256k1_ge_consts->order);

    /* Set 's1' to a random scalar, with value 's1num'. */
    secp256k1_rand256_test(c);
    secp256k1_scalar_t s1;
    secp256k1_scalar_set_b32(&s1, c, NULL);
    secp256k1_num_t s1num;
    secp256k1_num_set_bin(&s1num, c, 32);
    secp256k1_num_mod(&s1num, &secp256k1_ge_consts->order);

    /* Set 's2' to a random scalar, with value 'snum2', and byte array representation 'c'. */
    secp256k1_rand256_test(c);
    secp256k1_scalar_t s2;
    int overflow = 0;
    secp256k1_scalar_set_b32(&s2, c, &overflow);
    secp256k1_num_t s2num;
    secp256k1_num_set_bin(&s2num, c, 32);
    secp256k1_num_mod(&s2num, &secp256k1_ge_consts->order);

    {
        /* Test that fetching groups of 4 bits from a scalar and recursing n(i)=16*n(i-1)+p(i) reconstructs it. */
        secp256k1_num_t n, t, m;
        secp256k1_num_set_int(&n, 0);
        secp256k1_num_set_int(&m, 16);
        for (int i = 0; i < 256; i += 4) {
            secp256k1_num_set_int(&t, secp256k1_scalar_get_bits(&s, 256 - 4 - i, 4));
            secp256k1_num_mul(&n, &n, &m);
            secp256k1_num_add(&n, &n, &t);
        }
        CHECK(secp256k1_num_eq(&n, &snum));
    }

    {
        /* Test that get_b32 returns the same as get_bin on the number. */
        unsigned char r1[32];
        secp256k1_scalar_get_b32(r1, &s2);
        unsigned char r2[32];
        secp256k1_num_get_bin(r2, 32, &s2num);
        CHECK(memcmp(r1, r2, 32) == 0);
        /* If no overflow occurred when assigning, it should also be equal to the original byte array. */
        CHECK((memcmp(r1, c, 32) == 0) == (overflow == 0));
    }

    {
        /* Test that adding the scalars together is equal to adding their numbers together modulo the order. */
        secp256k1_num_t rnum;
        secp256k1_num_add(&rnum, &snum, &s2num);
        secp256k1_num_mod(&rnum, &secp256k1_ge_consts->order);
        secp256k1_scalar_t r;
        secp256k1_scalar_add(&r, &s, &s2);
        secp256k1_num_t r2num;
        secp256k1_scalar_get_num(&r2num, &r);
        CHECK(secp256k1_num_eq(&rnum, &r2num));
    }

    {
        /* Test that multipying the scalars is equal to multiplying their numbers modulo the order. */
        secp256k1_num_t rnum;
        secp256k1_num_mul(&rnum, &snum, &s2num);
        secp256k1_num_mod(&rnum, &secp256k1_ge_consts->order);
        secp256k1_scalar_t r;
        secp256k1_scalar_mul(&r, &s, &s2);
        secp256k1_num_t r2num;
        secp256k1_scalar_get_num(&r2num, &r);
        CHECK(secp256k1_num_eq(&rnum, &r2num));
        /* The result can only be zero if at least one of the factors was zero. */
        CHECK(secp256k1_scalar_is_zero(&r) == (secp256k1_scalar_is_zero(&s) || secp256k1_scalar_is_zero(&s2)));
        /* The results can only be equal to one of the factors if that factor was zero, or the other factor was one. */
        CHECK(secp256k1_num_eq(&rnum, &snum) == (secp256k1_scalar_is_zero(&s) || secp256k1_scalar_is_one(&s2)));
        CHECK(secp256k1_num_eq(&rnum, &s2num) == (secp256k1_scalar_is_zero(&s2) || secp256k1_scalar_is_one(&s)));
    }

    {
        /* Check that comparison with zero matches comparison with zero on the number. */
        CHECK(secp256k1_num_is_zero(&snum) == secp256k1_scalar_is_zero(&s));
        /* Check that comparison with the half order is equal to testing for high scalar. */
        CHECK(secp256k1_scalar_is_high(&s) == (secp256k1_num_cmp(&snum, &secp256k1_ge_consts->half_order) > 0));
        secp256k1_scalar_t neg;
        secp256k1_scalar_negate(&neg, &s);
        secp256k1_num_t negnum;
        secp256k1_num_sub(&negnum, &secp256k1_ge_consts->order, &snum);
        secp256k1_num_mod(&negnum, &secp256k1_ge_consts->order);
        /* Check that comparison with the half order is equal to testing for high scalar after negation. */
        CHECK(secp256k1_scalar_is_high(&neg) == (secp256k1_num_cmp(&negnum, &secp256k1_ge_consts->half_order) > 0));
        /* Negating should change the high property, unless the value was already zero. */
        CHECK((secp256k1_scalar_is_high(&s) == secp256k1_scalar_is_high(&neg)) == secp256k1_scalar_is_zero(&s));
        secp256k1_num_t negnum2;
        secp256k1_scalar_get_num(&negnum2, &neg);
        /* Negating a scalar should be equal to (order - n) mod order on the number. */
        CHECK(secp256k1_num_eq(&negnum, &negnum2));
        secp256k1_scalar_add(&neg, &neg, &s);
        /* Adding a number to its negation should result in zero. */
        CHECK(secp256k1_scalar_is_zero(&neg));
        secp256k1_scalar_negate(&neg, &neg);
        /* Negating zero should still result in zero. */
        CHECK(secp256k1_scalar_is_zero(&neg));
    }

    {
        /* Test that scalar inverses are equal to the inverse of their number modulo the order. */
        if (!secp256k1_scalar_is_zero(&s)) {
            secp256k1_scalar_t inv;
            secp256k1_scalar_inverse(&inv, &s);
            secp256k1_num_t invnum;
            secp256k1_num_mod_inverse(&invnum, &snum, &secp256k1_ge_consts->order);
            secp256k1_num_t invnum2;
            secp256k1_scalar_get_num(&invnum2, &inv);
            CHECK(secp256k1_num_eq(&invnum, &invnum2));
            secp256k1_scalar_mul(&inv, &inv, &s);
            /* Multiplying a scalar with its inverse must result in one. */
            CHECK(secp256k1_scalar_is_one(&inv));
            secp256k1_scalar_inverse(&inv, &inv);
            /* Inverting one must result in one. */
            CHECK(secp256k1_scalar_is_one(&inv));
        }
    }

    {
        /* Test commutativity of add. */
        secp256k1_scalar_t r1, r2;
        secp256k1_scalar_add(&r1, &s1, &s2);
        secp256k1_scalar_add(&r2, &s2, &s1);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test commutativity of mul. */
        secp256k1_scalar_t r1, r2;
        secp256k1_scalar_mul(&r1, &s1, &s2);
        secp256k1_scalar_mul(&r2, &s2, &s1);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test associativity of add. */
        secp256k1_scalar_t r1, r2;
        secp256k1_scalar_add(&r1, &s1, &s2);
        secp256k1_scalar_add(&r1, &r1, &s);
        secp256k1_scalar_add(&r2, &s2, &s);
        secp256k1_scalar_add(&r2, &s1, &r2);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test associativity of mul. */
        secp256k1_scalar_t r1, r2;
        secp256k1_scalar_mul(&r1, &s1, &s2);
        secp256k1_scalar_mul(&r1, &r1, &s);
        secp256k1_scalar_mul(&r2, &s2, &s);
        secp256k1_scalar_mul(&r2, &s1, &r2);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test distributitivity of mul over add. */
        secp256k1_scalar_t r1, r2, t;
        secp256k1_scalar_add(&r1, &s1, &s2);
        secp256k1_scalar_mul(&r1, &r1, &s);
        secp256k1_scalar_mul(&r2, &s1, &s);
        secp256k1_scalar_mul(&t, &s2, &s);
        secp256k1_scalar_add(&r2, &r2, &t);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }

    {
        /* Test square. */
        secp256k1_scalar_t r1, r2;
        secp256k1_scalar_sqr(&r1, &s1);
        secp256k1_scalar_mul(&r2, &s1, &s1);
        CHECK(secp256k1_scalar_eq(&r1, &r2));
    }
}

void run_scalar_tests(void) {
    for (int i = 0; i < 128 * count; i++) {
        scalar_test();
    }
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
    /* Infinitesimal probability of spurious failure here */
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

void run_field_inv(void) {
    secp256k1_fe_t x, xi, xii;
    for (int i=0; i<10*count; i++) {
        random_fe_non_zero(&x);
        secp256k1_fe_inv(&xi, &x);
        CHECK(check_fe_inverse(&x, &xi));
        secp256k1_fe_inv(&xii, &xi);
        CHECK(check_fe_equal(&x, &xii));
    }
}

void run_field_inv_var(void) {
    secp256k1_fe_t x, xi, xii;
    for (int i=0; i<10*count; i++) {
        random_fe_non_zero(&x);
        secp256k1_fe_inv_var(&xi, &x);
        CHECK(check_fe_inverse(&x, &xi));
        secp256k1_fe_inv_var(&xii, &xi);
        CHECK(check_fe_equal(&x, &xii));
    }
}

void run_field_inv_all(void) {
    secp256k1_fe_t x[16], xi[16], xii[16];
    /* Check it's safe to call for 0 elements */
    secp256k1_fe_inv_all(0, xi, x);
    for (int i=0; i<count; i++) {
        size_t len = (secp256k1_rand32() & 15) + 1;
        for (size_t j=0; j<len; j++)
            random_fe_non_zero(&x[j]);
        secp256k1_fe_inv_all(len, xi, x);
        for (size_t j=0; j<len; j++)
            CHECK(check_fe_inverse(&x[j], &xi[j]));
        secp256k1_fe_inv_all(len, xii, xi);
        for (size_t j=0; j<len; j++)
            CHECK(check_fe_equal(&x[j], &xii[j]));
    }
}

void run_field_inv_all_var(void) {
    secp256k1_fe_t x[16], xi[16], xii[16];
    /* Check it's safe to call for 0 elements */
    secp256k1_fe_inv_all_var(0, xi, x);
    for (int i=0; i<count; i++) {
        size_t len = (secp256k1_rand32() & 15) + 1;
        for (size_t j=0; j<len; j++)
            random_fe_non_zero(&x[j]);
        secp256k1_fe_inv_all_var(len, xi, x);
        for (size_t j=0; j<len; j++)
            CHECK(check_fe_inverse(&x[j], &xi[j]));
        secp256k1_fe_inv_all_var(len, xii, xi);
        for (size_t j=0; j<len; j++)
            CHECK(check_fe_equal(&x[j], &xii[j]));
    }
}

void run_sqr(void) {
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
        /* Check that the returned root is +/- the given known answer */
        secp256k1_fe_negate(&r2, &r1, 1);
        secp256k1_fe_add(&r1, k); secp256k1_fe_add(&r2, k);
        secp256k1_fe_normalize(&r1); secp256k1_fe_normalize(&r2);
        CHECK(secp256k1_fe_is_zero(&r1) || secp256k1_fe_is_zero(&r2));
    }
}

void run_sqrt(void) {
    secp256k1_fe_t ns, x, s, t;

    /* Check sqrt(0) is 0 */
    secp256k1_fe_set_int(&x, 0);
    secp256k1_fe_sqr(&s, &x);
    test_sqrt(&s, &x);

    /* Check sqrt of small squares (and their negatives) */
    for (int i=1; i<=100; i++) {
        secp256k1_fe_set_int(&x, i);
        secp256k1_fe_sqr(&s, &x);
        test_sqrt(&s, &x);
        secp256k1_fe_negate(&t, &s, 1);
        test_sqrt(&t, NULL);
    }

    /* Consistency checks for large random values */
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

/***** GROUP TESTS *****/

int ge_equals_ge(const secp256k1_ge_t *a, const secp256k1_ge_t *b) {
    if (a->infinity && b->infinity)
        return 1;
    return check_fe_equal(&a->x, &b->x) && check_fe_equal(&a->y, &b->y);
}

void ge_equals_gej(const secp256k1_ge_t *a, const secp256k1_gej_t *b) {
    secp256k1_ge_t bb;
    secp256k1_gej_t bj = *b;
    secp256k1_ge_set_gej_var(&bb, &bj);
    CHECK(ge_equals_ge(a, &bb));
}

void gej_equals_gej(const secp256k1_gej_t *a, const secp256k1_gej_t *b) {
    secp256k1_ge_t aa, bb;
    secp256k1_gej_t aj = *a, bj = *b;
    secp256k1_ge_set_gej_var(&aa, &aj);
    secp256k1_ge_set_gej_var(&bb, &bj);
    CHECK(ge_equals_ge(&aa, &bb));
}

void test_ge(void) {
    secp256k1_ge_t a, b, i, n;
    random_group_element_test(&a);
    random_group_element_test(&b);
    n = a;
    secp256k1_fe_normalize(&a.y);
    secp256k1_fe_negate(&n.y, &a.y, 1);
    secp256k1_ge_set_infinity(&i);
    random_field_element_magnitude(&a.x);
    random_field_element_magnitude(&a.y);
    random_field_element_magnitude(&b.x);
    random_field_element_magnitude(&b.y);
    random_field_element_magnitude(&n.x);
    random_field_element_magnitude(&n.y);

    secp256k1_gej_t aj, bj, ij, nj;
    random_group_element_jacobian_test(&aj, &a);
    random_group_element_jacobian_test(&bj, &b);
    secp256k1_gej_set_infinity(&ij);
    random_group_element_jacobian_test(&nj, &n);
    random_field_element_magnitude(&aj.x);
    random_field_element_magnitude(&aj.y);
    random_field_element_magnitude(&aj.z);
    random_field_element_magnitude(&bj.x);
    random_field_element_magnitude(&bj.y);
    random_field_element_magnitude(&bj.z);
    random_field_element_magnitude(&nj.x);
    random_field_element_magnitude(&nj.y);
    random_field_element_magnitude(&nj.z);

    /* gej + gej adds */
    secp256k1_gej_t aaj; secp256k1_gej_add_var(&aaj, &aj, &aj);
    secp256k1_gej_t abj; secp256k1_gej_add_var(&abj, &aj, &bj);
    secp256k1_gej_t aij; secp256k1_gej_add_var(&aij, &aj, &ij);
    secp256k1_gej_t anj; secp256k1_gej_add_var(&anj, &aj, &nj);
    secp256k1_gej_t iaj; secp256k1_gej_add_var(&iaj, &ij, &aj);
    secp256k1_gej_t iij; secp256k1_gej_add_var(&iij, &ij, &ij);

    /* gej + ge adds */
    secp256k1_gej_t aa; secp256k1_gej_add_ge_var(&aa, &aj, &a);
    secp256k1_gej_t ab; secp256k1_gej_add_ge_var(&ab, &aj, &b);
    secp256k1_gej_t ai; secp256k1_gej_add_ge_var(&ai, &aj, &i);
    secp256k1_gej_t an; secp256k1_gej_add_ge_var(&an, &aj, &n);
    secp256k1_gej_t ia; secp256k1_gej_add_ge_var(&ia, &ij, &a);
    secp256k1_gej_t ii; secp256k1_gej_add_ge_var(&ii, &ij, &i);

    /* const gej + ge adds */
    secp256k1_gej_t aac; secp256k1_gej_add_ge(&aac, &aj, &a);
    secp256k1_gej_t abc; secp256k1_gej_add_ge(&abc, &aj, &b);
    secp256k1_gej_t anc; secp256k1_gej_add_ge(&anc, &aj, &n);
    secp256k1_gej_t iac; secp256k1_gej_add_ge(&iac, &ij, &a);

    CHECK(secp256k1_gej_is_infinity(&an));
    CHECK(secp256k1_gej_is_infinity(&anj));
    CHECK(secp256k1_gej_is_infinity(&anc));
    gej_equals_gej(&aa, &aaj);
    gej_equals_gej(&aa, &aac);
    gej_equals_gej(&ab, &abj);
    gej_equals_gej(&ab, &abc);
    gej_equals_gej(&an, &anj);
    gej_equals_gej(&an, &anc);
    gej_equals_gej(&ia, &iaj);
    gej_equals_gej(&ai, &aij);
    gej_equals_gej(&ii, &iij);
    ge_equals_gej(&a, &ai);
    ge_equals_gej(&a, &ai);
    ge_equals_gej(&a, &iaj);
    ge_equals_gej(&a, &iaj);
    ge_equals_gej(&a, &iac);
}

void run_ge(void) {
    for (int i = 0; i < 2000*count; i++) {
        test_ge();
    }
}

/***** ECMULT TESTS *****/

void run_ecmult_chain(void) {
    /* random starting point A (on the curve) */
    secp256k1_fe_t ax; secp256k1_fe_set_hex(&ax, "8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004", 64);
    secp256k1_fe_t ay; secp256k1_fe_set_hex(&ay, "a357ae915c4a65281309edf20504740f0eb3343990216b4f81063cb65f2f7e0f", 64);
    secp256k1_gej_t a; secp256k1_gej_set_xy(&a, &ax, &ay);
    /* two random initial factors xn and gn */
    secp256k1_num_t xn;
    secp256k1_num_set_hex(&xn, "84cc5452f7fde1edb4d38a8ce9b1b84ccef31f146e569be9705d357a42985407", 64);
    secp256k1_num_t gn;
    secp256k1_num_set_hex(&gn, "a1e58d22553dcd42b23980625d4c57a96e9323d42b3152e5ca2c3990edc7c9de", 64);
    /* two small multipliers to be applied to xn and gn in every iteration: */
    secp256k1_num_t xf;
    secp256k1_num_set_hex(&xf, "1337", 4);
    secp256k1_num_t gf;
    secp256k1_num_set_hex(&gf, "7113", 4);
    /* accumulators with the resulting coefficients to A and G */
    secp256k1_num_t ae;
    secp256k1_num_set_int(&ae, 1);
    secp256k1_num_t ge;
    secp256k1_num_set_int(&ge, 0);
    /* the point being computed */
    secp256k1_gej_t x = a;
    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    for (int i=0; i<200*count; i++) {
        /* in each iteration, compute X = xn*X + gn*G; */
        secp256k1_ecmult(&x, &x, &xn, &gn);
        /* also compute ae and ge: the actual accumulated factors for A and G */
        /* if X was (ae*A+ge*G), xn*X + gn*G results in (xn*ae*A + (xn*ge+gn)*G) */
        secp256k1_num_mod_mul(&ae, &ae, &xn, order);
        secp256k1_num_mod_mul(&ge, &ge, &xn, order);
        secp256k1_num_add(&ge, &ge, &gn);
        secp256k1_num_mod(&ge, order);
        /* modify xn and gn */
        secp256k1_num_mod_mul(&xn, &xn, &xf, order);
        secp256k1_num_mod_mul(&gn, &gn, &gf, order);

        /* verify */
        if (i == 19999) {
            char res[132]; int resl = 132;
            secp256k1_gej_get_hex(res, &resl, &x);
            CHECK(strcmp(res, "(D6E96687F9B10D092A6F35439D86CEBEA4535D0D409F53586440BD74B933E830,B95CBCA2C77DA786539BE8FD53354D2D3B4F566AE658045407ED6015EE1B2A88)") == 0);
        }
    }
    /* redo the computation, but directly with the resulting ae and ge coefficients: */
    secp256k1_gej_t x2; secp256k1_ecmult(&x2, &a, &ae, &ge);
    char res[132]; int resl = 132;
    char res2[132]; int resl2 = 132;
    secp256k1_gej_get_hex(res, &resl, &x);
    secp256k1_gej_get_hex(res2, &resl2, &x2);
    CHECK(strcmp(res, res2) == 0);
    CHECK(strlen(res) == 131);
}

void test_point_times_order(const secp256k1_gej_t *point) {
    /* multiplying a point by the order results in O */
    const secp256k1_num_t *order = &secp256k1_ge_consts->order;
    secp256k1_num_t zero;
    secp256k1_num_set_int(&zero, 0);
    secp256k1_gej_t res;
    secp256k1_ecmult(&res, point, order, order); /* calc res = order * point + order * G; */
    CHECK(secp256k1_gej_is_infinity(&res));
}

void run_point_times_order(void) {
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
    secp256k1_num_set_int(&x, 0);
    secp256k1_num_set_int(&two, 2);
    int wnaf[257];
    int bits = secp256k1_ecmult_wnaf(wnaf, number, w);
    int zeroes = -1;
    for (int i=bits-1; i>=0; i--) {
        secp256k1_num_mul(&x, &x, &two);
        int v = wnaf[i];
        if (v) {
            CHECK(zeroes == -1 || zeroes >= w-1); /* check that distance between non-zero elements is at least w-1 */
            zeroes=0;
            CHECK((v & 1) == 1); /* check non-zero elements are odd */
            CHECK(v <= (1 << (w-1)) - 1); /* check range below */
            CHECK(v >= -(1 << (w-1)) - 1); /* check range above */
        } else {
            CHECK(zeroes != -1); /* check that no unnecessary zero padding exists */
            zeroes++;
        }
        secp256k1_num_set_int(&t, v);
        secp256k1_num_add(&x, &x, &t);
    }
    CHECK(secp256k1_num_eq(&x, number)); /* check that wnaf represents number */
}

void run_wnaf(void) {
    secp256k1_num_t n;
    for (int i=0; i<count; i++) {
        random_num_order(&n);
        if (i % 1)
            secp256k1_num_negate(&n);
        test_wnaf(&n, 4+(i%10));
    }
}

void random_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_scalar_t *key, const secp256k1_scalar_t *msg, int *recid) {
    secp256k1_scalar_t nonce;
    do {
        random_scalar_order_test(&nonce);
    } while(!secp256k1_ecdsa_sig_sign(sig, key, msg, &nonce, recid));
}

void test_ecdsa_sign_verify(void) {
    secp256k1_scalar_t msg, key;
    random_scalar_order_test(&msg);
    random_scalar_order_test(&key);
    secp256k1_gej_t pubj; secp256k1_ecmult_gen(&pubj, &key);
    secp256k1_ge_t pub; secp256k1_ge_set_gej(&pub, &pubj);
    secp256k1_ecdsa_sig_t sig;
    random_sign(&sig, &key, &msg, NULL);
    secp256k1_num_t msg_num;
    secp256k1_scalar_get_num(&msg_num, &msg);
    CHECK(secp256k1_ecdsa_sig_verify(&sig, &pub, &msg_num));
    secp256k1_num_inc(&msg_num);
    CHECK(!secp256k1_ecdsa_sig_verify(&sig, &pub, &msg_num));
}

void run_ecdsa_sign_verify(void) {
    for (int i=0; i<10*count; i++) {
        test_ecdsa_sign_verify();
    }
}

void test_ecdsa_end_to_end(void) {
    unsigned char privkey[32];
    unsigned char message[32];

    /* Generate a random key and message. */
    {
        secp256k1_num_t msg, key;
        random_num_order_test(&msg);
        random_num_order_test(&key);
        secp256k1_num_get_bin(privkey, 32, &key);
        secp256k1_num_get_bin(message, 32, &msg);
    }

    /* Construct and verify corresponding public key. */
    CHECK(secp256k1_ec_seckey_verify(privkey) == 1);
    unsigned char pubkey[65]; int pubkeylen = 65;
    CHECK(secp256k1_ec_pubkey_create(pubkey, &pubkeylen, privkey, secp256k1_rand32() % 2) == 1);
    CHECK(secp256k1_ec_pubkey_verify(pubkey, pubkeylen));

    /* Verify private key import and export. */
    unsigned char seckey[300]; int seckeylen = 300;
    CHECK(secp256k1_ec_privkey_export(privkey, seckey, &seckeylen, secp256k1_rand32() % 2) == 1);
    unsigned char privkey2[32];
    CHECK(secp256k1_ec_privkey_import(privkey2, seckey, seckeylen) == 1);
    CHECK(memcmp(privkey, privkey2, 32) == 0);

    /* Optionally tweak the keys using addition. */
    if (secp256k1_rand32() % 3 == 0) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        int ret1 = secp256k1_ec_privkey_tweak_add(privkey, rnd);
        int ret2 = secp256k1_ec_pubkey_tweak_add(pubkey, pubkeylen, rnd);
        CHECK(ret1 == ret2);
        if (ret1 == 0) return;
        unsigned char pubkey2[65]; int pubkeylen2 = 65;
        CHECK(secp256k1_ec_pubkey_create(pubkey2, &pubkeylen2, privkey, pubkeylen == 33) == 1);
        CHECK(memcmp(pubkey, pubkey2, pubkeylen) == 0);
    }

    /* Optionally tweak the keys using multiplication. */
    if (secp256k1_rand32() % 3 == 0) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        int ret1 = secp256k1_ec_privkey_tweak_mul(privkey, rnd);
        int ret2 = secp256k1_ec_pubkey_tweak_mul(pubkey, pubkeylen, rnd);
        CHECK(ret1 == ret2);
        if (ret1 == 0) return;
        unsigned char pubkey2[65]; int pubkeylen2 = 65;
        CHECK(secp256k1_ec_pubkey_create(pubkey2, &pubkeylen2, privkey, pubkeylen == 33) == 1);
        CHECK(memcmp(pubkey, pubkey2, pubkeylen) == 0);
    }

    /* Sign. */
    unsigned char signature[72]; int signaturelen = 72;
    while(1) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        if (secp256k1_ecdsa_sign(message, 32, signature, &signaturelen, privkey, rnd) == 1) {
            break;
        }
    }
    /* Verify. */
    CHECK(secp256k1_ecdsa_verify(message, 32, signature, signaturelen, pubkey, pubkeylen) == 1);
    /* Destroy signature and verify again. */
    signature[signaturelen - 1 - secp256k1_rand32() % 20] += 1 + (secp256k1_rand32() % 255);
    CHECK(secp256k1_ecdsa_verify(message, 32, signature, signaturelen, pubkey, pubkeylen) != 1);

    /* Compact sign. */
    unsigned char csignature[64]; int recid = 0;
    while(1) {
        unsigned char rnd[32];
        secp256k1_rand256_test(rnd);
        if (secp256k1_ecdsa_sign_compact(message, 32, csignature, privkey, rnd, &recid) == 1) {
            break;
        }
    }
    /* Recover. */
    unsigned char recpubkey[65]; int recpubkeylen = 0;
    CHECK(secp256k1_ecdsa_recover_compact(message, 32, csignature, recpubkey, &recpubkeylen, pubkeylen == 33, recid) == 1);
    CHECK(recpubkeylen == pubkeylen);
    CHECK(memcmp(pubkey, recpubkey, pubkeylen) == 0);
    /* Destroy signature and verify again. */
    csignature[secp256k1_rand32() % 64] += 1 + (secp256k1_rand32() % 255);
    CHECK(secp256k1_ecdsa_recover_compact(message, 32, csignature, recpubkey, &recpubkeylen, pubkeylen == 33, recid) != 1 ||
          memcmp(pubkey, recpubkey, pubkeylen) != 0);
    CHECK(recpubkeylen == pubkeylen);

}

void run_ecdsa_end_to_end(void) {
    for (int i=0; i<64*count; i++) {
        test_ecdsa_end_to_end();
    }
}

void test_ecdsa_infinity(void) {
    const unsigned char msg32[32] = {
        'T', 'h', 'i', 's', ' ', 'i', 's', ' ',
        'a', ' ', 'v', 'e', 'r', 'y', ' ', 's',
        'e', 'c', 'r', 'e', 't', ' ', 'm', 'e',
        's', 's', 'a', 'g', 'e', '.', '.', '.'
    };
    const unsigned char sig64[64] = {
        // Generated by signing the above message with nonce 'This is the nonce we will use...'
        // and secret key 0 (which is not valid), resulting in recid 0.
        0x67, 0xCB, 0x28, 0x5F, 0x9C, 0xD1, 0x94, 0xE8,
        0x40, 0xD6, 0x29, 0x39, 0x7A, 0xF5, 0x56, 0x96,
        0x62, 0xFD, 0xE4, 0x46, 0x49, 0x99, 0x59, 0x63,
        0x17, 0x9A, 0x7D, 0xD1, 0x7B, 0xD2, 0x35, 0x32,
        0x4B, 0x1B, 0x7D, 0xF3, 0x4C, 0xE1, 0xF6, 0x8E,
        0x69, 0x4F, 0xF6, 0xF1, 0x1A, 0xC7, 0x51, 0xDD,
        0x7D, 0xD7, 0x3E, 0x38, 0x7E, 0xE4, 0xFC, 0x86,
        0x6E, 0x1B, 0xE8, 0xEC, 0xC7, 0xDD, 0x95, 0x57
    };
    unsigned char pubkey[65];
    int pubkeylen = 65;
    CHECK(!secp256k1_ecdsa_recover_compact(msg32, 32, sig64, pubkey, &pubkeylen, 0, 0));
    CHECK(secp256k1_ecdsa_recover_compact(msg32, 32, sig64, pubkey, &pubkeylen, 0, 1));
    CHECK(!secp256k1_ecdsa_recover_compact(msg32, 32, sig64, pubkey, &pubkeylen, 0, 2));
    CHECK(!secp256k1_ecdsa_recover_compact(msg32, 32, sig64, pubkey, &pubkeylen, 0, 3));
}

void run_ecdsa_infinity(void) {
    test_ecdsa_infinity();
}

#ifdef ENABLE_OPENSSL_TESTS
EC_KEY *get_openssl_key(const secp256k1_scalar_t *key) {
    unsigned char privkey[300];
    int privkeylen;
    int compr = secp256k1_rand32() & 1;
    const unsigned char* pbegin = privkey;
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    CHECK(secp256k1_eckey_privkey_serialize(privkey, &privkeylen, key, compr));
    CHECK(d2i_ECPrivateKey(&ec_key, &pbegin, privkeylen));
    CHECK(EC_KEY_check_key(ec_key));
    return ec_key;
}

void test_ecdsa_openssl(void) {
    secp256k1_scalar_t key, msg;
    unsigned char message[32];
    secp256k1_rand256_test(message);
    secp256k1_scalar_set_b32(&msg, message, NULL);
    random_scalar_order_test(&key);
    secp256k1_gej_t qj;
    secp256k1_ecmult_gen(&qj, &key);
    secp256k1_ge_t q;
    secp256k1_ge_set_gej(&q, &qj);
    EC_KEY *ec_key = get_openssl_key(&key);
    CHECK(ec_key);
    unsigned char signature[80];
    unsigned int sigsize = 80;
    CHECK(ECDSA_sign(0, message, sizeof(message), signature, &sigsize, ec_key));
    secp256k1_ecdsa_sig_t sig;
    CHECK(secp256k1_ecdsa_sig_parse(&sig, signature, sigsize));
    secp256k1_num_t msg_num;
    secp256k1_scalar_get_num(&msg_num, &msg);
    CHECK(secp256k1_ecdsa_sig_verify(&sig, &q, &msg_num));
    secp256k1_num_inc(&sig.r);
    CHECK(!secp256k1_ecdsa_sig_verify(&sig, &q, &msg_num));

    random_sign(&sig, &key, &msg, NULL);
    int secp_sigsize = 80;
    CHECK(secp256k1_ecdsa_sig_serialize(signature, &secp_sigsize, &sig));
    CHECK(ECDSA_verify(0, message, sizeof(message), signature, secp_sigsize, ec_key) == 1);

    EC_KEY_free(ec_key);
}

void run_ecdsa_openssl(void) {
    for (int i=0; i<10*count; i++) {
        test_ecdsa_openssl();
    }
}
#endif

int main(int argc, char **argv) {
    /* find iteration count */
    if (argc > 1) {
        count = strtol(argv[1], NULL, 0);
    }

    /* find random seed */
    uint64_t seed;
    if (argc > 2) {
        seed = strtoull(argv[2], NULL, 0);
    } else {
        FILE *frand = fopen("/dev/urandom", "r");
        if (!frand || !fread(&seed, sizeof(seed), 1, frand)) {
            seed = time(NULL) * 1337;
        }
        fclose(frand);
    }
    secp256k1_rand_seed(seed);

    printf("test count = %i\n", count);
    printf("random seed = %llu\n", (unsigned long long)seed);

    /* initialize */
    secp256k1_start(SECP256K1_START_SIGN | SECP256K1_START_VERIFY);

    /* num tests */
    run_num_smalltests();

    /* scalar tests */
    run_scalar_tests();

    /* field tests */
    run_field_inv();
    run_field_inv_var();
    run_field_inv_all();
    run_field_inv_all_var();
    run_sqr();
    run_sqrt();

    /* group tests */
    run_ge();

    /* ecmult tests */
    run_wnaf();
    run_point_times_order();
    run_ecmult_chain();

    /* ecdsa tests */
    run_ecdsa_sign_verify();
    run_ecdsa_end_to_end();
    run_ecdsa_infinity();
#ifdef ENABLE_OPENSSL_TESTS
    run_ecdsa_openssl();
#endif

    printf("random run = %llu\n", (unsigned long long)secp256k1_rand32() + ((unsigned long long)secp256k1_rand32() << 32));

    /* shutdown */
    secp256k1_stop();
    return 0;
}
