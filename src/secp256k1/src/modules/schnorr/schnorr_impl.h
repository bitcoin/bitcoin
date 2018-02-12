/***********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                               *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. *
 ***********************************************************************/

#ifndef _SECP256K1_SCHNORR_IMPL_H_
#define _SECP256K1_SCHNORR_IMPL_H_

#include <string.h>

#include "schnorr.h"
#include "num.h"
#include "field.h"
#include "group.h"
#include "ecmult.h"
#include "ecmult_gen.h"

/**
 * Custom Schnorr-based signature scheme. They support multiparty signing, public key
 * recovery and batch validation.
 *
 * Rationale for verifying R's y coordinate:
 * In order to support batch validation and public key recovery, the full R point must
 * be known to verifiers, rather than just its x coordinate. In order to not risk
 * being more strict in batch validation than normal validation, validators must be
 * required to reject signatures with incorrect y coordinate. This is only possible
 * by including a (relatively slow) field inverse, or a field square root. However,
 * batch validation offers potentially much higher benefits than this cost.
 *
 * Rationale for having an implicit y coordinate oddness:
 * If we commit to having the full R point known to verifiers, there are two mechanism.
 * Either include its oddness in the signature, or give it an implicit fixed value.
 * As the R y coordinate can be flipped by a simple negation of the nonce, we choose the
 * latter, as it comes with nearly zero impact on signing or validation performance, and
 * saves a byte in the signature.
 *
 * Signing:
 *   Inputs: 32-byte message m, 32-byte scalar key x (!=0), 32-byte scalar nonce k (!=0)
 *
 *   Compute point R = k * G. Reject nonce if R's y coordinate is odd (or negate nonce).
 *   Compute 32-byte r, the serialization of R's x coordinate.
 *   Compute scalar h = Hash(r || m). Reject nonce if h == 0 or h >= order.
 *   Compute scalar s = k - h * x.
 *   The signature is (r, s).
 *
 *
 * Verification:
 *   Inputs: 32-byte message m, public key point Q, signature: (32-byte r, scalar s)
 *
 *   Signature is invalid if s >= order.
 *   Signature is invalid if r >= p.
 *   Compute scalar h = Hash(r || m). Signature is invalid if h == 0 or h >= order.
 *   Option 1 (faster for single verification):
 *     Compute point R = h * Q + s * G. Signature is invalid if R is infinity or R's y coordinate is odd.
 *     Signature is valid if the serialization of R's x coordinate equals r.
 *   Option 2 (allows batch validation and pubkey recovery):
 *     Decompress x coordinate r into point R, with odd y coordinate. Fail if R is not on the curve.
 *     Signature is valid if R + h * Q + s * G == 0.
 */

static int secp256k1_schnorr_sig_sign(const secp256k1_ecmult_gen_context* ctx, unsigned char *sig64, const secp256k1_scalar *key, const secp256k1_scalar *nonce, const secp256k1_ge *pubnonce, secp256k1_schnorr_msghash hash, const unsigned char *msg32) {
    secp256k1_gej Rj;
    secp256k1_ge Ra;
    unsigned char h32[32];
    secp256k1_scalar h, s;
    int overflow;
    secp256k1_scalar n;

    if (secp256k1_scalar_is_zero(key) || secp256k1_scalar_is_zero(nonce)) {
        return 0;
    }
    n = *nonce;

    secp256k1_ecmult_gen(ctx, &Rj, &n);
    if (pubnonce != NULL) {
        secp256k1_gej_add_ge(&Rj, &Rj, pubnonce);
    }
    secp256k1_ge_set_gej(&Ra, &Rj);
    secp256k1_fe_normalize(&Ra.y);
    if (secp256k1_fe_is_odd(&Ra.y)) {
        /* R's y coordinate is odd, which is not allowed (see rationale above).
           Force it to be even by negating the nonce. Note that this even works
           for multiparty signing, as the R point is known to all participants,
           which can all decide to flip the sign in unison, resulting in the
           overall R point to be negated too. */
        secp256k1_scalar_negate(&n, &n);
    }
    secp256k1_fe_normalize(&Ra.x);
    secp256k1_fe_get_b32(sig64, &Ra.x);
    hash(h32, sig64, msg32);
    overflow = 0;
    secp256k1_scalar_set_b32(&h, h32, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&h)) {
        secp256k1_scalar_clear(&n);
        return 0;
    }
    secp256k1_scalar_mul(&s, &h, key);
    secp256k1_scalar_negate(&s, &s);
    secp256k1_scalar_add(&s, &s, &n);
    secp256k1_scalar_clear(&n);
    secp256k1_scalar_get_b32(sig64 + 32, &s);
    return 1;
}

static int secp256k1_schnorr_sig_verify(const secp256k1_ecmult_context* ctx, const unsigned char *sig64, const secp256k1_ge *pubkey, secp256k1_schnorr_msghash hash, const unsigned char *msg32) {
    secp256k1_gej Qj, Rj;
    secp256k1_ge Ra;
    secp256k1_fe Rx;
    secp256k1_scalar h, s;
    unsigned char hh[32];
    int overflow;

    if (secp256k1_ge_is_infinity(pubkey)) {
        return 0;
    }
    hash(hh, sig64, msg32);
    overflow = 0;
    secp256k1_scalar_set_b32(&h, hh, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&h)) {
        return 0;
    }
    overflow = 0;
    secp256k1_scalar_set_b32(&s, sig64 + 32, &overflow);
    if (overflow) {
        return 0;
    }
    if (!secp256k1_fe_set_b32(&Rx, sig64)) {
        return 0;
    }
    secp256k1_gej_set_ge(&Qj, pubkey);
    secp256k1_ecmult(ctx, &Rj, &Qj, &h, &s);
    if (secp256k1_gej_is_infinity(&Rj)) {
        return 0;
    }
    secp256k1_ge_set_gej_var(&Ra, &Rj);
    secp256k1_fe_normalize_var(&Ra.y);
    if (secp256k1_fe_is_odd(&Ra.y)) {
        return 0;
    }
    return secp256k1_fe_equal_var(&Rx, &Ra.x);
}

static int secp256k1_schnorr_sig_recover(const secp256k1_ecmult_context* ctx, const unsigned char *sig64, secp256k1_ge *pubkey, secp256k1_schnorr_msghash hash, const unsigned char *msg32) {
    secp256k1_gej Qj, Rj;
    secp256k1_ge Ra;
    secp256k1_fe Rx;
    secp256k1_scalar h, s;
    unsigned char hh[32];
    int overflow;

    hash(hh, sig64, msg32);
    overflow = 0;
    secp256k1_scalar_set_b32(&h, hh, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&h)) {
        return 0;
    }
    overflow = 0;
    secp256k1_scalar_set_b32(&s, sig64 + 32, &overflow);
    if (overflow) {
        return 0;
    }
    if (!secp256k1_fe_set_b32(&Rx, sig64)) {
        return 0;
    }
    if (!secp256k1_ge_set_xo_var(&Ra, &Rx, 0)) {
        return 0;
    }
    secp256k1_gej_set_ge(&Rj, &Ra);
    secp256k1_scalar_inverse_var(&h, &h);
    secp256k1_scalar_negate(&s, &s);
    secp256k1_scalar_mul(&s, &s, &h);
    secp256k1_ecmult(ctx, &Qj, &Rj, &h, &s);
    if (secp256k1_gej_is_infinity(&Qj)) {
        return 0;
    }
    secp256k1_ge_set_gej(pubkey, &Qj);
    return 1;
}

static int secp256k1_schnorr_sig_combine(unsigned char *sig64, size_t n, const unsigned char * const *sig64ins) {
    secp256k1_scalar s = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);
    size_t i;
    for (i = 0; i < n; i++) {
        secp256k1_scalar si;
        int overflow;
        secp256k1_scalar_set_b32(&si, sig64ins[i] + 32, &overflow);
        if (overflow) {
            return -1;
        }
        if (i) {
            if (memcmp(sig64ins[i - 1], sig64ins[i], 32) != 0) {
                return -1;
            }
        }
        secp256k1_scalar_add(&s, &s, &si);
    }
    if (secp256k1_scalar_is_zero(&s)) {
        return 0;
    }
    memcpy(sig64, sig64ins[0], 32);
    secp256k1_scalar_get_b32(sig64 + 32, &s);
    secp256k1_scalar_clear(&s);
    return 1;
}

#endif
