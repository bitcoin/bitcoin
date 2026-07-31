#ifndef SECP256K1_BATCH_H
#define SECP256K1_BATCH_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This module supports batch verification for the following schemes:
 *
 *  1. BIP340 Schnorr signatures (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 *  2. BIP341 Tweaked public keys (https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki).
 */

/** Opaque data structure for batch verification context.
 *
 *  Stores points and scalars extracted from Schnorr signatures and tweaked
 *  public key checks. This data is then passed to the batch verification API
 *  to verify multiple signatures/tweak checks efficiently in a single operation.
 */
typedef struct secp256k1_batch_struct secp256k1_batch;

/** Create a secp256k1 batch verification context object (in dynamically allocated memory).
 *
 *  Returns: pointer to a newly created batch context object.
 *  Args:        ctx: pointer to a secp256k1 context object.
 *  In:    max_terms: the maximum number of schnorr signatures and/or tweak checks
 *                    that will be added to the batch object. For n schnorr
 *                    signatures and m tweak checks, this should be set to at
 *                    least 2*n + m.
 *        aux_rand16: 16 bytes of fresh randomness. While recommended to provide
 *                    this, it is only supplemental to security and can be NULL. A
 *                    NULL argument is treated the same as an all-zero one.
 */
SECP256K1_API secp256k1_batch* secp256k1_batch_create(
    const secp256k1_context* ctx,
    size_t max_terms,
    const unsigned char *aux_rand16
) SECP256K1_ARG_NONNULL(1) SECP256K1_WARN_UNUSED_RESULT;

/** Resets a secp256k1 batch verification context object (no memory allocation).
 *
 *  Use this function when you have a new set of signatures and/or tweaks to
 *  batch verify and want to reuse an existing batch context instead of creating
 *  a new one. This avoids additional memory allocation.
 *
 *  Args:        ctx: pointer to a secp256k1 context object.
 *             batch: pointer to a batch verification context to reset.
 *
 */
SECP256K1_API void secp256k1_batch_reset(
    const secp256k1_context *ctx,
    secp256k1_batch *batch
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Destroy a secp256k1 batch verification context object (created in dynamically allocated memory).
 *
 *  The batch verification context pointer may not be used afterwards.
 *
 *  Args:       ctx: pointer to a secp256k1 context object.
 *            batch: pointer to a batch verification context to destroy.
 */
SECP256K1_API void secp256k1_batch_destroy(
    const secp256k1_context* ctx,
    secp256k1_batch* batch
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Verifies all Schnorr signatures and/or tweak checks in the batch context.
 *
 *  Returns: 1 if all signatures and tweaks in the batch are valid.
 *           0 if at least one signature or tweak in the batch is invalid.
 *
 *  Note: Returns 1 if the batch context is empty (contains no signatures or tweaks).
 *
 *  Args:    ctx: pointer to a secp256k1 context object.
 *         batch: pointer to a batch verification context object.
 */
SECP256K1_API int secp256k1_batch_verify(
    const secp256k1_context *ctx,
    secp256k1_batch *batch
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_BATCH_H */
