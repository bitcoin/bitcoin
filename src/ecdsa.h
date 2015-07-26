/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

#include "scalar.h"
#include "group.h"
#include "ecmult.h"

static int secp256k1_ecdsa_sig_parse(secp256k1_scalar_t *r, secp256k1_scalar_t *s, const unsigned char *sig, int size);
static int secp256k1_ecdsa_sig_serialize(unsigned char *sig, int *size, const secp256k1_scalar_t *r, const secp256k1_scalar_t *s);
static int secp256k1_ecdsa_sig_verify(const secp256k1_ecmult_context_t *ctx, const secp256k1_scalar_t* r, const secp256k1_scalar_t* s, const secp256k1_ge_t *pubkey, const secp256k1_scalar_t *message);
static int secp256k1_ecdsa_sig_sign(const secp256k1_ecmult_gen_context_t *ctx, secp256k1_scalar_t* r, secp256k1_scalar_t* s, const secp256k1_scalar_t *seckey, const secp256k1_scalar_t *message, const secp256k1_scalar_t *nonce, int *recid);
static int secp256k1_ecdsa_sig_recover(const secp256k1_ecmult_context_t *ctx, const secp256k1_scalar_t* r, const secp256k1_scalar_t* s, secp256k1_ge_t *pubkey, const secp256k1_scalar_t *message, int recid);

#endif
