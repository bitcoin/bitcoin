/***********************************************************************
 * Copyright (c) 2015 Andrew Poelstra                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_SCALAR_REPR_IMPL_H
#define SECP256K1_SCALAR_REPR_IMPL_H

#include "checkmem.h"
#include "scalar.h"
#include "util.h"

#include <string.h>

SECP256K1_INLINE static int secp256k1_scalar_is_even(const secp256k1_scalar *a) {
    secp256k1_scalar_verify(a);

    return !(*a & 1);
}

SECP256K1_INLINE static void secp256k1_scalar_clear(secp256k1_scalar *r) { *r = 0; }

SECP256K1_INLINE static void secp256k1_scalar_set_int(secp256k1_scalar *r, unsigned int v) {
    *r = v % EXHAUSTIVE_TEST_ORDER;

    secp256k1_scalar_verify(r);
}

SECP256K1_INLINE static unsigned int secp256k1_scalar_get_bits(const secp256k1_scalar *a, unsigned int offset, unsigned int count) {
    secp256k1_scalar_verify(a);

    if (offset < 32)
        return ((*a >> offset) & ((((uint32_t)1) << count) - 1));
    else
        return 0;
}

SECP256K1_INLINE static unsigned int secp256k1_scalar_get_bits_var(const secp256k1_scalar *a, unsigned int offset, unsigned int count) {
    secp256k1_scalar_verify(a);

    return secp256k1_scalar_get_bits(a, offset, count);
}

SECP256K1_INLINE static int secp256k1_scalar_check_overflow(const secp256k1_scalar *a) { return *a >= EXHAUSTIVE_TEST_ORDER; }

static int secp256k1_scalar_add(secp256k1_scalar *r, const secp256k1_scalar *a, const secp256k1_scalar *b) {
    secp256k1_scalar_verify(a);
    secp256k1_scalar_verify(b);

    *r = (*a + *b) % EXHAUSTIVE_TEST_ORDER;

    secp256k1_scalar_verify(r);
    return *r < *b;
}

static void secp256k1_scalar_cadd_bit(secp256k1_scalar *r, unsigned int bit, int flag) {
    secp256k1_scalar_verify(r);

    if (flag && bit < 32)
        *r += ((uint32_t)1 << bit);

    secp256k1_scalar_verify(r);
#ifdef VERIFY
    VERIFY_CHECK(bit < 32);
    /* Verify that adding (1 << bit) will not overflow any in-range scalar *r by overflowing the underlying uint32_t. */
    VERIFY_CHECK(((uint32_t)1 << bit) - 1 <= UINT32_MAX - EXHAUSTIVE_TEST_ORDER);
#endif
}

static void secp256k1_scalar_set_b32(secp256k1_scalar *r, const unsigned char *b32, int *overflow) {
    int i;
    int over = 0;
    *r = 0;
    for (i = 0; i < 32; i++) {
        *r = (*r * 0x100) + b32[i];
        if (*r >= EXHAUSTIVE_TEST_ORDER) {
            over = 1;
            *r %= EXHAUSTIVE_TEST_ORDER;
        }
    }
    if (overflow) *overflow = over;

    secp256k1_scalar_verify(r);
}

static void secp256k1_scalar_get_b32(unsigned char *bin, const secp256k1_scalar* a) {
    secp256k1_scalar_verify(a);

    memset(bin, 0, 32);
    bin[28] = *a >> 24; bin[29] = *a >> 16; bin[30] = *a >> 8; bin[31] = *a;
}

SECP256K1_INLINE static int secp256k1_scalar_is_zero(const secp256k1_scalar *a) {
    secp256k1_scalar_verify(a);

    return *a == 0;
}

static void secp256k1_scalar_negate(secp256k1_scalar *r, const secp256k1_scalar *a) {
    secp256k1_scalar_verify(a);

    if (*a == 0) {
        *r = 0;
    } else {
        *r = EXHAUSTIVE_TEST_ORDER - *a;
    }

    secp256k1_scalar_verify(r);
}

SECP256K1_INLINE static int secp256k1_scalar_is_one(const secp256k1_scalar *a) {
    secp256k1_scalar_verify(a);

    return *a == 1;
}

static int secp256k1_scalar_is_high(const secp256k1_scalar *a) {
    secp256k1_scalar_verify(a);

    return *a > EXHAUSTIVE_TEST_ORDER / 2;
}

static int secp256k1_scalar_cond_negate(secp256k1_scalar *r, int flag) {
    secp256k1_scalar_verify(r);

    if (flag) secp256k1_scalar_negate(r, r);

    secp256k1_scalar_verify(r);
    return flag ? -1 : 1;
}

static void secp256k1_scalar_mul(secp256k1_scalar *r, const secp256k1_scalar *a, const secp256k1_scalar *b) {
    secp256k1_scalar_verify(a);
    secp256k1_scalar_verify(b);

    *r = (*a * *b) % EXHAUSTIVE_TEST_ORDER;

    secp256k1_scalar_verify(r);
}

static int secp256k1_scalar_shr_int(secp256k1_scalar *r, int n) {
    int ret;
    secp256k1_scalar_verify(r);
    VERIFY_CHECK(n > 0);
    VERIFY_CHECK(n < 16);

    ret = *r & ((1 << n) - 1);
    *r >>= n;

    secp256k1_scalar_verify(r);
    return ret;
}

static void secp256k1_scalar_split_128(secp256k1_scalar *r1, secp256k1_scalar *r2, const secp256k1_scalar *a) {
    secp256k1_scalar_verify(a);

    *r1 = *a;
    *r2 = 0;

    secp256k1_scalar_verify(r1);
    secp256k1_scalar_verify(r2);
}

SECP256K1_INLINE static int secp256k1_scalar_eq(const secp256k1_scalar *a, const secp256k1_scalar *b) {
    secp256k1_scalar_verify(a);
    secp256k1_scalar_verify(b);

    return *a == *b;
}

static SECP256K1_INLINE void secp256k1_scalar_cmov(secp256k1_scalar *r, const secp256k1_scalar *a, int flag) {
    uint32_t mask0, mask1;
    volatile int vflag = flag;
    secp256k1_scalar_verify(a);
    SECP256K1_CHECKMEM_CHECK_VERIFY(r, sizeof(*r));

    mask0 = vflag + ~((uint32_t)0);
    mask1 = ~mask0;
    *r = (*r & mask0) | (*a & mask1);

    secp256k1_scalar_verify(r);
}

static void secp256k1_scalar_inverse(secp256k1_scalar *r, const secp256k1_scalar *x) {
    int i;
    *r = 0;
    secp256k1_scalar_verify(x);

    for (i = 0; i < EXHAUSTIVE_TEST_ORDER; i++)
        if ((i * *x) % EXHAUSTIVE_TEST_ORDER == 1)
            *r = i;

    secp256k1_scalar_verify(r);
    /* If this VERIFY_CHECK triggers we were given a noninvertible scalar (and thus
     * have a composite group order; fix it in exhaustive_tests.c). */
    VERIFY_CHECK(*r != 0);
}

static void secp256k1_scalar_inverse_var(secp256k1_scalar *r, const secp256k1_scalar *x) {
    secp256k1_scalar_verify(x);

    secp256k1_scalar_inverse(r, x);

    secp256k1_scalar_verify(r);
}

#endif /* SECP256K1_SCALAR_REPR_IMPL_H */
