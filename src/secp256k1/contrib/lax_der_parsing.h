/***********************************************************************
 * Copyright (c) 2015 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

/****
 * Please do not link this file directly. It is not part of the libsecp256k1
 * project and does not promise any stability in its API, functionality or
 * presence. Projects which use this code should instead copy this header
 * and its accompanying .c file directly into their codebase.
 ****/

/* This file defines a function that parses DER with various errors and
 * violations. This is not a part of the library itself, because the allowed
 * violations are chosen arbitrarily and do not follow or establish any
 * standard.
 *
 * In many places it matters that different implementations do not only accept
 * the same set of valid signatures, but also reject the same set of signatures.
 * The only means to accomplish that is by strictly obeying a standard, and not
 * accepting anything else.
 *
 * Nonetheless, sometimes there is a need for compatibility with systems that
 * use signatures which do not strictly obey DER. The snippet below shows how
 * certain violations are easily supported. You may need to adapt it.
 *
 * Do not use this for new systems. Use well-defined DER or compact signatures
 * instead if you have the choice (see secp256k1_ecdsa_signature_parse_der and
 * secp256k1_ecdsa_signature_parse_compact).
 *
 * The supported violations are:
 * - All numbers are parsed as nonnegative integers, even though X.609-0207
 *   section 8.3.3 specifies that integers are always encoded as two's
 *   complement.
 * - Integers can have length 0, even though section 8.3.1 says they can't.
 * - Integers with overly long padding are accepted, violation section
 *   8.3.2.
 * - 127-byte long length descriptors are accepted, even though section
 *   8.1.3.5.c says that they are not.
 * - Trailing garbage data inside or after the signature is ignored.
 * - The length descriptor of the sequence is ignored.
 *
 * Compared to for example OpenSSL, many violations are NOT supported:
 * - Using overly long tag descriptors for the sequence or integers inside,
 *   violating section 8.1.2.2.
 * - Encoding primitive integers as constructed values, violating section
 *   8.3.1.
 */

#ifndef SECP256K1_CONTRIB_LAX_DER_PARSING_H
#define SECP256K1_CONTRIB_LAX_DER_PARSING_H

/* #include secp256k1.h only when it hasn't been included yet.
   This enables this file to be #included directly in other project
   files (such as tests.c) without the need to set an explicit -I flag,
   which would be necessary to locate secp256k1.h. */
#ifndef SECP256K1_H
#include <secp256k1.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Parse a signature in "lax DER" format
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args: ctx:      a secp256k1 context object
 *  Out:  sig:      pointer to a signature object
 *  In:   input:    pointer to the signature to be parsed
 *        inputlen: the length of the array pointed to be input
 *
 *  This function will accept any valid DER encoded signature, even if the
 *  encoded numbers are out of range. In addition, it will accept signatures
 *  which violate the DER spec in various ways. Its purpose is to allow
 *  validation of the Tortoisecoin blockchain, which includes non-DER signatures
 *  from before the network rules were updated to enforce DER. Note that
 *  the set of supported violations is a strict subset of what OpenSSL will
 *  accept.
 *
 *  After the call, sig will always be initialized. If parsing failed or the
 *  encoded numbers are out of range, signature validation with it is
 *  guaranteed to fail for every message and public key.
 */
int ecdsa_signature_parse_der_lax(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature* sig,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_CONTRIB_LAX_DER_PARSING_H */
