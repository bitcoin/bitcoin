#ifndef SECP256K1_SCHNORRSIG_H
#define SECP256K1_SCHNORRSIG_H

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This module implements a variant of Schnorr signatures compliant with
 *  Bitcoin Improvement Proposal 340 "Schnorr Signatures for secp256k1"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 */

/** A pointer to a function to deterministically generate a nonce.
 *
 *  Same as secp256k1_nonce function with the exception of accepting an
 *  additional pubkey argument and not requiring an attempt argument. The pubkey
 *  argument can protect signature schemes with key-prefixed challenge hash
 *  inputs against reusing the nonce when signing with the wrong precomputed
 *  pubkey.
 *
 *  Returns: 1 if a nonce was successfully generated. 0 will cause signing to
 *           return an error.
 *  Out:  nonce32: pointer to a 32-byte array to be filled by the function
 *  In:       msg: the message being verified. Is NULL if and only if msglen
 *                 is 0.
 *         msglen: the length of the message
 *          key32: pointer to a 32-byte secret key (will not be NULL)
 *     xonly_pk32: the 32-byte serialized xonly pubkey corresponding to key32
 *                 (will not be NULL)
 *           algo: pointer to an array describing the signature
 *                 algorithm (will not be NULL)
 *        algolen: the length of the algo array
 *           data: arbitrary data pointer that is passed through
 *
 *  Except for test cases, this function should compute some cryptographic hash of
 *  the message, the key, the pubkey, the algorithm description, and data.
 */
typedef int (*secp256k1_nonce_function_hardened)(
    unsigned char *nonce32,
    const unsigned char *msg,
    size_t msglen,
    const unsigned char *key32,
    const unsigned char *xonly_pk32,
    const unsigned char *algo,
    size_t algolen,
    void *data
);

/** An implementation of the nonce generation function as defined in Bitcoin
 *  Improvement Proposal 340 "Schnorr Signatures for secp256k1"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
 *
 *  If a data pointer is passed, it is assumed to be a pointer to 32 bytes of
 *  auxiliary random data as defined in BIP-340. If the data pointer is NULL,
 *  the nonce derivation procedure follows BIP-340 by setting the auxiliary
 *  random data to zero. The algo argument must be non-NULL, otherwise the
 *  function will fail and return 0. The hash will be tagged with algo.
 *  Therefore, to create BIP-340 compliant signatures, algo must be set to
 *  "BIP0340/nonce" and algolen to 13.
 */
SECP256K1_API extern const secp256k1_nonce_function_hardened secp256k1_nonce_function_bip340;

/** Data structure that contains additional arguments for schnorrsig_sign_custom.
 *
 *  A schnorrsig_extraparams structure object can be initialized correctly by
 *  setting it to SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT.
 *
 *  Members:
 *      magic: set to SECP256K1_SCHNORRSIG_EXTRAPARAMS_MAGIC at initialization
 *             and has no other function than making sure the object is
 *             initialized.
 *    noncefp: pointer to a nonce generation function. If NULL,
 *             secp256k1_nonce_function_bip340 is used
 *      ndata: pointer to arbitrary data used by the nonce generation function
 *             (can be NULL). If it is non-NULL and
 *             secp256k1_nonce_function_bip340 is used, then ndata must be a
 *             pointer to 32-byte auxiliary randomness as per BIP-340.
 */
typedef struct {
    unsigned char magic[4];
    secp256k1_nonce_function_hardened noncefp;
    void* ndata;
} secp256k1_schnorrsig_extraparams;

#define SECP256K1_SCHNORRSIG_EXTRAPARAMS_MAGIC { 0xda, 0x6f, 0xb3, 0x8c }
#define SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT {\
    SECP256K1_SCHNORRSIG_EXTRAPARAMS_MAGIC,\
    NULL,\
    NULL\
}

/** Create a Schnorr signature.
 *
 *  Does _not_ strictly follow BIP-340 because it does not verify the resulting
 *  signature. Instead, you can manually use secp256k1_schnorrsig_verify and
 *  abort if it fails.
 *
 *  This function only signs 32-byte messages. If you have messages of a
 *  different size (or the same size but without a context-specific tag
 *  prefix), it is recommended to create a 32-byte message hash with
 *  secp256k1_tagged_sha256 and then sign the hash. Tagged hashing allows
 *  providing an context-specific tag for domain separation. This prevents
 *  signatures from being valid in multiple contexts by accident.
 *
 *  Returns 1 on success, 0 on failure.
 *  Args:    ctx: pointer to a context object, initialized for signing.
 *  Out:   sig64: pointer to a 64-byte array to store the serialized signature.
 *  In:    msg32: the 32-byte message being signed.
 *       keypair: pointer to an initialized keypair.
 *    aux_rand32: 32 bytes of fresh randomness. While recommended to provide
 *                this, it is only supplemental to security and can be NULL. A
 *                NULL argument is treated the same as an all-zero one. See
 *                BIP-340 "Default Signing" for a full explanation of this
 *                argument and for guidance if randomness is expensive.
 */
SECP256K1_API int secp256k1_schnorrsig_sign32(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_keypair *keypair,
    const unsigned char *aux_rand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Same as secp256k1_schnorrsig_sign32, but DEPRECATED. Will be removed in
 *  future versions. */
SECP256K1_API int secp256k1_schnorrsig_sign(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_keypair *keypair,
    const unsigned char *aux_rand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4)
  SECP256K1_DEPRECATED("Use secp256k1_schnorrsig_sign32 instead");

/** Create a Schnorr signature with a more flexible API.
 *
 *  Same arguments as secp256k1_schnorrsig_sign except that it allows signing
 *  variable length messages and accepts a pointer to an extraparams object that
 *  allows customizing signing by passing additional arguments.
 *
 *  Creates the same signatures as schnorrsig_sign if msglen is 32 and the
 *  extraparams.ndata is the same as aux_rand32.
 *
 *  In:     msg: the message being signed. Can only be NULL if msglen is 0.
 *       msglen: length of the message
 *  extraparams: pointer to a extraparams object (can be NULL)
 */
SECP256K1_API int secp256k1_schnorrsig_sign_custom(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg,
    size_t msglen,
    const secp256k1_keypair *keypair,
    secp256k1_schnorrsig_extraparams *extraparams
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(5);

/** Verify a Schnorr signature.
 *
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *  Args:    ctx: a secp256k1 context object, initialized for verification.
 *  In:    sig64: pointer to the 64-byte signature to verify.
 *           msg: the message being verified. Can only be NULL if msglen is 0.
 *        msglen: length of the message
 *        pubkey: pointer to an x-only public key to verify with (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorrsig_verify(
    const secp256k1_context* ctx,
    const unsigned char *sig64,
    const unsigned char *msg,
    size_t msglen,
    const secp256k1_xonly_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(5);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_SCHNORRSIG_H */
