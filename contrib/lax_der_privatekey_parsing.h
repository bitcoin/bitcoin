/***********************************************************************
 * Copyright (c) 2014, 2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

/****
 * Please do not link this file directly. It is not part of the libsecp256k1
 * project and does not promise any stability in its API, functionality or
 * presence. Projects which use this code should instead copy this header
 * and its accompanying .c file directly into their codebase.
 ****/

/* This file contains code snippets that parse DER private keys with
 * various errors and violations.  This is not a part of the library
 * itself, because the allowed violations are chosen arbitrarily and
 * do not follow or establish any standard.
 *
 * It also contains code to serialize private keys in a compatible
 * manner.
 *
 * These functions are meant for compatibility with applications
 * that require BER encoded keys. When working with secp256k1-specific
 * code, the simple 32-byte private keys normally used by the
 * library are sufficient.
 */

#ifndef SECP256K1_CONTRIB_BER_PRIVATEKEY_H
#define SECP256K1_CONTRIB_BER_PRIVATEKEY_H

#include <secp256k1.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Export a private key in DER format.
 *
 *  Returns: 1 if the private key was valid.
 *  Args: ctx:        pointer to a context object, initialized for signing (cannot
 *                    be NULL)
 *  Out: privkey:     pointer to an array for storing the private key in BER.
 *                    Should have space for 279 bytes, and cannot be NULL.
 *       privkeylen:  Pointer to an int where the length of the private key in
 *                    privkey will be stored.
 *  In:  seckey:      pointer to a 32-byte secret key to export.
 *       compressed:  1 if the key should be exported in
 *                    compressed format, 0 otherwise
 *
 *  This function is purely meant for compatibility with applications that
 *  require BER encoded keys. When working with secp256k1-specific code, the
 *  simple 32-byte private keys are sufficient.
 *
 *  Note that this function does not guarantee correct DER output. It is
 *  guaranteed to be parsable by secp256k1_ec_privkey_import_der
 */
SECP256K1_WARN_UNUSED_RESULT int ec_privkey_export_der(
    const secp256k1_context* ctx,
    unsigned char *privkey,
    size_t *privkeylen,
    const unsigned char *seckey,
    int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Import a private key in DER format.
 * Returns: 1 if a private key was extracted.
 * Args: ctx:        pointer to a context object (cannot be NULL).
 * Out:  seckey:     pointer to a 32-byte array for storing the private key.
 *                   (cannot be NULL).
 * In:   privkey:    pointer to a private key in DER format (cannot be NULL).
 *       privkeylen: length of the DER private key pointed to be privkey.
 *
 * This function will accept more than just strict DER, and even allow some BER
 * violations. The public key stored inside the DER-encoded private key is not
 * verified for correctness, nor are the curve parameters. Use this function
 * only if you know in advance it is supposed to contain a secp256k1 private
 * key.
 */
SECP256K1_WARN_UNUSED_RESULT int ec_privkey_import_der(
    const secp256k1_context* ctx,
    unsigned char *seckey,
    const unsigned char *privkey,
    size_t privkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_CONTRIB_BER_PRIVATEKEY_H */
