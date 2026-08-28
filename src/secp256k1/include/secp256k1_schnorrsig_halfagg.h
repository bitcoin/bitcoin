#ifndef SECP256K1_SCHNORRSIG_HALFAGG_H
#define SECP256K1_SCHNORRSIG_HALFAGG_H

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This module implements incremental (Half-)Aggregation of Schnorr
 *  signatures as specified by BIP 458 "Half-Aggregation of BIP 340 Signatures"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0458.mediawiki).
 */

/** (Half-)Aggregate a sequence of Schnorr signatures.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:           ctx: a secp256k1 context object.
 *  Out:         aggsig: pointer to an array of aggsig_len many bytes to
 *                       store the serialized aggregate signature. The size
 *                       is expected to be 32*(n+1) bytes.
 *  In/Out:  aggsig_len: size of the aggsig array that is passed in bytes;
 *                       will be overwritten to be the exact size of aggsig.
 *  In:         pubkeys: Array of n many x-only public keys.
 *                       Can only be NULL if n is 0.
 *               msgs32: Array of n many 32-byte messages.
 *                       Can only be NULL if n is 0.
 *               sigs64: Array of n many 64-byte signatures.
 *                       Can only be NULL if n is 0.
 *                    n: number of signatures to be aggregated.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_aggregate(
    const secp256k1_context *ctx,
    unsigned char *aggsig,
    size_t *aggsig_len,
    const secp256k1_xonly_pubkey *pubkeys,
    const unsigned char *msgs32,
    const unsigned char *sigs64,
    size_t n
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Incrementally (Half-)Aggregate a sequence of Schnorr
 *  signatures to an existing half-aggregate signature.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:           ctx: a secp256k1 context object.
 *  In/Out:      aggsig: pointer to the serialized aggregate signature
 *                       that is input. The first 32*(n_before+1) of this
 *                       array should hold the input aggsig. It will be
 *                       overwritten by the new serialized aggregate signature.
 *                       It should be large enough for that, see aggsig_len.
 *           aggsig_len: size of aggsig array in bytes.
 *                       Should be large enough to hold the new
 *                       serialized aggregate signature, i.e.,
 *                       should satisfy aggsig_len >= 32*(n_before+n_new+1).
 *                       It will be overwritten to be the exact size of the
 *                       resulting aggsig.
 *  In:     all_pubkeys: Array of (n_before + n_new) many x-only public keys,
 *                       including both the ones for the already aggregated signature
 *                       and the ones for the signatures that should be added.
 *                       Can only be NULL if n_before + n_new is 0.
 *           all_msgs32: Array of (n_before + n_new) many 32-byte messages,
 *                       including both the ones for the already aggregated signature
 *                       and the ones for the signatures that should be added.
 *                       Can only be NULL if n_before + n_new is 0.
 *           new_sigs64: Array of n_new many 64-byte signatures, containing the new
 *                       signatures that should be added. Can only be NULL if n_new is 0.
 *             n_before: Number of signatures that have already been aggregated
 *                       in the input aggregate signature.
 *                n_new: Number of signatures that should now be added
 *                       to the aggregate signature.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_inc_aggregate(
    const secp256k1_context *ctx,
    unsigned char *aggsig,
    size_t *aggsig_len,
    const secp256k1_xonly_pubkey* all_pubkeys,
    const unsigned char *all_msgs32,
    const unsigned char *new_sigs64,
    size_t n_before,
    size_t n_new
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Verify a (Half-)aggregate Schnorr signature.
 *
 *  Returns:          1: correct signature.
 *                    0: incorrect signature.
 *  Args:           ctx: a secp256k1 context object.
 *  In:         pubkeys: Array of n many x-only public keys. Can only be NULL if n is 0.
 *               msgs32: Array of n many 32-byte messages. Can only be NULL if n is 0.
 *                    n: number of signatures that have been aggregated.
 *               aggsig: Pointer to an array of aggsig_len many bytes
 *                       containing the serialized aggregate
 *                       signature to be verified.
 *           aggsig_len: Size of the aggregate signature in bytes.
 *                       Must be aggsig_len = 32*(n+1)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_aggverify(
    const secp256k1_context *ctx,
    const secp256k1_xonly_pubkey *pubkeys,
    const unsigned char *msgs32,
    size_t n,
    const unsigned char *aggsig,
    size_t aggsig_len
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(5);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_HALFAGG_H */
