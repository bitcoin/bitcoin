#ifndef SECP256K1_ELLSWIFT_H
#define SECP256K1_ELLSWIFT_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This module provides an implementation of ElligatorSwift as well as a
 * version of x-only ECDH using it (including compatibility with BIP324).
 *
 * ElligatorSwift is described in https://eprint.iacr.org/2022/759 by
 * Chavez-Saab, Rodriguez-Henriquez, and Tibouchi. It permits encoding
 * uniformly chosen public keys as 64-byte arrays which are indistinguishable
 * from uniformly random arrays.
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
 * - Return the first in [u + 4 * Y^2, (-X/Y - u) / 2, (X/Y - u) / 2] that is an
 *   X coordinate on the curve (at least one of them is, for any u and t).
 *
 * Then an ElligatorSwift encoding of x consists of the 32-byte big-endian
 * encodings of field elements u and t concatenated, where f(u,t) = x.
 * The encoding algorithm is described in the paper, and effectively picks a
 * uniformly random pair (u,t) among those which encode x.
 *
 * If the Y coordinate is relevant, it is given the same parity as t.
 *
 * Changes w.r.t. the paper:
 * - The u=0, t=0, and u^3+t^2+7=0 conditions result in decoding to the point
 *   at infinity in the paper. Here they are remapped to finite points.
 * - The paper uses an additional encoding bit for the parity of y. Here the
 *   parity of t is used (negating t does not affect the decoded x coordinate,
 *   so this is possible).
 *
 * For mathematical background about the scheme, see the doc/ellswift.md file.
 */

/** A pointer to a function used by secp256k1_ellswift_xdh to hash the shared X
 *  coordinate along with the encoded public keys to a uniform shared secret.
 *
 *  Returns: 1 if a shared secret was successfully computed.
 *           0 will cause secp256k1_ellswift_xdh to fail and return 0.
 *           Other return values are not allowed, and the behaviour of
 *           secp256k1_ellswift_xdh is undefined for other return values.
 *  Out:     output:     pointer to an array to be filled by the function
 *  In:      x32:        pointer to the 32-byte serialized X coordinate
 *                       of the resulting shared point (will not be NULL)
 *           ell_a64:    pointer to the 64-byte encoded public key of party A
 *                       (will not be NULL)
 *           ell_b64:    pointer to the 64-byte encoded public key of party B
 *                       (will not be NULL)
 *           data:       arbitrary data pointer that is passed through
 */
typedef int (*secp256k1_ellswift_xdh_hash_function)(
    unsigned char *output,
    const unsigned char *x32,
    const unsigned char *ell_a64,
    const unsigned char *ell_b64,
    void *data
);

/** An implementation of an secp256k1_ellswift_xdh_hash_function which uses
 *  SHA256(prefix64 || ell_a64 || ell_b64 || x32), where prefix64 is the 64-byte
 *  array pointed to by data. */
SECP256K1_API const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_prefix;

/** An implementation of an secp256k1_ellswift_xdh_hash_function compatible with
 *  BIP324. It returns H_tag(ell_a64 || ell_b64 || x32), where H_tag is the
 *  BIP340 tagged hash function with tag "bip324_ellswift_xonly_ecdh". Equivalent
 *  to secp256k1_ellswift_xdh_hash_function_prefix with prefix64 set to
 *  SHA256("bip324_ellswift_xonly_ecdh")||SHA256("bip324_ellswift_xonly_ecdh").
 *  The data argument is ignored. */
SECP256K1_API const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_bip324;

/** Construct a 64-byte ElligatorSwift encoding of a given pubkey.
 *
 *  Returns: 1 always.
 *  Args:    ctx:        pointer to a context object
 *  Out:     ell64:      pointer to a 64-byte array to be filled
 *  In:      pubkey:     pointer to a secp256k1_pubkey containing an
 *                       initialized public key
 *           rnd32:      pointer to 32 bytes of randomness
 *
 * It is recommended that rnd32 consists of 32 uniformly random bytes, not
 * known to any adversary trying to detect whether public keys are being
 * encoded, though 16 bytes of randomness (padded to an array of 32 bytes,
 * e.g., with zeros) suffice to make the result indistinguishable from
 * uniform. The randomness in rnd32 must not be a deterministic function of
 * the pubkey (it can be derived from the private key, though).
 *
 * It is not guaranteed that the computed encoding is stable across versions
 * of the library, even if all arguments to this function (including rnd32)
 * are the same.
 *
 * This function runs in variable time.
 */
SECP256K1_API int secp256k1_ellswift_encode(
    const secp256k1_context *ctx,
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
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *ell64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Compute an ElligatorSwift public key for a secret key.
 *
 *  Returns: 1: secret was valid, public key was stored.
 *           0: secret was invalid, try again.
 *  Args:    ctx:        pointer to a context object
 *  Out:     ell64:      pointer to a 64-byte array to receive the ElligatorSwift
 *                       public key
 *  In:      seckey32:   pointer to a 32-byte secret key
 *           auxrnd32:   (optional) pointer to 32 bytes of randomness
 *
 * Constant time in seckey and auxrnd32, but not in the resulting public key.
 *
 * It is recommended that auxrnd32 contains 32 uniformly random bytes, though
 * it is optional (and does result in encodings that are indistinguishable from
 * uniform even without any auxrnd32). It differs from the (mandatory) rnd32
 * argument to secp256k1_ellswift_encode in this regard.
 *
 * This function can be used instead of calling secp256k1_ec_pubkey_create
 * followed by secp256k1_ellswift_encode. It is safer, as it uses the secret
 * key as entropy for the encoding (supplemented with auxrnd32, if provided).
 *
 * Like secp256k1_ellswift_encode, this function does not guarantee that the
 * computed encoding is stable across versions of the library, even if all
 * arguments (including auxrnd32) are the same.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ellswift_create(
    const secp256k1_context *ctx,
    unsigned char *ell64,
    const unsigned char *seckey32,
    const unsigned char *auxrnd32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Given a private key, and ElligatorSwift public keys sent in both directions,
 *  compute a shared secret using x-only Elliptic Curve Diffie-Hellman (ECDH).
 *
 *  Returns: 1: shared secret was successfully computed
 *           0: secret was invalid or hashfp returned 0
 *  Args:    ctx:       pointer to a context object.
 *  Out:     output:    pointer to an array to be filled by hashfp.
 *  In:      ell_a64:   pointer to the 64-byte encoded public key of party A
 *                      (will not be NULL)
 *           ell_b64:   pointer to the 64-byte encoded public key of party B
 *                      (will not be NULL)
 *           seckey32:  pointer to our 32-byte secret key
 *           party:     boolean indicating which party we are: zero if we are
 *                      party A, non-zero if we are party B. seckey32 must be
 *                      the private key corresponding to that party's ell_?64.
 *                      This correspondence is not checked.
 *           hashfp:    pointer to a hash function.
 *           data:      arbitrary data pointer passed through to hashfp.
 *
 * Constant time in seckey32.
 *
 * This function is more efficient than decoding the public keys, and performing
 * ECDH on them.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ellswift_xdh(
  const secp256k1_context *ctx,
  unsigned char *output,
  const unsigned char *ell_a64,
  const unsigned char *ell_b64,
  const unsigned char *seckey32,
  int party,
  secp256k1_ellswift_xdh_hash_function hashfp,
  void *data
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(7);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_ELLSWIFT_H */
