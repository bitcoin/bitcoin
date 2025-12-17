#ifndef SECP256K1_EXTRAKEYS_H
#define SECP256K1_EXTRAKEYS_H

#include "secp256k1.h"

/** Opaque data structure that holds a parsed and valid "x-only" public key.
 *  An x-only pubkey encodes a point whose Y coordinate is even. It is
 *  serialized using only its X coordinate (32 bytes). See BIP-340 for more
 *  information about x-only pubkeys.
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage, transmission, use
 *  use secp256k1_xonly_pubkey_serialize and secp256k1_xonly_pubkey_parse. To
 *  compare keys, use secp256k1_xonly_pubkey_cmp.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_xonly_pubkey;

/** Parse a 32-byte sequence into a xonly_pubkey object.
 *
 *  Returns: 1 if the public key was fully valid.
 *           0 if the public key could not be parsed or is invalid.
 *
 *  Out: pubkey: pointer to a pubkey object. If 1 is returned, it is set to a
 *               parsed version of input. If not, it's set to an invalid value.
 *  In: input32: pointer to a serialized xonly_pubkey.
 */
static SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_parse(
    secp256k1_xonly_pubkey* pubkey,
    const unsigned char *input32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Serialize an xonly_pubkey object into a 32-byte sequence.
 *
 *  Returns: 1 always.
 *
 *  Out: output32: a pointer to a 32-byte array to place the serialized key in.
 *  In:    pubkey: a pointer to a secp256k1_xonly_pubkey containing an initialized public key.
 */
static int secp256k1_xonly_pubkey_serialize(
    unsigned char *output32,
    const secp256k1_xonly_pubkey* pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);
/** Converts a secp256k1_pubkey into a secp256k1_xonly_pubkey.
 *
 *  Returns: 1 always.
 *
 *  Out: xonly_pubkey: pointer to an x-only public key object for placing the converted public key.
 *          pk_parity: Ignored if NULL. Otherwise, pointer to an integer that
 *                     will be set to 1 if the point encoded by xonly_pubkey is
 *                     the negation of the pubkey and set to 0 otherwise.
 *  In:        pubkey: pointer to a public key that is converted.
 */
static SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_from_pubkey(
    secp256k1_xonly_pubkey *xonly_pubkey,
    int *pk_parity,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3);

/** Tweak an x-only public key by adding the generator multiplied with tweak32
 *  to it.
 *
 *  Note that the resulting point can not in general be represented by an x-only
 *  pubkey because it may have an odd Y coordinate. Instead, the output_pubkey
 *  is a normal secp256k1_pubkey.
 *
 *  Returns: 0 if the arguments are invalid or the resulting public key would be
 *           invalid (only when the tweak is the negation of the corresponding
 *           secret key). 1 otherwise.
 *
 *  Out:  output_pubkey: pointer to a public key to store the result. Will be set
 *                       to an invalid value if this function returns 0.
 *  In: internal_pubkey: pointer to an x-only pubkey to apply the tweak to.
 *              tweak32: pointer to a 32-byte tweak. If the tweak is invalid
 *                       according to secp256k1_ec_seckey_verify, this function
 *                       returns 0. For uniformly random 32-byte arrays the
 *                       chance of being invalid is negligible (around 1 in 2^128).
 */
static SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_tweak_add(
    secp256k1_pubkey *output_pubkey,
    const secp256k1_xonly_pubkey *internal_pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
#endif /* SECP256K1_EXTRAKEYS_H */
