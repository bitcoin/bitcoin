#ifndef SECP256K1_BATCH_H
#define SECP256K1_BATCH_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This module implements a Batch Verification object that supports:
 *
 *  1. Schnorr signatures compliant with Bitcoin Improvement Proposal 340
 *     "Schnorr Signatures for secp256k1"
 *     (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 *
 *  2. Taproot commitments compliant with Bitcoin Improvemtn Proposal 341
 *     "Taproot: SegWit version 1 spending rules"
 *     (https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki).
 */

/** Opaque data structure that holds information required for the batch verification.
 *
 *  The purpose of this structure is to store elliptic curve points, their scalar
 *  coefficients, and scalar coefficient of generator point participating in Multi-Scalar
 *  Point Multiplication computation, which is done by `secp256k1_ecmult_strauss_batch_internal`
 */
typedef struct secp256k1_batch_struct secp256k1_batch;

/** Create a secp256k1 batch object object (in dynamically allocated memory).
 *
 *  This function uses malloc to allocate memory. It is guaranteed that malloc is
 *  called at most twice for every call of this function.
 *
 *  Returns: a newly created batch object.
 *  Args:        ctx: an existing `secp256k1_context` object. Not to be confused
 *                    with the batch object object that this function creates.
 *  In:    max_terms: Max number of (scalar, curve point) pairs that the batch
 *                    object can store.
 *                    1. `batch_add_schnorrsig`         - adds two scalar-point pairs to the batch
 *                    2. `batch_add_xonpub_tweak_check` - adds one scalar-point pair to the batch
 *                    Hence, for adding n schnorrsigs and m tweak checks, `max_terms`
 *                    should be set to 2*n + m.
 *        aux_rand16: 16 bytes of fresh randomness. While recommended to provide
 *                    this, it is only supplemental to security and can be NULL. A
 *                    NULL argument is treated the same as an all-zero one.
 */
SECP256K1_API secp256k1_batch* secp256k1_batch_create(
    const secp256k1_context* ctx,
    size_t max_terms,
    const unsigned char *aux_rand16
) SECP256K1_ARG_NONNULL(1) SECP256K1_WARN_UNUSED_RESULT;

/** Destroy a secp256k1 batch object (created in dynamically allocated memory).
 *
 *  The batch object's pointer may not be used afterwards.
 *
 *  Args:       ctx: a secp256k1 context object.
 *            batch: an existing batch object to destroy, constructed
 *                   using `secp256k1_batch_create`
 */
SECP256K1_API void secp256k1_batch_destroy(
    const secp256k1_context* ctx,
    secp256k1_batch* batch
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Checks if a batch can be used by the `secp256k1_batch_add_*` APIs.
 *
 *  Returns: 1: batch can be used by `secp256k1_batch_add_*` APIs.
 *           0: batch cannot be used by `secp256k1_batch_add_*` APIs.
 *
 *  Args:    ctx: a secp256k1 context object (can be initialized for none).
 *         batch: a secp256k1 batch object that contains a set of schnorrsigs/tweaks.
 *
 *  You are advised to check if `secp256k1_batch_usable` returns 1 before calling
 *  any `secp256k1_batch_add_*` API. We recommend this because `secp256k1_batch_add_*`
 *  will fail in two cases:
 *       - case 1: unparsable input (schnorrsig or tweak check)
 *       - case 2: unusable (or invalid) batch
 *  Calling `secp256k1_batch_usable` beforehand helps eliminate case 2 if
 *  `secp256k1_batch_add_*` fails.
 *
 *  If you ignore the above advice, all the secp256k1_batch APIs will still
 *  work correctly. It simply makes it hard to understand the reason behind
 *  `secp256k1_batch_add_*` failure (if occurs).
 */
SECP256K1_API int secp256k1_batch_usable(
    const secp256k1_context *ctx,
    const secp256k1_batch *batch
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Verify the set of schnorr signatures or tweaked pubkeys present in the secp256k1_batch.
 *
 *  Returns: 1: every schnorrsig/tweak (in batch) is valid
 *           0: atleaset one of the schnorrsig/tweak (in batch) is invalid
 *
 *  In particular, returns 1 if the batch object is empty (does not contain any schnorrsigs/tweaks).
 *
 *  Args:    ctx: a secp256k1 context object (can be initialized for none).
 *         batch: a secp256k1 batch object that contains a set of schnorrsigs/tweaks.
 */
SECP256K1_API int secp256k1_batch_verify(
    const secp256k1_context *ctx,
    secp256k1_batch *batch
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_BATCH_H */
