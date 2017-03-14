/**********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_SCHNORR_MAIN
#define SECP256K1_MODULE_SCHNORR_MAIN

#include "include/secp256k1_schnorr.h"
#include "modules/schnorr/schnorr_impl.h"

static void secp256k1_schnorr_msghash_sha256(unsigned char *h32, const unsigned char *r32, const unsigned char *msg32) {
    secp256k1_sha256_t sha;
    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&sha, r32, 32);
    secp256k1_sha256_write(&sha, msg32, 32);
    secp256k1_sha256_finalize(&sha, h32);
}

static const unsigned char secp256k1_schnorr_algo16[17] = "Schnorr+SHA256  ";

int secp256k1_schnorr_sign(const secp256k1_context* ctx, unsigned char *sig64, const unsigned char *msg32, const unsigned char *seckey, secp256k1_nonce_function noncefp, const void* noncedata) {
    secp256k1_scalar sec, non;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    while (1) {
        unsigned char nonce32[32];
        ret = noncefp(nonce32, msg32, seckey, secp256k1_schnorr_algo16, (void*)noncedata, count);
        if (!ret) {
            break;
        }
        secp256k1_scalar_set_b32(&non, nonce32, &overflow);
        memset(nonce32, 0, 32);
        if (!secp256k1_scalar_is_zero(&non) && !overflow) {
            if (secp256k1_schnorr_sig_sign(&ctx->ecmult_gen_ctx, sig64, &sec, &non, NULL, secp256k1_schnorr_msghash_sha256, msg32)) {
                break;
            }
        }
        count++;
    }
    if (!ret) {
        memset(sig64, 0, 64);
    }
    secp256k1_scalar_clear(&non);
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_schnorr_verify(const secp256k1_context* ctx, const unsigned char *sig64, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ge q;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(pubkey != NULL);

    secp256k1_pubkey_load(ctx, &q, pubkey);
    return secp256k1_schnorr_sig_verify(&ctx->ecmult_ctx, sig64, &q, secp256k1_schnorr_msghash_sha256, msg32);
}

int secp256k1_schnorr_recover(const secp256k1_context* ctx, secp256k1_pubkey *pubkey, const unsigned char *sig64, const unsigned char *msg32) {
    secp256k1_ge q;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(pubkey != NULL);

    if (secp256k1_schnorr_sig_recover(&ctx->ecmult_ctx, sig64, &q, secp256k1_schnorr_msghash_sha256, msg32)) {
        secp256k1_pubkey_save(pubkey, &q);
        return 1;
    } else {
        memset(pubkey, 0, sizeof(*pubkey));
        return 0;
    }
}

int secp256k1_schnorr_generate_nonce_pair(const secp256k1_context* ctx, secp256k1_pubkey *pubnonce, unsigned char *privnonce32, const unsigned char *sec32, const unsigned char *msg32, secp256k1_nonce_function noncefp, const void* noncedata) {
    int count = 0;
    int ret = 1;
    secp256k1_gej Qj;
    secp256k1_ge Q;
    secp256k1_scalar sec;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sec32 != NULL);
    ARG_CHECK(pubnonce != NULL);
    ARG_CHECK(privnonce32 != NULL);

    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    do {
        int overflow;
        ret = noncefp(privnonce32, sec32, msg32, secp256k1_schnorr_algo16, (void*)noncedata, count++);
        if (!ret) {
            break;
        }
        secp256k1_scalar_set_b32(&sec, privnonce32, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&sec)) {
            continue;
        }
        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &Qj, &sec);
        secp256k1_ge_set_gej(&Q, &Qj);

        secp256k1_pubkey_save(pubnonce, &Q);
        break;
    } while(1);

    secp256k1_scalar_clear(&sec);
    if (!ret) {
        memset(pubnonce, 0, sizeof(*pubnonce));
    }
    return ret;
}

int secp256k1_schnorr_partial_sign(const secp256k1_context* ctx, unsigned char *sig64, const unsigned char *msg32, const unsigned char *sec32, const secp256k1_pubkey *pubnonce_others, const unsigned char *secnonce32) {
    int overflow = 0;
    secp256k1_scalar sec, non;
    secp256k1_ge pubnon;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(sec32 != NULL);
    ARG_CHECK(secnonce32 != NULL);
    ARG_CHECK(pubnonce_others != NULL);

    secp256k1_scalar_set_b32(&sec, sec32, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&sec)) {
        return -1;
    }
    secp256k1_scalar_set_b32(&non, secnonce32, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&non)) {
        return -1;
    }
    secp256k1_pubkey_load(ctx, &pubnon, pubnonce_others);
    return secp256k1_schnorr_sig_sign(&ctx->ecmult_gen_ctx, sig64, &sec, &non, &pubnon, secp256k1_schnorr_msghash_sha256, msg32);
}

int secp256k1_schnorr_partial_combine(const secp256k1_context* ctx, unsigned char *sig64, const unsigned char * const *sig64sin, size_t n) {
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(n >= 1);
    ARG_CHECK(sig64sin != NULL);
    return secp256k1_schnorr_sig_combine(sig64, n, sig64sin);
}

#endif
