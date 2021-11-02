#ifndef SECP256K1_ELLSQ_H
#define SECP256K1_ELLSQ_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This module provides an implementation of the Elligator Squared encoding
 * for secp256k1 public keys. Given a uniformly random public key, this
 * produces a 64-byte encoding that is indistinguishable from uniformly
 * random bytes.
 *
 * Elligator Squared is described in https://eprint.iacr.org/2014/043.pdf by
 * Mehdi Tibouchi. The mapping function used is described in
 * https://www.di.ens.fr/~fouque/pub/latincrypt12.pdf by Fouque and Tibouchi.
 *
 * Let f be the function from field elements to curve points, defined as
 * follows:
 * f(t):
 * - Let c = 0xa2d2ba93507f1df233770c2a797962cc61f6d15da14ecd47d8d27ae1cd5f852
 * - Let x1 = (c - 1)/2 - c*t^2 / (t^2 + 8) (mod p)
 * - Let x2 = (-c - 1)/2 + c*t^2 / (t^2 + 8) (mod p)
 * - Let x3 = 1 - (t^2 + 8)^2 / (3*t^2) (mod p)
 * - Let x be the first of [x1,x2,x3] that is an X coordinate on the curve
 *   (at least one of them is, for any field element t).
 * - Let y be the the corresponding Y coordinate to x, with the same parity
 *   as t (even if t is even, odd if t is odd).
 * - Return the curve point with coordinates (x, y).
 *
 * Then an Elligator Squared encoding of P consists of the 32-byte big-endian
 * encodings of field elements u1 and u2 concatenated, where f(u1)+f(u2) = P.
 * The encoding algorithm is described in the paper, and effectively picks a
 * uniformly random pair (u1,u2) among those which encode P.
 *
 * To make the encoding able to deal with all inputs, if f(u1)+f(u2) is the
 * point at infinity, the decoding is defined to be f(u1) instead.
 */

/* Construct a 64-byte Elligator Squared encoding of a given pubkey.
 *
 *  Returns: 1 when pubkey is valid.
 *  Args:    ctx:        pointer to a context object
 *  Out:     ell64:      pointer to a 64-byte array to be filled
 *  In:      rnd32:      pointer to 32 bytes of entropy (must be unpredictable)
 *           pubkey:     a pointer to a secp256k1_pubkey containing an
 *                       initialized public key
 *
 * This function runs in variable time.
 */
SECP256K1_API int secp256k1_ellsq_encode(
    const secp256k1_context* ctx,
    unsigned char *ell64,
    const unsigned char *rnd32,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Decode a 64-bytes Elligator Squared encoded public key.
 *
 *  Returns: always 1
 *  Args:    ctx:        pointer to a context object
 *  Out:     pubkey:     pointer to a secp256k1_pubkey that will be filled
 *  In:      ell64:      pointer to a 64-byte array to decode
 *
 * This function runs in variable time.
 */
SECP256K1_API int secp256k1_ellsq_decode(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *ell64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_ELLSQ_H */
