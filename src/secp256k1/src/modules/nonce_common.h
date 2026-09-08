/***********************************************************************
 * Copyright (c) The libsecp256k1 developers                           *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_NONCE_COMMON_H
#define SECP256K1_MODULE_NONCE_COMMON_H

#include "../../include/secp256k1.h"

#include "../group.h"
#include "../scalar.h"

/* Helpers shared between the modules implementing interactive signing
 * protocols with two-point nonces (musig and fullagg). The magic
 * argument distinguishes the modules' opaque objects from each other so that
 * passing an object to the wrong module's API fails. */

/* Saves a secret nonce into a 132 byte buffer: magic (4 bytes) || k[0] (32
 * bytes) || k[1] (32 bytes) || pk (64 bytes). */
static void secp256k1_secnonce_save(unsigned char *data, const unsigned char *magic, const secp256k1_scalar *k, const secp256k1_ge *pk);
static int secp256k1_secnonce_load(const secp256k1_context *ctx, secp256k1_scalar *k, secp256k1_ge *pk, const unsigned char *data, const unsigned char *magic);
/* If flag is 1, invalidate the secnonce; if flag is 0, leave it.
 * Constant-time. Flag must be 0 or 1. */
static void secp256k1_secnonce_invalidate(const secp256k1_context *ctx, unsigned char *data, int flag);

/* Saves two group elements into a 132 byte buffer: magic (4 bytes) || ges[0]
 * (64 bytes) || ges[1] (64 bytes). Requires that none of the provided group
 * elements is infinity. */
static void secp256k1_nonce_points_save(unsigned char *data, const unsigned char *magic, const secp256k1_ge *ges);
/* Loads two group elements. Returns 1 unless the buffer wasn't properly
 * initialized. */
static int secp256k1_nonce_points_load(const secp256k1_context *ctx, secp256k1_ge *ges, const unsigned char *data, const unsigned char *magic);
/* Parses two compressed points and verifies that they are in the correct
 * subgroup. */
static int secp256k1_nonce_points_parse(secp256k1_ge *ges, const unsigned char *in66);
/* Serializes two group elements as compressed points. Requires that none of
 * the provided group elements is infinity. */
static void secp256k1_nonce_points_serialize(unsigned char *out66, secp256k1_ge *ges);

/* Saves a partial signature into a 36 byte buffer: magic (4 bytes) || s (32
 * bytes). */
static void secp256k1_partial_sig_save(unsigned char *data, const unsigned char *magic, const secp256k1_scalar *s);
static int secp256k1_partial_sig_load(const secp256k1_context *ctx, secp256k1_scalar *s, const unsigned char *data, const unsigned char *magic);

/* out_nonce = nonce_pts[0] + b*nonce_pts[1] */
static void secp256k1_effective_nonce(secp256k1_gej *out_nonce, const secp256k1_ge *nonce_pts, const secp256k1_scalar *b);

static void secp256k1_partial_sign_clear(secp256k1_scalar *sk, secp256k1_scalar *k);

#endif
