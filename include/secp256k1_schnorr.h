#ifndef _SECP256K1_SCHNORR_
# define _SECP256K1_SCHNORR_

# include "secp256k1.h"

# ifdef __cplusplus
extern "C" {
# endif

/** Create a signature using a custom EC-Schnorr-SHA256 construction. It
 *  produces non-malleable 64-byte signatures which support public key recovery
 *  batch validation, and multiparty signing.
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, or the private key was
 *              invalid.
 *  In:      ctx:    pointer to a context object, initialized for signing
 *                   (cannot be NULL)
 *           msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL,
 *                   secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation
 *                   function (can be NULL)
 *  Out:     sig64:  pointer to a 64-byte array where the signature will be
 *                   placed (cannot be NULL)
 */
int secp256k1_schnorr_sign(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  unsigned char *sig64,
  const unsigned char *seckey,
  secp256k1_nonce_function_t noncefp,
  const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verify a signature created by secp256k1_schnorr_sign.
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *  In:      ctx:       a secp256k1 context object, initialized for verification.
 *           msg32:     the 32-byte message hash being verified (cannot be NULL)
 *           sig64:     the 64-byte signature being verified (cannot be NULL)
 *           pubkey:    the public key to verify with (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorr_verify(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const unsigned char *sig64,
  const secp256k1_pubkey_t *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Recover an EC public key from a Schnorr signature created using
 *  secp256k1_schnorr_sign.
 *  Returns: 1: public key successfully recovered (which guarantees a correct
 *           signature).
 *           0: otherwise.
 *  In:      ctx:        pointer to a context object, initialized for
 *                       verification (cannot be NULL)
 *           msg32:      the 32-byte message hash assumed to be signed (cannot
 *                       be NULL)
 *           sig64:      signature as 64 byte array (cannot be NULL)
 *  Out:     pubkey:     pointer to a pubkey to set to the recovered public key
 *                       (cannot be NULL).
 */
int secp256k1_schnorr_recover(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const unsigned char *sig64,
  secp256k1_pubkey_t *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Generate a nonce pair deterministically for use with
 *  secp256k1_schnorr_partial_sign.
 *  Returns: 1: valid nonce pair was generated.
 *           0: otherwise (nonce generation function failed)
 *  In:      ctx:         pointer to a context object, initialized for signing
 *                        (cannot be NULL)
 *           msg32:       the 32-byte message hash assumed to be signed (cannot
 *                        be NULL)
 *           sec32:       the 32-byte private key (cannot be NULL)
 *           noncefp:     pointer to a nonce generation function. If NULL,
 *                        secp256k1_nonce_function_default is used
 *           noncedata:   pointer to arbitrary data used by the nonce generation
 *                        function (can be NULL)
 *  Out:     pubnonce:    public side of the nonce (cannot be NULL)
 *           privnonce32: private side of the nonce (32 byte) (cannot be NULL)
 *
 *  Do not use the output as a private/public key pair for signing/validation.
 */
int secp256k1_schnorr_generate_nonce_pair(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const unsigned char *sec32,
  secp256k1_nonce_function_t noncefp,
  const void* noncedata,
  secp256k1_pubkey_t *pubnonce,
  unsigned char *privnonce32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7);

/** Produce a partial Schnorr signature, which can be combined using
 *  secp256k1_schnorr_partial_combine, to end up with a full signature that is
 *  verifiable using secp256k1_schnorr_verify.
 *  Returns: 1: signature created succesfully.
 *           0: no valid signature exists with this combination of keys, nonces
 *              and message (chance around 1 in 2^128)
 *          -1: invalid private key, nonce, or public nonces.
 *  In:   ctx:             pointer to context object, initialized for signing (cannot
 *                         be NULL)
 *        msg32:           pointer to 32-byte message to sign
 *        sec32:           pointer to 32-byte private key
 *        secnonce32:      pointer to 32-byte array containing our nonce
 *        pubnonce_others: pointer to pubkey containing the sum of the other's
 *                         nonces (see secp256k1_ec_pubkey_combine)
 *  Out:  sig64:           pointer to 64-byte array to put partial signature in
 *
 * The intended procedure for creating a multiparty signature is:
 * - Each signer S[i] with private key x[i] and public key Q[i] runs
 *   secp256k1_schnorr_generate_nonce_pair to produce a pair (k[i],R[i]) of
 *   private/public nonces.
 * - All signers communicate their public nonces to each other (revealing your
 *   private nonce can lead to discovery of your private key, so it should be
 *   considered secret).
 * - All signers combine all the public nonces they received (excluding their
 *   own) using secp256k1_ec_pubkey_combine to obtain an
 *   Rall[i] = sum(R[0..i-1,i+1..n]).
 * - All signers produce a partial signature using
 *   secp256k1_schnorr_partial_sign, passing in their own private key x[i],
 *   their own private nonce k[i], and the sum of the others' public nonces
 *   Rall[i].
 * - All signers communicate their partial signatures to each other.
 * - Someone combines all partial signatures using
 *   secp256k1_schnorr_partial_combine, to obtain a full signature.
 * - The resulting signature is validatable using secp256k1_schnorr_verify, with
 *   public key equal to the result of secp256k1_ec_pubkey_combine of the
 *   signers' public keys (sum(Q[0..n])).
 *
 *  Note that secp256k1_schnorr_partial_combine and secp256k1_ec_pubkey_combine
 *  function take their arguments in any order, and it is possible to
 *  pre-combine several inputs already with one call, and add more inputs later
 *  by calling the function again (they are commutative and associative).
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorr_partial_sign(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  unsigned char *sig64,
  const unsigned char *sec32,
  const unsigned char *secnonce32,
  const secp256k1_pubkey_t *pubnonce_others
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

/** Combine multiple Schnorr partial signatures.
 * Returns: 1: the passed signatures were succesfully combined.
 *          0: the resulting signature is not valid (chance of 1 in 2^256)
 *         -1: some inputs were invalid, or the signatures were not created
 *             using the same set of nonces
 * In:     ctx:      pointer to a context object
 *         sig64:    pointer to a 64-byte array to place the combined signature
 *                   (cannot be NULL)
 *         n:        the number of signatures to combine (at least 1)
 * Out:    sig64sin: pointer to an array of n pointers to 64-byte input
 *                   signatures
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorr_partial_combine(
  const secp256k1_context_t* ctx,
  unsigned char *sig64,
  int n,
  const unsigned char * const * sig64sin
) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

# ifdef __cplusplus
}
# endif

#endif
