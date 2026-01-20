#ifndef SECP256K1_MUSIG_H
#define SECP256K1_MUSIG_H

#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/** This module implements BIP 327 "MuSig2 for BIP340-compatible
 *  Multi-Signatures"
 *  (https://github.com/bitcoin/bips/blob/master/bip-0327.mediawiki)
 *  v1.0.0. You can find an example demonstrating the musig module in
 *  examples/musig.c.
 *
 *  The module also supports BIP 341 ("Taproot") public key tweaking.
 *
 *  It is recommended to read the documentation in this include file carefully.
 *  Further notes on API usage can be found in doc/musig.md
 *
 *  Since the first version of MuSig is essentially replaced by MuSig2, we use
 *  MuSig, musig and MuSig2 synonymously unless noted otherwise.
 */

/** Opaque data structures
 *
 *  The exact representation of data inside the opaque data structures is
 *  implementation defined and not guaranteed to be portable between different
 *  platforms or versions. With the exception of `secp256k1_musig_secnonce`, the
 *  data structures can be safely copied/moved. If you need to convert to a
 *  format suitable for storage, transmission, or comparison, use the
 *  corresponding serialization and parsing functions.
 */

/** Opaque data structure that caches information about public key aggregation.
 *
 *  Guaranteed to be 197 bytes in size. No serialization and parsing functions
 *  (yet).
 */
typedef struct secp256k1_musig_keyagg_cache {
    unsigned char data[197];
} secp256k1_musig_keyagg_cache;

/** Opaque data structure that holds a signer's _secret_ nonce.
 *
 *  Guaranteed to be 132 bytes in size.
 *
 *  WARNING: This structure MUST NOT be copied or read or written to directly. A
 *  signer who is online throughout the whole process and can keep this
 *  structure in memory can use the provided API functions for a safe standard
 *  workflow.
 *
 *  Copying this data structure can result in nonce reuse which will leak the
 *  secret signing key.
 */
typedef struct secp256k1_musig_secnonce {
    unsigned char data[132];
} secp256k1_musig_secnonce;

/** Opaque data structure that holds a signer's public nonce.
 *
 *  Guaranteed to be 132 bytes in size. Serialized and parsed with
 *  `musig_pubnonce_serialize` and `musig_pubnonce_parse`.
 */
typedef struct secp256k1_musig_pubnonce {
    unsigned char data[132];
} secp256k1_musig_pubnonce;

/** Opaque data structure that holds an aggregate public nonce.
 *
 *  Guaranteed to be 132 bytes in size. Serialized and parsed with
 *  `musig_aggnonce_serialize` and `musig_aggnonce_parse`.
 */
typedef struct secp256k1_musig_aggnonce {
    unsigned char data[132];
} secp256k1_musig_aggnonce;

/** Opaque data structure that holds a MuSig session.
 *
 *  This structure is not required to be kept secret for the signing protocol to
 *  be secure. Guaranteed to be 133 bytes in size. No serialization and parsing
 *  functions (yet).
 */
typedef struct secp256k1_musig_session {
    unsigned char data[133];
} secp256k1_musig_session;

/** Opaque data structure that holds a partial MuSig signature.
 *
 *  Guaranteed to be 36 bytes in size. Serialized and parsed with
 *  `musig_partial_sig_serialize` and `musig_partial_sig_parse`.
 */
typedef struct secp256k1_musig_partial_sig {
    unsigned char data[36];
} secp256k1_musig_partial_sig;

/** Parse a signer's public nonce.
 *
 *  Returns: 1 when the nonce could be parsed, 0 otherwise.
 *  Args:    ctx: pointer to a context object
 *  Out:   nonce: pointer to a nonce object
 *  In:     in66: pointer to the 66-byte nonce to be parsed
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_pubnonce_parse(
    const secp256k1_context *ctx,
    secp256k1_musig_pubnonce *nonce,
    const unsigned char *in66
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a signer's public nonce
 *
 *  Returns: 1 always
 *  Args:    ctx: pointer to a context object
 *  Out:   out66: pointer to a 66-byte array to store the serialized nonce
 *  In:    nonce: pointer to the nonce
 */
SECP256K1_API int secp256k1_musig_pubnonce_serialize(
    const secp256k1_context *ctx,
    unsigned char *out66,
    const secp256k1_musig_pubnonce *nonce
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse an aggregate public nonce.
 *
 *  Returns: 1 when the nonce could be parsed, 0 otherwise.
 *  Args:    ctx: pointer to a context object
 *  Out:   nonce: pointer to a nonce object
 *  In:     in66: pointer to the 66-byte nonce to be parsed
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_aggnonce_parse(
    const secp256k1_context *ctx,
    secp256k1_musig_aggnonce *nonce,
    const unsigned char *in66
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize an aggregate public nonce
 *
 *  Returns: 1 always
 *  Args:    ctx: pointer to a context object
 *  Out:   out66: pointer to a 66-byte array to store the serialized nonce
 *  In:    nonce: pointer to the nonce
 */
SECP256K1_API int secp256k1_musig_aggnonce_serialize(
    const secp256k1_context *ctx,
    unsigned char *out66,
    const secp256k1_musig_aggnonce *nonce
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse a MuSig partial signature.
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args:    ctx: pointer to a context object
 *  Out:     sig: pointer to a signature object
 *  In:     in32: pointer to the 32-byte signature to be parsed
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_partial_sig_parse(
    const secp256k1_context *ctx,
    secp256k1_musig_partial_sig *sig,
    const unsigned char *in32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a MuSig partial signature
 *
 *  Returns: 1 always
 *  Args:    ctx: pointer to a context object
 *  Out:   out32: pointer to a 32-byte array to store the serialized signature
 *  In:      sig: pointer to the signature
 */
SECP256K1_API int secp256k1_musig_partial_sig_serialize(
    const secp256k1_context *ctx,
    unsigned char *out32,
    const secp256k1_musig_partial_sig *sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Computes an aggregate public key and uses it to initialize a keyagg_cache
 *
 *  Different orders of `pubkeys` result in different `agg_pk`s.
 *
 *  Before aggregating, the pubkeys can be sorted with `secp256k1_ec_pubkey_sort`
 *  which ensures the same `agg_pk` result for the same multiset of pubkeys.
 *  This is useful to do before `pubkey_agg`, such that the order of pubkeys
 *  does not affect the aggregate public key.
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise
 *  Args:        ctx: pointer to a context object
 *  Out:      agg_pk: the MuSig-aggregated x-only public key. If you do not need it,
 *                    this arg can be NULL.
 *      keyagg_cache: if non-NULL, pointer to a musig_keyagg_cache struct that
 *                    is required for signing (or observing the signing session
 *                    and verifying partial signatures).
 *   In:     pubkeys: input array of pointers to public keys to aggregate. The order
 *                    is important; a different order will result in a different
 *                    aggregate public key.
 *         n_pubkeys: length of pubkeys array. Must be greater than 0.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_pubkey_agg(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *agg_pk,
    secp256k1_musig_keyagg_cache *keyagg_cache,
    const secp256k1_pubkey * const *pubkeys,
    size_t n_pubkeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(4);

/** Obtain the aggregate public key from a keyagg_cache.
 *
 *  This is only useful if you need the non-xonly public key, in particular for
 *  plain (non-xonly) tweaking or batch-verifying multiple key aggregations
 *  (not implemented).
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise
 *  Args:        ctx: pointer to a context object
 *  Out:      agg_pk: the MuSig-aggregated public key.
 *  In: keyagg_cache: pointer to a `musig_keyagg_cache` struct initialized by
 *                    `musig_pubkey_agg`
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_pubkey_get(
    const secp256k1_context *ctx,
    secp256k1_pubkey *agg_pk,
    const secp256k1_musig_keyagg_cache *keyagg_cache
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Apply plain "EC" tweaking to a public key in a given keyagg_cache by adding
 *  the generator multiplied with `tweak32` to it. This is useful for deriving
 *  child keys from an aggregate public key via BIP 32 where `tweak32` is set to
 *  a hash as defined in BIP 32.
 *
 *  Callers are responsible for deriving `tweak32` in a way that does not reduce
 *  the security of MuSig (for example, by following BIP 32).
 *
 *  The tweaking method is the same as `secp256k1_ec_pubkey_tweak_add`. So after
 *  the following pseudocode buf and buf2 have identical contents (absent
 *  earlier failures).
 *
 *  secp256k1_musig_pubkey_agg(..., keyagg_cache, pubkeys, ...)
 *  secp256k1_musig_pubkey_get(..., agg_pk, keyagg_cache)
 *  secp256k1_musig_pubkey_ec_tweak_add(..., output_pk, tweak32, keyagg_cache)
 *  secp256k1_ec_pubkey_serialize(..., buf, ..., output_pk, ...)
 *  secp256k1_ec_pubkey_tweak_add(..., agg_pk, tweak32)
 *  secp256k1_ec_pubkey_serialize(..., buf2, ..., agg_pk, ...)
 *
 *  This function is required if you want to _sign_ for a tweaked aggregate key.
 *  If you are only computing a public key but not intending to create a
 *  signature for it, use `secp256k1_ec_pubkey_tweak_add` instead.
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise
 *  Args:            ctx: pointer to a context object
 *  Out:   output_pubkey: pointer to a public key to store the result. Will be set
 *                        to an invalid value if this function returns 0. If you
 *                        do not need it, this arg can be NULL.
 *  In/Out: keyagg_cache: pointer to a `musig_keyagg_cache` struct initialized by
 *                       `musig_pubkey_agg`
 *  In:          tweak32: pointer to a 32-byte tweak. The tweak is valid if it passes
 *                        `secp256k1_ec_seckey_verify` and is not equal to the
 *                        secret key corresponding to the public key represented
 *                        by keyagg_cache or its negation. For uniformly random
 *                        32-byte arrays the chance of being invalid is
 *                        negligible (around 1 in 2^128).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_pubkey_ec_tweak_add(
    const secp256k1_context *ctx,
    secp256k1_pubkey *output_pubkey,
    secp256k1_musig_keyagg_cache *keyagg_cache,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Apply x-only tweaking to a public key in a given keyagg_cache by adding the
 *  generator multiplied with `tweak32` to it. This is useful for creating
 *  Taproot outputs where `tweak32` is set to a TapTweak hash as defined in BIP
 *  341.
 *
 *  Callers are responsible for deriving `tweak32` in a way that does not reduce
 *  the security of MuSig (for example, by following Taproot BIP 341).
 *
 *  The tweaking method is the same as `secp256k1_xonly_pubkey_tweak_add`. So in
 *  the following pseudocode xonly_pubkey_tweak_add_check (absent earlier
 *  failures) returns 1.
 *
 *  secp256k1_musig_pubkey_agg(..., agg_pk, keyagg_cache, pubkeys, ...)
 *  secp256k1_musig_pubkey_xonly_tweak_add(..., output_pk, keyagg_cache, tweak32)
 *  secp256k1_xonly_pubkey_serialize(..., buf, output_pk)
 *  secp256k1_xonly_pubkey_tweak_add_check(..., buf, ..., agg_pk, tweak32)
 *
 *  This function is required if you want to _sign_ for a tweaked aggregate key.
 *  If you are only computing a public key but not intending to create a
 *  signature for it, use `secp256k1_xonly_pubkey_tweak_add` instead.
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise
 *  Args:            ctx: pointer to a context object
 *  Out:   output_pubkey: pointer to a public key to store the result. Will be set
 *                        to an invalid value if this function returns 0. If you
 *                        do not need it, this arg can be NULL.
 *  In/Out: keyagg_cache: pointer to a `musig_keyagg_cache` struct initialized by
 *                       `musig_pubkey_agg`
 *  In:          tweak32: pointer to a 32-byte tweak. The tweak is valid if it passes
 *                        `secp256k1_ec_seckey_verify` and is not equal to the
 *                        secret key corresponding to the public key represented
 *                        by keyagg_cache or its negation. For uniformly random
 *                        32-byte arrays the chance of being invalid is
 *                        negligible (around 1 in 2^128).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_pubkey_xonly_tweak_add(
    const secp256k1_context *ctx,
    secp256k1_pubkey *output_pubkey,
    secp256k1_musig_keyagg_cache *keyagg_cache,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Starts a signing session by generating a nonce
 *
 *  This function outputs a secret nonce that will be required for signing and a
 *  corresponding public nonce that is intended to be sent to other signers.
 *
 *  MuSig differs from regular Schnorr signing in that implementers _must_ take
 *  special care to not reuse a nonce. This can be ensured by following these rules:
 *
 *  1. Each call to this function must have a UNIQUE session_secrand32 that must
 *     NOT BE REUSED in subsequent calls to this function and must be KEPT
 *     SECRET (even from other signers).
 *  2. If you already know the seckey, message or aggregate public key
 *     cache, they can be optionally provided to derive the nonce and increase
 *     misuse-resistance. The extra_input32 argument can be used to provide
 *     additional data that does not repeat in normal scenarios, such as the
 *     current time.
 *  3. Avoid copying (or serializing) the secnonce. This reduces the possibility
 *     that it is used more than once for signing.
 *
 *  If you don't have access to good randomness for session_secrand32, but you
 *  have access to a non-repeating counter, then see
 *  secp256k1_musig_nonce_gen_counter.
 *
 *  Remember that nonce reuse will leak the secret key!
 *  Note that using the same seckey for multiple MuSig sessions is fine.
 *
 *  Returns: 0 if the arguments are invalid and 1 otherwise
 *  Args:         ctx: pointer to a context object (not secp256k1_context_static)
 *  Out:     secnonce: pointer to a structure to store the secret nonce
 *           pubnonce: pointer to a structure to store the public nonce
 *  In/Out:
 *  session_secrand32: a 32-byte session_secrand32 as explained above. Must be unique to this
 *                     call to secp256k1_musig_nonce_gen and must be uniformly
 *                     random. If the function call is successful, the
 *                     session_secrand32 buffer is invalidated to prevent reuse.
 *  In:
 *             seckey: the 32-byte secret key that will later be used for signing, if
 *                     already known (can be NULL)
 *             pubkey: public key of the signer creating the nonce. The secnonce
 *                     output of this function cannot be used to sign for any
 *                     other public key. While the public key should correspond
 *                     to the provided seckey, a mismatch will not cause the
 *                     function to return 0.
 *              msg32: the 32-byte message that will later be signed, if already known
 *                     (can be NULL)
 *       keyagg_cache: pointer to the keyagg_cache that was used to create the aggregate
 *                     (and potentially tweaked) public key if already known
 *                     (can be NULL)
 *      extra_input32: an optional 32-byte array that is input to the nonce
 *                     derivation function (can be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_nonce_gen(
    const secp256k1_context *ctx,
    secp256k1_musig_secnonce *secnonce,
    secp256k1_musig_pubnonce *pubnonce,
    unsigned char *session_secrand32,
    const unsigned char *seckey,
    const secp256k1_pubkey *pubkey,
    const unsigned char *msg32,
    const secp256k1_musig_keyagg_cache *keyagg_cache,
    const unsigned char *extra_input32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(6);


/** Alternative way to generate a nonce and start a signing session
 *
 *  This function outputs a secret nonce that will be required for signing and a
 *  corresponding public nonce that is intended to be sent to other signers.
 *
 *  This function differs from `secp256k1_musig_nonce_gen` by accepting a
 *  non-repeating counter value instead of a secret random value. This requires
 *  that a secret key is provided to `secp256k1_musig_nonce_gen_counter`
 *  (through the keypair argument), as opposed to `secp256k1_musig_nonce_gen`
 *  where the seckey argument is optional.
 *
 *  MuSig differs from regular Schnorr signing in that implementers _must_ take
 *  special care to not reuse a nonce. This can be ensured by following these rules:
 *
 *  1. The nonrepeating_cnt argument must be a counter value that never repeats,
 *     i.e., you must never call `secp256k1_musig_nonce_gen_counter` twice with
 *     the same keypair and nonrepeating_cnt value. For example, this implies
 *     that if the same keypair is used with `secp256k1_musig_nonce_gen_counter`
 *     on multiple devices, none of the devices should have the same counter
 *     value as any other device.
 *  2. If the seckey, message or aggregate public key cache is already available
 *     at this stage, any of these can be optionally provided, in which case
 *     they will be used in the derivation of the nonce and increase
 *     misuse-resistance. The extra_input32 argument can be used to provide
 *     additional data that does not repeat in normal scenarios, such as the
 *     current time.
 *  3. Avoid copying (or serializing) the secnonce. This reduces the possibility
 *     that it is used more than once for signing.
 *
 *  Remember that nonce reuse will leak the secret key!
 *  Note that using the same keypair for multiple MuSig sessions is fine.
 *
 *  Returns: 0 if the arguments are invalid and 1 otherwise
 *  Args:         ctx: pointer to a context object (not secp256k1_context_static)
 *  Out:     secnonce: pointer to a structure to store the secret nonce
 *           pubnonce: pointer to a structure to store the public nonce
 *  In:
 *   nonrepeating_cnt: the value of a counter as explained above. Must be
 *                     unique to this call to secp256k1_musig_nonce_gen.
 *            keypair: keypair of the signer creating the nonce. The secnonce
 *                     output of this function cannot be used to sign for any
 *                     other keypair.
 *              msg32: the 32-byte message that will later be signed, if already known
 *                     (can be NULL)
 *       keyagg_cache: pointer to the keyagg_cache that was used to create the aggregate
 *                     (and potentially tweaked) public key if already known
 *                     (can be NULL)
 *      extra_input32: an optional 32-byte array that is input to the nonce
 *                     derivation function (can be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_nonce_gen_counter(
    const secp256k1_context *ctx,
    secp256k1_musig_secnonce *secnonce,
    secp256k1_musig_pubnonce *pubnonce,
    uint64_t nonrepeating_cnt,
    const secp256k1_keypair *keypair,
    const unsigned char *msg32,
    const secp256k1_musig_keyagg_cache *keyagg_cache,
    const unsigned char *extra_input32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

/** Aggregates the nonces of all signers into a single nonce
 *
 *  This can be done by an untrusted party to reduce the communication
 *  between signers. Instead of everyone sending nonces to everyone else, there
 *  can be one party receiving all nonces, aggregating the nonces with this
 *  function and then sending only the aggregate nonce back to the signers.
 *
 *  If the aggregator does not compute the aggregate nonce correctly, the final
 *  signature will be invalid.
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise
 *  Args:           ctx: pointer to a context object
 *  Out:       aggnonce: pointer to an aggregate public nonce object for
 *                       musig_nonce_process
 *  In:       pubnonces: array of pointers to public nonces sent by the
 *                       signers
 *          n_pubnonces: number of elements in the pubnonces array. Must be
 *                       greater than 0.
 */
SECP256K1_API int secp256k1_musig_nonce_agg(
    const secp256k1_context *ctx,
    secp256k1_musig_aggnonce *aggnonce,
    const secp256k1_musig_pubnonce * const *pubnonces,
    size_t n_pubnonces
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Takes the aggregate nonce and creates a session that is required for signing
 *  and verification of partial signatures.
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise
 *  Args:          ctx: pointer to a context object
 *  Out:       session: pointer to a struct to store the session
 *  In:       aggnonce: pointer to an aggregate public nonce object that is the
 *                      output of musig_nonce_agg
 *              msg32:  the 32-byte message to sign
 *       keyagg_cache:  pointer to the keyagg_cache that was used to create the
 *                      aggregate (and potentially tweaked) pubkey
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_nonce_process(
    const secp256k1_context *ctx,
    secp256k1_musig_session *session,
    const secp256k1_musig_aggnonce *aggnonce,
    const unsigned char *msg32,
    const secp256k1_musig_keyagg_cache *keyagg_cache
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Produces a partial signature
 *
 *  This function overwrites the given secnonce with zeros and will abort if given a
 *  secnonce that is all zeros. This is a best effort attempt to protect against nonce
 *  reuse. However, this is of course easily defeated if the secnonce has been
 *  copied (or serialized). Remember that nonce reuse will leak the secret key!
 *
 *  For signing to succeed, the secnonce provided to this function must have
 *  been generated for the provided keypair. This means that when signing for a
 *  keypair consisting of a seckey and pubkey, the secnonce must have been
 *  created by calling musig_nonce_gen with that pubkey. Otherwise, the
 *  illegal_callback is called.
 *
 *  This function does not verify the output partial signature, deviating from
 *  the BIP 327 specification. It is recommended to verify the output partial
 *  signature with `secp256k1_musig_partial_sig_verify` to prevent random or
 *  adversarially provoked computation errors.
 *
 *  Returns: 0 if the arguments are invalid or the provided secnonce has already
 *           been used for signing, 1 otherwise
 *  Args:         ctx: pointer to a context object
 *  Out:  partial_sig: pointer to struct to store the partial signature
 *  In/Out:  secnonce: pointer to the secnonce struct created in
 *                     musig_nonce_gen that has been never used in a
 *                     partial_sign call before and has been created for the
 *                     keypair
 *  In:       keypair: pointer to keypair to sign the message with
 *       keyagg_cache: pointer to the keyagg_cache that was output when the
 *                     aggregate public key for this session
 *            session: pointer to the session that was created with
 *                     musig_nonce_process
 */
SECP256K1_API int secp256k1_musig_partial_sign(
    const secp256k1_context *ctx,
    secp256k1_musig_partial_sig *partial_sig,
    secp256k1_musig_secnonce *secnonce,
    const secp256k1_keypair *keypair,
    const secp256k1_musig_keyagg_cache *keyagg_cache,
    const secp256k1_musig_session *session
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

/** Verifies an individual signer's partial signature
 *
 *  The signature is verified for a specific signing session. In order to avoid
 *  accidentally verifying a signature from a different or non-existing signing
 *  session, you must ensure the following:
 *    1. The `keyagg_cache` argument is identical to the one used to create the
 *       `session` with `musig_nonce_process`.
 *    2. The `pubkey` argument must be identical to the one sent by the signer
 *       before aggregating it with `musig_pubkey_agg` to create the
 *       `keyagg_cache`.
 *    3. The `pubnonce` argument must be identical to the one sent by the signer
 *       before aggregating it with `musig_nonce_agg` and using the result to
 *       create the `session` with `musig_nonce_process`.
 *
 *  It is not required to call this function in regular MuSig sessions, because
 *  if any partial signature does not verify, the final signature will not
 *  verify either, so the problem will be caught. However, this function
 *  provides the ability to identify which specific partial signature fails
 *  verification.
 *
 *  Returns: 0 if the arguments are invalid or the partial signature does not
 *           verify, 1 otherwise
 *  Args         ctx: pointer to a context object
 *  In:  partial_sig: pointer to partial signature to verify, sent by
 *                    the signer associated with `pubnonce` and `pubkey`
 *          pubnonce: public nonce of the signer in the signing session
 *            pubkey: public key of the signer in the signing session
 *      keyagg_cache: pointer to the keyagg_cache that was output when the
 *                    aggregate public key for this signing session
 *           session: pointer to the session that was created with
 *                    `musig_nonce_process`
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_musig_partial_sig_verify(
    const secp256k1_context *ctx,
    const secp256k1_musig_partial_sig *partial_sig,
    const secp256k1_musig_pubnonce *pubnonce,
    const secp256k1_pubkey *pubkey,
    const secp256k1_musig_keyagg_cache *keyagg_cache,
    const secp256k1_musig_session *session
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

/** Aggregates partial signatures
 *
 *  Returns: 0 if the arguments are invalid, 1 otherwise (which does NOT mean
 *           the resulting signature verifies).
 *  Args:         ctx: pointer to a context object
 *  Out:        sig64: complete (but possibly invalid) Schnorr signature
 *  In:       session: pointer to the session that was created with
 *                     musig_nonce_process
 *       partial_sigs: array of pointers to partial signatures to aggregate
 *             n_sigs: number of elements in the partial_sigs array. Must be
 *                     greater than 0.
 */
SECP256K1_API int secp256k1_musig_partial_sig_agg(
    const secp256k1_context *ctx,
    unsigned char *sig64,
    const secp256k1_musig_session *session,
    const secp256k1_musig_partial_sig * const *partial_sigs,
    size_t n_sigs
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

#ifdef __cplusplus
}
#endif

#endif
