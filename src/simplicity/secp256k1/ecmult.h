/***********************************************************************
 * Copyright (c) 2013, 2014, 2017 Pieter Wuille, Andrew Poelstra       *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_H
#define SECP256K1_ECMULT_H

#include "group.h"
#include "scalar.h"

#define ECMULT_WINDOW_SIZE 15

/** The number of entries a table with precomputed multiples needs to have. */
#define ECMULT_TABLE_SIZE(w) (1L << ((w)-2))

/** Double multiply: R = na*A + ng*G */
static void secp256k1_ecmult(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_scalar *na, const secp256k1_scalar *ng);

#if 0
typedef int (secp256k1_ecmult_multi_callback)(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data);

/**
 * Multi-multiply: R = inp_g_sc * G + sum_i ni * Ai.
 * Chooses the right algorithm for a given number of points and scratch space
 * size. Resets and overwrites the given scratch space. If the points do not
 * fit in the scratch space the algorithm is repeatedly run with batches of
 * points. If no scratch space is given then a simple algorithm is used that
 * simply multiplies the points with the corresponding scalars and adds them up.
 * Returns: 1 on success (including when inp_g_sc is NULL and n is 0)
 *          0 if there is not enough scratch space for a single point or
 *          callback returns 0
 */
static int secp256k1_ecmult_multi_var(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n);
#endif

#endif /* SECP256K1_ECMULT_H */
