#ifndef SECP256K1_FULLAGG_H
#define SECP256K1_FULLAGG_H

#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/** This module implements BIP 459 "DahLIAS fully aggregated signatures for
 *  secp256k1" (https://github.com/bitcoin/bips/blob/master/bip-0459.mediawiki),
 *  allowing multiple signers to each sign different messages and aggregate
 *  their signatures into a single signature. You can find an example
 *  demonstrating the fullagg module in examples/fullagg.c.
 */

/** Opaque data structures
 *
 *  The exact representation of data inside the opaque data structures is
 *  implementation defined and not guaranteed to be portable between different
 *  platforms or versions. With the exception of `secp256k1_fullagg_secnonce`,
 *  the data structures can be safely copied/moved. If you need to convert to a
 *  format suitable for storage, transmission, or comparison, use the
 *  corresponding serialization and parsing functions.
 */

/** Opaque data structure that holds a signer's _secret_ nonce (r1_i and r2_i).
 *
 *  Guaranteed to be 132 bytes in size.
 *
 *  WARNING: This structure MUST NOT be copied or read or written to directly.
 *  A signer who is online throughout the whole process and can keep this
 *  structure in memory can use the provided API functions for a safe standard
 *  workflow.
 *
 *  Copying this data structure can result in nonce reuse which will leak the
 *  secret signing key.
 */
typedef struct secp256k1_fullagg_secnonce {
    unsigned char data[132];
} secp256k1_fullagg_secnonce;

/** Opaque data structure that holds a signer's public nonce (R1_i and R2_i).
 *
 *  Guaranteed to be 132 bytes in size. Serialized and parsed with
 *  `fullagg_pubnonce_serialize` and `fullagg_pubnonce_parse`.
 */
typedef struct secp256k1_fullagg_pubnonce {
    unsigned char data[132];
} secp256k1_fullagg_pubnonce;

/** Opaque data structure that holds an aggregate public nonce (aggregated R1 and R2).
 *
 *  Guaranteed to be 132 bytes in size. Serialized and parsed with
 *  `fullagg_aggnonce_serialize` and `fullagg_aggnonce_parse`.
 */
typedef struct secp256k1_fullagg_aggnonce {
    unsigned char data[132];
} secp256k1_fullagg_aggnonce;

/** Opaque data structure that holds a FullAgg session.
 *
 *  This structure contains the computed values needed for signing:
 *  the final nonce R, the nonce coefficient b, the aggregate nonce, and the
 *  number of signers.
 *
 *  Guaranteed to be 143 bytes in size.
 */
typedef struct secp256k1_fullagg_session {
    unsigned char data[143];
} secp256k1_fullagg_session;

/** Opaque data structure that holds a partial FullAgg signature (s_i).
 *
 *  Guaranteed to be 36 bytes in size. Serialized and parsed with
 *  `fullagg_partial_sig_serialize` and `fullagg_partial_sig_parse`.
 */
typedef struct secp256k1_fullagg_partial_sig {
    unsigned char data[36];
} secp256k1_fullagg_partial_sig;

/** Parse a signer's public nonce.
 *
 *  Returns: 1 when the nonce could be parsed, 0 otherwise.
 *  Args:    ctx: pointer to a context object
 *  Out:   nonce: pointer to a nonce object
 *  In:     in66: pointer to the 66-byte nonce to be parsed
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_pubnonce_parse(
    const secp256k1_context *ctx,
    secp256k1_fullagg_pubnonce *nonce,
    const unsigned char *in66
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a signer's public nonce
 *
 *  Returns: 1 always
 *  Args:    ctx: pointer to a context object
 *  Out:   out66: pointer to a 66-byte array to store the serialized nonce
 *  In:    nonce: pointer to the nonce
 */
SECP256K1_API int secp256k1_fullagg_pubnonce_serialize(
    const secp256k1_context *ctx,
    unsigned char *out66,
    const secp256k1_fullagg_pubnonce *nonce
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse an aggregate public nonce.
 *
 *  Returns: 1 when the nonce could be parsed, 0 otherwise.
 *  Args:    ctx: pointer to a context object
 *  Out:   nonce: pointer to a nonce object
 *  In:     in66: pointer to the 66-byte nonce to be parsed
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_aggnonce_parse(
    const secp256k1_context *ctx,
    secp256k1_fullagg_aggnonce *nonce,
    const unsigned char *in66
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize an aggregate public nonce
 *
 *  Returns: 1 always
 *  Args:    ctx: pointer to a context object
 *  Out:   out66: pointer to a 66-byte array to store the serialized nonce
 *  In:    nonce: pointer to the nonce
 */
SECP256K1_API int secp256k1_fullagg_aggnonce_serialize(
    const secp256k1_context *ctx,
    unsigned char *out66,
    const secp256k1_fullagg_aggnonce *nonce
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse a FullAgg partial signature.
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args:    ctx: pointer to a context object
 *  Out:     sig: pointer to a signature object
 *  In:     in32: pointer to the 32-byte signature to be parsed
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_partial_sig_parse(
    const secp256k1_context *ctx,
    secp256k1_fullagg_partial_sig *sig,
    const unsigned char *in32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a FullAgg partial signature
 *
 *  Returns: 1 always
 *  Args:    ctx: pointer to a context object
 *  Out:   out32: pointer to a 32-byte array to store the serialized signature
 *  In:      sig: pointer to the signature
 */
SECP256K1_API int secp256k1_fullagg_partial_sig_serialize(
    const secp256k1_context *ctx,
    unsigned char *out32,
    const secp256k1_fullagg_partial_sig *sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Starts a signing session by generating a nonce
 *
 *  This function outputs a secret nonce that will be required for signing and a
 *  corresponding public nonce that is intended to be sent to the coordinator.
 *
 *  FullAgg differs from regular signing in that implementers _must_ take
 *  special care to not reuse a nonce. This can be ensured by following these rules:
 *
 *  1. Each call to this function must have a UNIQUE session_secrand32 that must
 *     NOT BE REUSED in subsequent calls to this function and must be KEPT
 *     SECRET (even from other signers).
 *  2. If you already know the seckey, it can be optionally provided to derive
 *     the nonce and increase misuse-resistance. The extra_input32 argument can
 *     be used to provide additional data that does not repeat in normal
 *     scenarios, such as the current time.
 *  3. Avoid copying (or serializing) the secnonce. This reduces the possibility
 *     that it is used more than once for signing.
 *
 *  If you don't have access to good randomness for session_secrand32, but you
 *  have access to a non-repeating counter, then see
 *  secp256k1_fullagg_nonce_gen_counter.
 *
 *  Remember that nonce reuse will leak the secret key!
 *  Note that using the same seckey for multiple FullAgg sessions is fine.
 *
 *  Returns: 0 if the arguments are invalid and 1 otherwise
 *  Args:         ctx: pointer to a context object (not secp256k1_context_static)
 *  Out:     secnonce: pointer to a structure to store the secret nonce (r1_i, r2_i)
 *           pubnonce: pointer to a structure to store the public nonce (R1_i, R2_i)
 *  In/Out:
 *  session_secrand32: a 32-byte session_secrand32 as explained above. Must be unique to
 *                     this call to secp256k1_fullagg_nonce_gen and must be
 *                     uniformly random. If the function call is successful, the
 *                     session_secrand32 buffer is invalidated to prevent reuse.
 *  In:
 *             seckey: the 32-byte secret key that will later be used for signing, if
 *                     already known (can be NULL)
 *             pubkey: x-only public key of the signer creating the nonce. The
 *                     secnonce output of this function cannot be used to sign
 *                     for any other public key. While the public key should
 *                     correspond to the provided seckey, a mismatch will not
 *                     cause the function to return 0.
 *      extra_input32: an optional 32-byte array that is input to the nonce
 *                     derivation function (can be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_nonce_gen(
    const secp256k1_context *ctx,
    secp256k1_fullagg_secnonce *secnonce,
    secp256k1_fullagg_pubnonce *pubnonce,
    unsigned char *session_secrand32,
    const unsigned char *seckey,
    const secp256k1_xonly_pubkey *pubkey,
    const unsigned char *extra_input32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(6);

/** Alternative way to generate a nonce and start a signing session
 *
 *  This function outputs a secret nonce that will be required for signing and a
 *  corresponding public nonce that is intended to be sent to the coordinator.
 *
 *  This function differs from `secp256k1_fullagg_nonce_gen` by accepting a
 *  non-repeating counter value instead of a secret random value. This requires
 *  that a secret key is provided to `secp256k1_fullagg_nonce_gen_counter`
 *  (through the keypair argument).
 *
 *  Returns: 0 if the arguments are invalid and 1 otherwise
 *  Args:         ctx: pointer to a context object (not secp256k1_context_static)
 *  Out:     secnonce: pointer to a structure to store the secret nonce
 *           pubnonce: pointer to a structure to store the public nonce
 *  In:
 *   nonrepeating_cnt: the value of a counter as explained above. Must be
 *                     unique to this call to secp256k1_fullagg_nonce_gen_counter.
 *            keypair: keypair of the signer creating the nonce. The secnonce
 *                     output of this function cannot be used to sign for any
 *                     other keypair.
 *      extra_input32: an optional 32-byte array that is input to the nonce
 *                     derivation function (can be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_nonce_gen_counter(
    const secp256k1_context *ctx,
    secp256k1_fullagg_secnonce *secnonce,
    secp256k1_fullagg_pubnonce *pubnonce,
    uint64_t nonrepeating_cnt,
    const secp256k1_keypair *keypair,
    const unsigned char *extra_input32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

/** Aggregates the nonces of all signers into a single nonce
 *
 *  This is done by the coordinator to compute the aggregate nonces R1 and R2
 *  from all signers' individual nonces R1_i and R2_i.
 *
 *  If the aggregator does not compute the aggregate nonce correctly, the final
 *  signature will be invalid.
 *
 *  Returns: 0 if the arguments are invalid or if an aggregate nonce is the
 *           point at infinity, 1 otherwise. In the (cryptographically
 *           unreachable for honest signers) failure case of an aggregate nonce
 *           at infinity, the signers should restart the session with fresh
 *           nonces.
 *  Args:           ctx: pointer to a context object
 *  Out:       aggnonce: pointer to an aggregate public nonce object for
 *                       fullagg_session_init (contains R1 and R2)
 *  In:       pubnonces: array of pointers to public nonces sent by the
 *                       signers (each containing R1_i and R2_i)
 *          n_pubnonces: number of elements in the pubnonces array. Must be
 *                       greater than 0.
 */
SECP256K1_API int secp256k1_fullagg_nonce_agg(
    const secp256k1_context *ctx,
    secp256k1_fullagg_aggnonce *aggnonce,
    const secp256k1_fullagg_pubnonce * const *pubnonces,
    size_t n_pubnonces
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Initialize a FullAgg signing session
 *
 *  Creates a session context that contains the computed values needed for
 *  signing and verification of partial signatures. This includes the final
 *  nonce R and the nonce coefficient b. The arrays of public keys, messages,
 *  and nonces must be provided again when signing or verifying.
 *
 *  Returns: 0 if the arguments are invalid or if the final nonce is the point
 *           at infinity, 1 otherwise. In the (cryptographically unreachable
 *           for honest signers) failure case of a final nonce at infinity, the
 *           signers should restart the session with fresh nonces.
 *  Args:          ctx: pointer to a context object
 *  Out:       session: pointer to a struct to store the session
 *  In:       aggnonce: pointer to an aggregate public nonce object that is the
 *                      output of fullagg_nonce_agg
 *             pubkeys: array of pointers to the x-only public keys of all
 *                      signers. The order must be consistent across all
 *                      signers and the coordinator.
 *            messages: array of pointers to 32-byte messages, where messages[i]
 *                      is the message to be signed by the signer with pubkeys[i]
 *           pubnonces: array of pointers to public nonces, where pubnonces[i]
 *                      contains the R2_i value from the signer with pubkeys[i].
 *           n_signers: number of signers (length of pubkeys, messages, and
 *                      pubnonces arrays). Must be greater than 0.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_session_init(
    const secp256k1_context *ctx,
    secp256k1_fullagg_session *session,
    const secp256k1_fullagg_aggnonce *aggnonce,
    const secp256k1_xonly_pubkey * const *pubkeys,
    const unsigned char * const *messages,
    const secp256k1_fullagg_pubnonce * const *pubnonces,
    size_t n_signers
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

/** Produces a partial signature
 *
 *  This function overwrites the given secnonce with zeros and will abort if given a
 *  secnonce that is all zeros. This is a best effort attempt to protect against nonce
 *  reuse. However, this is of course easily defeated if the secnonce has been
 *  copied (or serialized). Remember that nonce reuse will leak the secret key!
 *
 *  For signing to succeed, the secnonce provided to this function must have
 *  been generated for the provided keypair.
 *
 *  This function checks that the signer's public key and intended message
 *  appear at signer_index in the arrays and that the signer's R2_i nonce
 *  appears exactly once in the pubnonces array, at signer_index. It also
 *  verifies that the session was initialized with the same lists of public
 *  keys, messages, and public nonces by recomputing the nonce coefficient.
 *
 *  Returns: 0 if the arguments are invalid or the provided secnonce has already
 *           been used for signing, 1 otherwise
 *  Args:         ctx: pointer to a context object (not secp256k1_context_static)
 *  Out:  partial_sig: pointer to struct to store the partial signature
 *  In/Out:  secnonce: pointer to the secnonce struct created in
 *                     fullagg_nonce_gen that has never been used in a
 *                     partial_sign call before
 *  In:       keypair: pointer to keypair to sign the message with
 *              msg32: the 32-byte message that the signer intends to sign. It
 *                     must match the message at signer_index in the messages
 *                     array.
 *            session: pointer to the session that was created with
 *                     fullagg_session_init
 *            pubkeys: array of pointers to x-only public keys (same as in
 *                     session_init)
 *           messages: array of pointers to messages (same as in session_init)
 *          pubnonces: array of pointers to public nonces (same as in session_init)
 *          n_signers: number of signers (length of the pubkeys, messages, and
 *                     pubnonces arrays). Must be greater than 0 and equal to
 *                     the number of signers in the session.
 *       signer_index: the index of this signer in the arrays (0-indexed)
 */
SECP256K1_API int secp256k1_fullagg_partial_sign(
    const secp256k1_context *ctx,
    secp256k1_fullagg_partial_sig *partial_sig,
    secp256k1_fullagg_secnonce *secnonce,
    const secp256k1_keypair *keypair,
    const unsigned char *msg32,
    const secp256k1_fullagg_session *session,
    const secp256k1_xonly_pubkey * const *pubkeys,
    const unsigned char * const *messages,
    const secp256k1_fullagg_pubnonce * const *pubnonces,
    size_t n_signers,
    size_t signer_index
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7) SECP256K1_ARG_NONNULL(8) SECP256K1_ARG_NONNULL(9);

/** Verifies an individual signer's partial signature
 *
 *  The aggregate nonce and the session values are recomputed from the
 *  pubnonces, so the result cannot be influenced by the coordinator. If the
 *  public nonces are received over authenticated channels and this function
 *  succeeds for the partial signatures of all signers, then partial_sig_agg
 *  produces a valid aggregate signature.
 *
 *  It is not required to call this function in regular FullAgg sessions,
 *  because if any partial signature does not verify, the final signature
 *  will not verify either. However, this function provides the ability to
 *  identify which specific partial signature fails verification.
 *
 *  Returns: 0 if the arguments are invalid or the partial signature does not
 *           verify, 1 otherwise
 *  Args         ctx: pointer to a context object
 *  In:  partial_sig: pointer to partial signature to verify
 *         pubnonces: array of pointers to public nonces (same as in
 *                    session_init)
 *           pubkeys: array of pointers to x-only public keys (same as in
 *                    session_init)
 *          messages: array of pointers to messages (same as in session_init)
 *         n_signers: number of signers (length of pubnonces, pubkeys, and
 *                    messages arrays). Must be greater than 0.
 *      signer_index: the index of the signer being verified in the arrays
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_partial_sig_verify(
    const secp256k1_context *ctx,
    const secp256k1_fullagg_partial_sig *partial_sig,
    const secp256k1_fullagg_pubnonce * const *pubnonces,
    const secp256k1_xonly_pubkey * const *pubkeys,
    const unsigned char * const *messages,
    size_t n_signers,
    size_t signer_index
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Aggregates partial signatures
 *
 *  Produces the final aggregated signature from all partial signatures.
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise (which does NOT mean
 *           the resulting signature verifies).
 *  Args:         ctx: pointer to a context object
 *  Out:        sig64: complete (but possibly invalid) aggregate signature
 *  In:       session: pointer to the session that was created with
 *                     fullagg_session_init
 *       partial_sigs: array of pointers to partial signatures to aggregate
 *             n_sigs: number of elements in the partial_sigs array. Must be
 *                     greater than 0 and equal to the number of signers.
 */
SECP256K1_API int secp256k1_fullagg_partial_sig_agg(
    const secp256k1_context *ctx,
    unsigned char *sig64,
    const secp256k1_fullagg_session *session,
    const secp256k1_fullagg_partial_sig * const *partial_sigs,
    size_t n_sigs
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verify a FullAgg aggregate signature
 *
 *  Verifies that the signature is valid for the given list of public keys
 *  and their corresponding messages.
 *
 *  Returns: 1 if the signature is valid, 0 otherwise
 *  Args:        ctx: pointer to a context object
 *  In:        sig64: pointer to the 64-byte signature to verify
 *           pubkeys: array of pointers to x-only public keys
 *          messages: array of pointers to 32-byte messages, where messages[i]
 *                    corresponds to pubkeys[i]
 *         n_signers: number of signers (length of pubkeys and messages arrays)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_fullagg_verify(
    const secp256k1_context *ctx,
    const unsigned char *sig64,
    const secp256k1_xonly_pubkey * const *pubkeys,
    const unsigned char * const *messages,
    size_t n_signers
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

#ifdef __cplusplus
}
#endif

#endif
