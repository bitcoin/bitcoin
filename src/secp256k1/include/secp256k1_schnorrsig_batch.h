#ifndef SECP256K1_SCHNORRSIG_BATCH_H
#define SECP256K1_SCHNORRSIG_BATCH_H

#include "secp256k1.h"
#include "secp256k1_schnorrsig.h"
#include "secp256k1_batch.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Implements batch verification for BIP-340 Schnorr signatures.
 *
 *  See include/secp256k1_schnorrsig.h for the signature scheme details.
 */

/** Adds a Schnorr signature verification to the batch context.
 *
 *  Args:    ctx: pointer to a secp256k1 context object.
 *         batch: pointer to a batch verification context object.
 *  In:    sig64: pointer to the BIP-340 Schnorr signature to verify (cannot be NULL).
 *           msg: pointer to the message being verified (can only be NULL if msglen is 0).
 *        msglen: length of the message in bytes.
 *        pubkey: pointer to an x-only public key to verify with (cannot be NULL).
 */
SECP256K1_API void secp256k1_batch_add_schnorrsig(
    const secp256k1_context* ctx,
    secp256k1_batch *batch,
    const unsigned char *sig64,
    const unsigned char *msg,
    size_t msglen,
    const secp256k1_xonly_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(6);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_BATCH_H */
