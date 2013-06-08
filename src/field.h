// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

#if defined(USE_FIELD_GMP)
#include "field_gmp.h"
#elif defined(USE_FIELD_10X26)
#include "field_10x26.h"
#elif defined(USE_FIELD_5X52)
#include "field_5x52.h"
#elif defined(USE_FIELD_5X64)
#include "field_5x64.h"
#else
#error "Please select field implementation"
#endif

typedef struct {
    secp256k1_num_t p;
} secp256k1_fe_consts_t;

static const secp256k1_fe_consts_t *secp256k1_fe_consts = NULL;

/** Initialize field element precomputation data. */
void static secp256k1_fe_start(void);

/** Unload field element precomputation data. */
void static secp256k1_fe_stop(void);

/** Normalize a field element. */
void static secp256k1_fe_normalize(secp256k1_fe_t *r);

/** Set a field element equal to a small integer. Resulting field element is normalized. */
void static secp256k1_fe_set_int(secp256k1_fe_t *r, int a);

/** Verify whether a field element is zero. Requires the input to be normalized. */
int  static secp256k1_fe_is_zero(const secp256k1_fe_t *a);

/** Check the "oddness" of a field element. Requires the input to be normalized. */
int  static secp256k1_fe_is_odd(const secp256k1_fe_t *a);

/** Compare two field elements. Requires both inputs to be normalized */
int  static secp256k1_fe_equal(const secp256k1_fe_t *a, const secp256k1_fe_t *b);

/** Set a field element equal to 32-byte big endian value. Resulting field element is normalized. */
void static secp256k1_fe_set_b32(secp256k1_fe_t *r, const unsigned char *a);

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
void static secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe_t *a);

/** Set a field element equal to the additive inverse of another. Takes a maximum magnitude of the input
 *  as an argument. The magnitude of the output is one higher. */
void static secp256k1_fe_negate(secp256k1_fe_t *r, const secp256k1_fe_t *a, int m);

/** Multiplies the passed field element with a small integer constant. Multiplies the magnitude by that
 *  small integer. */
void static secp256k1_fe_mul_int(secp256k1_fe_t *r, int a);

/** Adds a field element to another. The result has the sum of the inputs' magnitudes as magnitude. */
void static secp256k1_fe_add(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Sets a field element to be the product of two others. Requires the inputs' magnitudes to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
void static secp256k1_fe_mul(secp256k1_fe_t *r, const secp256k1_fe_t *a, const secp256k1_fe_t *b);

/** Sets a field element to be the square of another. Requires the input's magnitude to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
void static secp256k1_fe_sqr(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Sets a field element to be the (modular) square root of another. Requires the inputs' magnitude to
 *  be at most 8. The output magnitude is 1 (but not guaranteed to be normalized). */
void static secp256k1_fe_sqrt(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Sets a field element to be the (modular) inverse of another. Requires the input's magnitude to be
 *  at most 8. The output magnitude is 1 (but not guaranteed to be normalized). */
void static secp256k1_fe_inv(secp256k1_fe_t *r, const secp256k1_fe_t *a);

/** Potentially faster version of secp256k1_fe_inv, without constant-time guarantee. */
void static secp256k1_fe_inv_var(secp256k1_fe_t *r, const secp256k1_fe_t *a);


/** Convert a field element to a hexadecimal string. */
void static secp256k1_fe_get_hex(char *r, int *rlen, const secp256k1_fe_t *a);

/** Convert a hexadecimal string to a field element. */
void static secp256k1_fe_set_hex(secp256k1_fe_t *r, const char *a, int alen);

#endif
