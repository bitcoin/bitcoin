#ifndef SECP256K1_SCHNORRSIG_BATCH_H
#define SECP256K1_SCHNORRSIG_BATCH_H

#include "secp256k1.h"
#include "secp256k1_schnorrsig.h"
#include "secp256k1_batch.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This header file implements batch verification functionality for Schnorr
 *  signature (see include/secp256k1_schnorrsig.h).
 */

/** Adds a Schnorr signature to the batch object (secp256k1_batch)
 *  defined in the Batch module (see include/secp256k1_batch.h).
 *
 *  Returns: 1: successfully added the signature to the batch
 *           0: unparseable signature or unusable batch (according to
 *              secp256k1_batch_usable).
 *  Args:    ctx: a secp256k1 context object (can be initialized for none).
 *         batch: a secp256k1 batch object created using `secp256k1_batch_create`.
 *  In:    sig64: pointer to the 64-byte signature to verify.
 *           msg: the message being verified. Can only be NULL if msglen is 0.
 *        msglen: length of the message.
 *        pubkey: pointer to an x-only public key to verify with (cannot be NULL).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_batch_add_schnorrsig(
    const secp256k1_context* ctx,
    secp256k1_batch *batch,
    const unsigned char *sig64,
    const unsigned char *msg,
    size_t msglen,
    const secp256k1_xonly_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(6);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_BATCH_H */
