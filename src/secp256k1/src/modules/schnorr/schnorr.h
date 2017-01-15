/***********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                               *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. *
 ***********************************************************************/

#ifndef _SECP256K1_MODULE_SCHNORR_H_
#define _SECP256K1_MODULE_SCHNORR_H_

#include "scalar.h"
#include "group.h"

typedef void (*secp256k1_schnorr_msghash)(unsigned char *h32, const unsigned char *r32, const unsigned char *msg32);

static int secp256k1_schnorr_sig_sign(const secp256k1_ecmult_gen_context* ctx, unsigned char *sig64, const secp256k1_scalar *key, const secp256k1_scalar *nonce, const secp256k1_ge *pubnonce, secp256k1_schnorr_msghash hash, const unsigned char *msg32);
static int secp256k1_schnorr_sig_verify(const secp256k1_ecmult_context* ctx, const unsigned char *sig64, const secp256k1_ge *pubkey, secp256k1_schnorr_msghash hash, const unsigned char *msg32);
static int secp256k1_schnorr_sig_recover(const secp256k1_ecmult_context* ctx, const unsigned char *sig64, secp256k1_ge *pubkey, secp256k1_schnorr_msghash hash, const unsigned char *msg32);
static int secp256k1_schnorr_sig_combine(unsigned char *sig64, size_t n, const unsigned char * const *sig64ins);

#endif
