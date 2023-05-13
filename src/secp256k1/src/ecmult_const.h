/***********************************************************************
 * Copyright (c) 2015 Andrew Poelstra                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_CONST_H
#define SECP256K1_ECMULT_CONST_H

#include "scalar.h"
#include "group.h"

/**
 * Multiply: R = q*A (in constant-time)
 * Here `bits` should be set to the maximum bitlength of the _absolute value_ of `q`, plus
 * one because we internally sometimes add 2 to the number during the WNAF conversion.
 * A must not be infinity.
 */
static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *q, int bits);

/**
 * Same as secp256k1_ecmult_const, but takes in an x coordinate of the base point
 * only, specified as fraction n/d (numerator/denominator). Only the x coordinate of the result is
 * returned.
 *
 * If known_on_curve is 0, a verification is performed that n/d is a valid X
 * coordinate, and 0 is returned if not. Otherwise, 1 is returned.
 *
 * d being NULL is interpreted as d=1. If non-NULL, d must not be zero. q must not be zero.
 *
 * Constant time in the value of q, but not any other inputs.
 */
static int secp256k1_ecmult_const_xonly(
    secp256k1_fe *r,
    const secp256k1_fe *n,
    const secp256k1_fe *d,
    const secp256k1_scalar *q,
    int bits,
    int known_on_curve
);

#endif /* SECP256K1_ECMULT_CONST_H */
