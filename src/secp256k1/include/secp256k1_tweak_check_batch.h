#ifndef SECP256K1_TWEAK_CHECK_BATCH_H
#define SECP256K1_TWEAK_CHECK_BATCH_H

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"
#include "secp256k1_batch.h"

#ifdef __cplusplus
extern "C" {
#endif

 /** Implements batch verification for BIP-341 xonly tweaked public keys.
 *
 *  See include/secp256k1_extrakeys.h for the more details.
 */

/** Adds an x-only tweaked public key verification to the batch context.
 *
 *  The tweaked public key is represented by its 32-byte x-only serialization
 *  and its pk_parity, both obtained by converting the result of tweak_add to
 *  a secp256k1_xonly_pubkey.
 *
 *  Args:            ctx: pointer to a secp256k1 context object.
 *                 batch: pointer to a batch verification context object.
 *  In: tweaked_pubkey32: pointer to a serialized xonly tweaked pubkey (cannot be NULL).
 *     tweaked_pk_parity: parity of the xonly tweaked public key. Must match the pk_parity returned by
 *                        secp256k1_xonly_pubkey_from_pubkey when called with the tweaked public key,
 *                        or batch verification will fail.
 *       internal_pubkey: pointer to an x-only public key object to apply the tweak to.
 *               tweak32: pointer to a 32-byte tweak.
 */
SECP256K1_API void secp256k1_batch_add_xonlypub_tweak_check(
    const secp256k1_context* ctx,
    secp256k1_batch *batch,
    const unsigned char *tweaked_pubkey32,
    int tweaked_pk_parity,
    const secp256k1_xonly_pubkey *internal_pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_TWEAK_CHECK_BATCH_H */
