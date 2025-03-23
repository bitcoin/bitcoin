#ifndef SECP256K1_TWEAK_CHECK_BATCH_H
#define SECP256K1_TWEAK_CHECK_BATCH_H

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"
#include "secp256k1_batch.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This header file implements batch verification functionality for
 *  x-only tweaked public key check (see include/secp256k1_extrakeys.h).
 */

/** Adds a x-only tweaked pubkey check to the batch object (secp256k1_batch)
 *  defined in the Batch module (see include/secp256k1_batch.h).
 *
 *  The tweaked pubkey is represented by its 32-byte x-only serialization and
 *  its pk_parity, which can both be obtained by converting the result of
 *  tweak_add to a secp256k1_xonly_pubkey.
 *
 *  Returns: 1: successfully added the tweaked pubkey check to the batch
 *           0: unparseable tweaked pubkey check or unusable batch (according to
 *              secp256k1_batch_usable).
 *  Args:            ctx: pointer to a context object initialized for verification.
 *                 batch: a secp256k1 batch object created using `secp256k1_batch_create`.
 *  In: tweaked_pubkey32: pointer to a serialized xonly_pubkey.
 *     tweaked_pk_parity: the parity of the tweaked pubkey (whose serialization
 *                        is passed in as tweaked_pubkey32). This must match the
 *                        pk_parity value that is returned when calling
 *                        secp256k1_xonly_pubkey_from_pubkey with the tweaked pubkey, or
 *                        the final secp256k1_batch_verify on this batch will fail.
 *       internal_pubkey: pointer to an x-only public key object to apply the tweak to.
 *               tweak32: pointer to a 32-byte tweak.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_batch_add_xonlypub_tweak_check(
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
