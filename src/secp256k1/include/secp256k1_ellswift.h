#ifndef SECP256K1_ELLSWIFT_H
#define SECP256K1_ELLSWIFT_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This module provides an implementation of ElligatorSwift as well as
 * a version of x-only ECDH using it.
 *
 * ElligatorSwift is described in https://eprint.iacr.org/2022/759 by
 * Chavez-Saab, Rodriguez-Henriquez, and Tibouchi. It permits encoding
 * public keys in 64-byte objects which are indistinguishable from
 * uniformly random.
 *
 * Let f be the function from pairs of field elements to point X coordinates,
 * defined as follows (all operations modulo p = 2^256 - 2^32 - 977)
 * f(u,t):
 * - Let C = 0xa2d2ba93507f1df233770c2a797962cc61f6d15da14ecd47d8d27ae1cd5f852,
 *   a square root of -3.
 * - If u=0, set u=1 instead.
 * - If t=0, set t=1 instead.
 * - If u^3 + t^2 + 7 = 0, multiply t by 2.
 * - Let X = (u^3 + 7 - t^2) / (2 * t)
 * - Let Y = (X + t) / (C * u)
 * - Return the first of [u + 4 * Y^2, (-X/Y - u) / 2, (X/Y - u) / 2] that is an
 *   X coordinate on the curve (at least one of them is, for any inputs u and t).
 *
 * Then an ElligatorSwift encoding of x consists of the 32-byte big-endian
 * encodings of field elements u and t concatenated, where f(u,t) = x.
 * The encoding algorithm is described in the paper, and effectively picks a
 * uniformly random pair (u,t) among those which encode x.
 *
 * If the Y coordinate is relevant, it is given the same parity as t.
 *
 * Changes w.r.t. the the paper:
 * - The u=0, t=0, and u^3+t^2+7=0 conditions result in decoding to the point
 *   at infinity in the paper. Here they are remapped to finite points.
 * - The paper uses an additional encoding bit for the parity of y. Here the
 *   parity of t is used (negating t does not affect the decoded x coordinate,
 *   so this is possible).
 */

/** A pointer to a function used for hashing the shared X coordinate along
 *  with the encoded public keys to a uniform shared secret.
 *
 *  Returns: 1 if a shared secret was was successfully computed.
 *           0 will cause secp256k1_ellswift_xdh to fail and return 0.
 *           Other return values are not allowed, and the behaviour of
 *           secp256k1_ellswift_xdh is undefined for other return values.
 *  Out:     output:     pointer to an array to be filled by the function
 *  In:      x32:        pointer to the 32-byte serialized X coordinate
 *                       of the resulting shared point
 *           ours64:     pointer to the 64-byte encoded public key we sent
 *                       to the other party
 *           theirs64:   pointer to the 64-byte encoded public key we received
 *                       from the other party
 *           data:       arbitrary data pointer that is passed through
 */
typedef int (*secp256k1_ellswift_xdh_hash_function)(
  unsigned char *output,
  const unsigned char *x32,
  const unsigned char *ours64,
  const unsigned char *theirs64,
  void *data
);

/** An implementation of an secp256k1_ellswift_xdh_hash_function which uses
 *  SHA256(key1 || key2 || x32), where (key1, key2) = sorted([ours64, theirs64]), and
 *  ignores data. The sorting is lexicographic. */
SECP256K1_API extern const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_sha256;

/** A default secp256k1_ellswift_xdh_hash_function, currently secp256k1_ellswift_xdh_hash_function_sha256. */
SECP256K1_API extern const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_default;

/* Construct a 64-byte ElligatorSwift encoding of a given pubkey.
 *
 *  Returns: 1 when pubkey is valid.
 *  Args:    ctx:        pointer to a context object
 *  Out:     ell64:      pointer to a 64-byte array to be filled
 *  In:      pubkey:     a pointer to a secp256k1_pubkey containing an
 *                       initialized public key
 *           rnd32:      pointer to 32 bytes of entropy (must be unpredictable)
 *
 * This function runs in variable time.
 */
SECP256K1_API int secp256k1_ellswift_encode(
    const secp256k1_context* ctx,
    unsigned char *ell64,
    const secp256k1_pubkey *pubkey,
    const unsigned char *rnd32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Decode a 64-bytes ElligatorSwift encoded public key.
 *
 *  Returns: always 1
 *  Args:    ctx:        pointer to a context object
 *  Out:     pubkey:     pointer to a secp256k1_pubkey that will be filled
 *  In:      ell64:      pointer to a 64-byte array to decode
 *
 * This function runs in variable time.
 */
SECP256K1_API int secp256k1_ellswift_decode(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *ell64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Compute an ElligatorSwift public key for a secret key.
 *
 *  Returns: 1: secret was valid, public key was stored.
 *           0: secret was invalid, try again.
 *  Args:    ctx:         pointer to a context object, initialized for signing.
 *  Out:     ell64:       pointer to a 64-byte area to receive the ElligatorSwift public key
 *  In:      seckey32:    pointer to a 32-byte secret key.
 *           auxrand32:   (optional) pointer to 32 bytes of additional randomness
 *
 * Constant time in seckey and auxrand32, but not in the resulting public key.
 *
 * This function can be used instead of calling secp256k1_ec_pubkey_create followed
 * by secp256k1_ellswift_encode. It is safer, as it can use the secret key as
 * entropy for the encoding. That means that if the secret key itself is
 * unpredictable, no additional auxrand32 is needed to achieve indistinguishability
 * of the encoding.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ellswift_create(
    const secp256k1_context* ctx,
    unsigned char *ell64,
    const unsigned char *seckey32,
    const unsigned char *auxrand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Given a private key, and ElligatorSwift public keys sent in both directions,
 *  compute a shared secret using x-only Diffie-Hellman.
 *
 *  Returns: 1: shared secret was succesfully computed
 *           0: secret was invalid or hashfp returned 0
 *  Args:    ctx:        pointer to a context object.
 *  Out:     output:     pointer to an array to be filled by hashfp.
 *  In:      theirs64:   a pointer to the 64-byte ElligatorSwift public key received from the other party.
 *           ours64:     a pointer to the 64-byte ElligatorSwift public key sent to the other party.
 *           seckey32:   a pointer to the 32-byte private key corresponding to ours64.
 *           hashfp:     pointer to a hash function. If NULL,
 *                       secp256k1_elswift_xdh_hash_function_default is used
 *                       (in which case, 32 bytes will be written to output).
 *           data:       arbitrary data pointer that is passed through to hashfp
 *                       (ignored for secp256k1_ellswift_xdh_hash_function_default).
 *
 * Constant time in seckey32.
 *
 * This function is more efficient than decoding the public keys, and performing ECDH on them.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ellswift_xdh(
  const secp256k1_context* ctx,
  unsigned char *output,
  const unsigned char* theirs64,
  const unsigned char* ours64,
  const unsigned char* seckey32,
  secp256k1_ellswift_xdh_hash_function hashfp,
  void *data
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);


#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_ELLSWIFT_H */
