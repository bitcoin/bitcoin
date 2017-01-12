/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_FIELD_
#define _SECP256K1_FIELD_

/** Field element module.
 *
 *  Field elements can be represented in several ways, but code accessing
 *  it (and implementations) need to take certain properaties into account:
 *  - Each field element can be normalized or not.
 *  - Each field element has a magnitude, which represents how far away
 *    its representation is away from normalization. Normalized elements
 *    always have a magnitude of 1, but a magnitude of 1 doesn't imply
 *    normality.
 */

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#if defined(USE_FIELD_GMP)
#include "field_gmp.h"
#elif defined(USE_FIELD_10X26)
#include "field_10x26.h"
#elif defined(USE_FIELD_5X52)
#include "field_5x52.h"
#else
#error "Please select field implementation"
#endif

typedef struct {
#ifndef USE_NUM_NONE
    secp256k1_num_t p;
#endif
    secp256k1_fe_t order;
} secp256k1_fe_consts_t;

static const secp256k1_fe_consts_t *secp256k1_fe_consts = NULL;

/** Initialize field element precomputation data. */
static void secp256k1_fe_start(void);

/** Unload field element precomputation data. */
static void secp256k1_fe_stop(void);

/** Normalize a field element. */
static void secp256k1_fe_normalize(secp256k1_fe_t *r);

/** Normalize a field element, without constant-time guarantee. */
static void secp256k1_fe_normalize_var(secp256k1_fe_t *r);

/** Set a field element equal to a small integer. Resulting field element is normalized. */
static void secp256k1_fe_set_int(secp256k1_fe_t *r, int a);

/** Verify whether a field element is zero. Requires the input to be normalized. */
static int secp256k1_fe_is_zero(const secp256k1_fe_t *a);

/** Check the "oddness" of a field element. Requires the input to be normalized. */
static int secp256k1_fe_is_odd(const secp256k1_fe_t *a);

/** Compare two field elements. Requires both inputs to be normalized */
static int secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b);

/** Compare two field elements. Requires both inputs to be normalized */
static int secp256k1_fe_cmp_var(const secp256k1_fe_t *a, const secp256k1_fe_t *b);

/** Set a field element equal to 32-byte big endian value. If succesful, the resulting field element is normalized. */
static int secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a);

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a);

/** Set a field element equal to the additive inverse of another. Takes a maximum magnitude of the input
 *  as an argument. The magnitude of the output is one higher. */
static void secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *a, int m);

/** Multiplies the passed field element with a small integer constant. Multiplies the magnitude by that
 *  small integer. */
static void secp256k1_fe_mul_int(secp256k1_fe_t *r, int a);

/** Adds a field element to another. The result has the sum of the inputs' magnitudes as magnitude. */
static void secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Sets a field element to be the product of two others. Requires the inputs' magnitudes to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *a, const secp256k1_fe_t * SECP256K1_RESTRICT b);

/** Sets a field element to be the square of another. Requires the input's magnitude to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Sets a field element to be the (modular) square root (if any exist) of another. Requires the
 *  input's magnitude to be at most 8. The output magnitude is 1 (but not guaranteed to be
 *  normalized). Return value indicates whether a square root was found. */
static int secp256k1_fe_sqrt_var(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Sets a field element to be the (modular) inverse of another. Requires the input's magnitude to be
 *  at most 8. The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_inv(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Potentially faster version of secp256k1_fe_inv, without constant-time guarantee. */
static void secp256k1_fe_inv_var(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Calculate the (modular) inverses of a batch of field elements. Requires the inputs' magnitudes to be
 *  at most 8. The output magnitudes are 1 (but not guaranteed to be normalized). The inputs and
 *  outputs must not overlap in memory. */
static void secp256k1_fe_inv_all_var(size_t len, secp256k1_fe_t r[len], const secp256k1_fe_t a[len]);

/** Convert a field element to a hexadecimal string. */
static void secp256k1_fe_get_hex(char *r, int *rlen, const secp256k1_fe_t *a);

/** Convert a hexadecimal string to a field element. */
static int secp256k1_fe_set_hex(secp256k1_fe_t *r, const char *a, int alen);

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time. */
static void secp256k1_fe_cmov(secp256k1_fe_t *r, const secp256k1_fe_t *a, int flag);

#endif
