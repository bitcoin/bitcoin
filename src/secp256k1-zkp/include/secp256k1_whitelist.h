/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_WHITELIST_
#define _SECP256K1_WHITELIST_

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECP256K1_WHITELIST_MAX_N_KEYS	256

/** Opaque data structure that holds a parsed whitelist proof
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. Nor is
 *  it guaranteed to have any particular size, nor that identical signatures
 *  will have identical representation. (That is, memcmp may return nonzero
 *  even for identical signatures.)
 *
 *  To obtain these properties, instead use secp256k1_whitelist_signature_parse
 *  and secp256k1_whitelist_signature_serialize to encode/decode signatures
 *  into a well-defined format.
 *
 *  The representation is exposed to allow creation of these objects on the
 *  stack; please *do not* use these internals directly. To learn the number
 *  of keys for a signature, use `secp256k1_whitelist_signature_n_keys`.
 */
typedef struct {
    size_t n_keys;
    /* e0, scalars */
    unsigned char data[32 * (1 + SECP256K1_WHITELIST_MAX_N_KEYS)];
} secp256k1_whitelist_signature;

/** Parse a whitelist signature
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args: ctx:    a secp256k1 context object
 *  Out:  sig:    a pointer to a signature object
 *  In:   input:  a pointer to the array to parse
 *    input_len:  the length of the above array
 *
 *  The signature must consist of a 1-byte n_keys value, followed by a 32-byte
 *  big endian e0 value, followed by n_keys many 32-byte big endian s values.
 *  If n_keys falls outside of [0..SECP256K1_WHITELIST_MAX_N_KEYS] the encoding
 *  is invalid.
 *
 *  The total length of the input array must therefore be 33 + 32 * n_keys.
 *  If the length `input_len` does not match this value, parsing will fail.
 *
 *  After the call, sig will always be initialized. If parsing failed or any
 *  scalar values overflow or are zero, the resulting sig value is guaranteed
 *  to fail validation for any set of keys.
 */
SECP256K1_API int secp256k1_whitelist_signature_parse(
    const secp256k1_context* ctx,
    secp256k1_whitelist_signature *sig,
    const unsigned char *input,
    size_t input_len
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Returns the number of keys a signature expects to have.
 *
 *  Returns: the number of keys for the given signature
 *  In: sig: a pointer to a signature object
 */
SECP256K1_API size_t secp256k1_whitelist_signature_n_keys(
    const secp256k1_whitelist_signature *sig
) SECP256K1_ARG_NONNULL(1);

/** Serialize a whitelist signature
 *
 *  Returns: 1
 *  Args:   ctx:        a secp256k1 context object
 *  Out:    output64:   a pointer to an array to store the serialization
 *  In/Out: output_len: length of the above array, updated with the actual serialized length
 *  In:     sig:        a pointer to an initialized signature object
 *
 *  See secp256k1_whitelist_signature_parse for details about the encoding.
 */
SECP256K1_API int secp256k1_whitelist_signature_serialize(
    const secp256k1_context* ctx,
    unsigned char *output,
    size_t *output_len,
    const secp256k1_whitelist_signature *sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Compute a whitelist signature
 * Returns 1: signature was successfully created
 *         0: signature was not successfully created
 * In:     ctx: pointer to a context object, initialized for signing and verification
 *         online_pubkeys: list of all online pubkeys
 *         offline_pubkeys: list of all offline pubkeys
 *         n_keys: the number of entries in each of the above two arrays
 *         sub_pubkey: the key to be whitelisted
 *         online_seckey: the secret key to the signer's online pubkey
 *         summed_seckey: the secret key to the sum of (whitelisted key, signer's offline pubkey)
 *         index: the signer's index in the lists of keys
 *         noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *         ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 * Out:    sig: The produced signature.
 *
 * The signatures are of the list of all passed pubkeys in the order
 *     ( whitelist, online_1, offline_1, online_2, offline_2, ... )
 * The verification key list consists of
 *     online_i + H(offline_i + whitelist)(offline_i + whitelist)
 * for each public key pair (offline_i, offline_i). Here H means sha256 of the
 * compressed serialization of the key.
 */
SECP256K1_API int secp256k1_whitelist_sign(
  const secp256k1_context* ctx,
  secp256k1_whitelist_signature *sig,
  const secp256k1_pubkey *online_pubkeys,
  const secp256k1_pubkey *offline_pubkeys,
  const size_t n_keys,
  const secp256k1_pubkey *sub_pubkey,
  const unsigned char *online_seckey,
  const unsigned char *summed_seckey,
  const size_t index,
  secp256k1_nonce_function noncefp,
  const void *noncedata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7) SECP256K1_ARG_NONNULL(8);

/** Verify a whitelist signature
 * Returns 1: signature is valid
 *         0: signature is not valid
 * In:     ctx: pointer to a context object, initialized for signing and verification
 *         sig: the signature to be verified
 *         online_pubkeys: list of all online pubkeys
 *         offline_pubkeys: list of all offline pubkeys
 *         n_keys: the number of entries in each of the above two arrays
 *         sub_pubkey: the key to be whitelisted
 */
SECP256K1_API int secp256k1_whitelist_verify(
  const secp256k1_context* ctx,
  const secp256k1_whitelist_signature *sig,
  const secp256k1_pubkey *online_pubkeys,
  const secp256k1_pubkey *offline_pubkeys,
  const size_t n_keys,
  const secp256k1_pubkey *sub_pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(6);

#ifdef __cplusplus
}
#endif

#endif
