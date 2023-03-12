/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FIELD_H
#define SECP256K1_FIELD_H

/** Field element module.
 *
 *  Field elements can be represented in several ways, but code accessing
 *  it (and implementations) need to take certain properties into account:
 *  - Each field element can be normalized or not.
 *  - Each field element has a magnitude, which represents how far away
 *    its representation is away from normalization. Normalized elements
 *    always have a magnitude of 0 or 1, but a magnitude of 1 doesn't
 *    imply normality.
 */

#include "util.h"

#if defined(SECP256K1_WIDEMUL_INT128)
#include "field_5x52.h"
#elif defined(SECP256K1_WIDEMUL_INT64)
#include "field_10x26.h"
#else
#error "Please select wide multiplication implementation"
#endif

static const secp256k1_fe secp256k1_fe_one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
static const secp256k1_fe secp256k1_const_beta = SECP256K1_FE_CONST(
    0x7ae96a2bul, 0x657c0710ul, 0x6e64479eul, 0xac3434e9ul,
    0x9cf04975ul, 0x12f58995ul, 0xc1396c28ul, 0x719501eeul
);

/** Normalize a field element. This brings the field element to a canonical representation, reduces
 *  its magnitude to 1, and reduces it modulo field size `p`.
 */
static void secp256k1_fe_normalize(secp256k1_fe *r);

/** Weakly normalize a field element: reduce its magnitude to 1, but don't fully normalize. */
static void secp256k1_fe_normalize_weak(secp256k1_fe *r);

/** Normalize a field element, without constant-time guarantee. */
static void secp256k1_fe_normalize_var(secp256k1_fe *r);

/** Verify whether a field element represents zero i.e. would normalize to a zero value. */
static int secp256k1_fe_normalizes_to_zero(const secp256k1_fe *r);

/** Verify whether a field element represents zero i.e. would normalize to a zero value,
 *  without constant-time guarantee. */
static int secp256k1_fe_normalizes_to_zero_var(const secp256k1_fe *r);

/** Set a field element equal to a small (not greater than 0x7FFF), non-negative integer.
 *  Resulting field element is normalized; it has magnitude 0 if a == 0, and magnitude 1 otherwise.
 */
static void secp256k1_fe_set_int(secp256k1_fe *r, int a);

/** Sets a field element equal to zero, initializing all fields. */
static void secp256k1_fe_clear(secp256k1_fe *a);

/** Verify whether a field element is zero. Requires the input to be normalized. */
static int secp256k1_fe_is_zero(const secp256k1_fe *a);

/** Check the "oddness" of a field element. Requires the input to be normalized. */
static int secp256k1_fe_is_odd(const secp256k1_fe *a);

/** Compare two field elements. Requires magnitude-1 inputs. */
static int secp256k1_fe_equal(const secp256k1_fe *a, const secp256k1_fe *b);

/** Same as secp256k1_fe_equal, but may be variable time. */
static int secp256k1_fe_equal_var(const secp256k1_fe *a, const secp256k1_fe *b);

/** Compare two field elements. Requires both inputs to be normalized */
static int secp256k1_fe_cmp_var(const secp256k1_fe *a, const secp256k1_fe *b);

/** Set a field element equal to 32-byte big endian value. If successful, the resulting field element is normalized. */
static int secp256k1_fe_set_b32(secp256k1_fe *r, const unsigned char *a);

/** Convert a field element to a 32-byte big endian value. Requires the input to be normalized */
static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe *a);

/** Set a field element equal to the additive inverse of another. Takes a maximum magnitude of the input
 *  as an argument. The magnitude of the output is one higher. */
static void secp256k1_fe_negate(secp256k1_fe *r, const secp256k1_fe *a, int m);

/** Adds a small integer (up to 0x7FFF) to r. The resulting magnitude increases by one. */
static void secp256k1_fe_add_int(secp256k1_fe *r, int a);

/** Multiplies the passed field element with a small integer constant. Multiplies the magnitude by that
 *  small integer. */
static void secp256k1_fe_mul_int(secp256k1_fe *r, int a);

/** Adds a field element to another. The result has the sum of the inputs' magnitudes as magnitude. */
static void secp256k1_fe_add(secp256k1_fe *r, const secp256k1_fe *a);

/** Sets a field element to be the product of two others. Requires the inputs' magnitudes to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_mul(secp256k1_fe *r, const secp256k1_fe *a, const secp256k1_fe * SECP256K1_RESTRICT b);

/** Sets a field element to be the square of another. Requires the input's magnitude to be at most 8.
 *  The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_sqr(secp256k1_fe *r, const secp256k1_fe *a);

/** If a has a square root, it is computed in r and 1 is returned. If a does not
 *  have a square root, the root of its negation is computed and 0 is returned.
 *  The input's magnitude can be at most 8. The output magnitude is 1 (but not
 *  guaranteed to be normalized). The result in r will always be a square
 *  itself. */
static int secp256k1_fe_sqrt(secp256k1_fe *r, const secp256k1_fe *a);

/** Sets a field element to be the (modular) inverse of another. Requires the input's magnitude to be
 *  at most 8. The output magnitude is 1 (but not guaranteed to be normalized). */
static void secp256k1_fe_inv(secp256k1_fe *r, const secp256k1_fe *a);

/** Potentially faster version of secp256k1_fe_inv, without constant-time guarantee. */
static void secp256k1_fe_inv_var(secp256k1_fe *r, const secp256k1_fe *a);

/** Convert a field element to the storage type. */
static void secp256k1_fe_to_storage(secp256k1_fe_storage *r, const secp256k1_fe *a);

/** Convert a field element back from the storage type. */
static void secp256k1_fe_from_storage(secp256k1_fe *r, const secp256k1_fe_storage *a);

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time.  Both *r and *a must be initialized.*/
static void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_fe_storage *a, int flag);

/** If flag is true, set *r equal to *a; otherwise leave it. Constant-time.  Both *r and *a must be initialized.*/
static void secp256k1_fe_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag);

/** Halves the value of a field element modulo the field prime. Constant-time.
 *  For an input magnitude 'm', the output magnitude is set to 'floor(m/2) + 1'.
 *  The output is not guaranteed to be normalized, regardless of the input. */
static void secp256k1_fe_half(secp256k1_fe *r);

/** Sets each limb of 'r' to its upper bound at magnitude 'm'. The output will also have its
 *  magnitude set to 'm' and is normalized if (and only if) 'm' is zero. */
static void secp256k1_fe_get_bounds(secp256k1_fe *r, int m);

/** Determine whether a is a square (modulo p). */
static int secp256k1_fe_is_square_var(const secp256k1_fe *a);

#endif /* SECP256K1_FIELD_H */
