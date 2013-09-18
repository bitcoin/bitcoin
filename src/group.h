// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_GROUP_
#define _SECP256K1_GROUP_

#include "num.h"
#include "field.h"

/** A group element of the secp256k1 curve, in affine coordinates. */
typedef struct {
    secp256k1_fe_t x;
    secp256k1_fe_t y;
    int infinity; // whether this represents the point at infinity
} secp256k1_ge_t;

/** A group element of the secp256k1 curve, in jacobian coordinates. */
typedef struct {
    secp256k1_fe_t x; // actual X: x/z^2
    secp256k1_fe_t y; // actual Y: y/z^3
    secp256k1_fe_t z;
    int infinity; // whether this represents the point at infinity
} secp256k1_gej_t;

/** Global constants related to the group */
typedef struct {
    secp256k1_num_t order; // the order of the curve (= order of its generator)
    secp256k1_num_t half_order; // half the order of the curve (= order of its generator)
    secp256k1_ge_t g; // the generator point

    // constants related to secp256k1's efficiently computable endomorphism
    secp256k1_fe_t beta;
    secp256k1_num_t lambda, a1b2, b1, a2;
} secp256k1_ge_consts_t;

static const secp256k1_ge_consts_t *secp256k1_ge_consts = NULL;

/** Initialize the group module. */
void static secp256k1_ge_start(void);

/** De-initialize the group module. */
void static secp256k1_ge_stop(void);

/** Set a group element equal to the point at infinity */
void static secp256k1_ge_set_infinity(secp256k1_ge_t *r);

/** Set a group element equal to the point with given X and Y coordinates */
void static secp256k1_ge_set_xy(secp256k1_ge_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y);

/** Set a group element (jacobian) equal to the point with given X coordinate, and given oddness for Y.
    The result is not guaranteed to be valid. */
void static secp256k1_ge_set_xo(secp256k1_ge_t *r, const secp256k1_fe_t *x, int odd);

/** Check whether a group element is the point at infinity. */
int  static secp256k1_ge_is_infinity(const secp256k1_ge_t *a);

/** Check whether a group element is valid (i.e., on the curve). */
int  static secp256k1_ge_is_valid(const secp256k1_ge_t *a);

void static secp256k1_ge_neg(secp256k1_ge_t *r, const secp256k1_ge_t *a);

/** Get a hex representation of a point. *rlen will be overwritten with the real length. */
void static secp256k1_ge_get_hex(char *r, int *rlen, const secp256k1_ge_t *a);

/** Set a group element equal to another which is given in jacobian coordinates */
void static secp256k1_ge_set_gej(secp256k1_ge_t *r, secp256k1_gej_t *a);


/** Set a group element (jacobian) equal to the point at infinity. */
void static secp256k1_gej_set_infinity(secp256k1_gej_t *r);

/** Set a group element (jacobian) equal to the point with given X and Y coordinates. */
void static secp256k1_gej_set_xy(secp256k1_gej_t *r, const secp256k1_fe_t *x, const secp256k1_fe_t *y);

/** Set a group element (jacobian) equal to another which is given in affine coordinates. */
void static secp256k1_gej_set_ge(secp256k1_gej_t *r, const secp256k1_ge_t *a);

/** Get the X coordinate of a group element (jacobian). */
void static secp256k1_gej_get_x(secp256k1_fe_t *r, const secp256k1_gej_t *a);

/** Set r equal to the inverse of a (i.e., mirrored around the X axis) */
void static secp256k1_gej_neg(secp256k1_gej_t *r, const secp256k1_gej_t *a);

/** Check whether a group element is the point at infinity. */
int  static secp256k1_gej_is_infinity(const secp256k1_gej_t *a);

/** Set r equal to the double of a. */
void static secp256k1_gej_double(secp256k1_gej_t *r, const secp256k1_gej_t *a);

/** Set r equal to the sum of a and b. */
void static secp256k1_gej_add(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_gej_t *b);

/** Set r equal to the sum of a and b (with b given in jacobian coordinates). This is more efficient
    than secp256k1_gej_add. */
void static secp256k1_gej_add_ge(secp256k1_gej_t *r, const secp256k1_gej_t *a, const secp256k1_ge_t *b);

/** Get a hex representation of a point. *rlen will be overwritten with the real length. */
void static secp256k1_gej_get_hex(char *r, int *rlen, const secp256k1_gej_t *a);

/** Set r to be equal to lambda times a, where lambda is chosen in a way such that this is very fast. */
void static secp256k1_gej_mul_lambda(secp256k1_gej_t *r, const secp256k1_gej_t *a);

/** Find r1 and r2 such that r1+r2*lambda = a, and r1 and r2 are maximum 128 bits long (given that a is
    not more than 256 bits). */
void static secp256k1_gej_split_exp(secp256k1_num_t *r1, secp256k1_num_t *r2, const secp256k1_num_t *a);

#endif
