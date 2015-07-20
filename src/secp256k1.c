/**********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#define SECP256K1_BUILD (1)

#include "include/secp256k1.h"

#include "util.h"
#include "num_impl.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "group_impl.h"
#include "ecmult_impl.h"
#include "ecmult_gen_impl.h"
#include "ecdsa_impl.h"
#include "eckey_impl.h"
#include "hash_impl.h"

struct secp256k1_context_struct {
    secp256k1_ecmult_context_t ecmult_ctx;
    secp256k1_ecmult_gen_context_t ecmult_gen_ctx;
};

secp256k1_context_t* secp256k1_context_create(int flags) {
    secp256k1_context_t* ret = (secp256k1_context_t*)checked_malloc(sizeof(secp256k1_context_t));

    secp256k1_ecmult_context_init(&ret->ecmult_ctx);
    secp256k1_ecmult_gen_context_init(&ret->ecmult_gen_ctx);

    if (flags & SECP256K1_CONTEXT_SIGN) {
        secp256k1_ecmult_gen_context_build(&ret->ecmult_gen_ctx);
    }
    if (flags & SECP256K1_CONTEXT_VERIFY) {
        secp256k1_ecmult_context_build(&ret->ecmult_ctx);
    }

    return ret;
}

secp256k1_context_t* secp256k1_context_clone(const secp256k1_context_t* ctx) {
    secp256k1_context_t* ret = (secp256k1_context_t*)checked_malloc(sizeof(secp256k1_context_t));
    secp256k1_ecmult_context_clone(&ret->ecmult_ctx, &ctx->ecmult_ctx);
    secp256k1_ecmult_gen_context_clone(&ret->ecmult_gen_ctx, &ctx->ecmult_gen_ctx);
    return ret;
}

void secp256k1_context_destroy(secp256k1_context_t* ctx) {
    secp256k1_ecmult_context_clear(&ctx->ecmult_ctx);
    secp256k1_ecmult_gen_context_clear(&ctx->ecmult_gen_ctx);

    free(ctx);
}

static void secp256k1_pubkey_load(secp256k1_ge_t* ge, const secp256k1_pubkey_t* pubkey) {
    if (sizeof(secp256k1_ge_storage_t) == 64) {
        /* When the secp256k1_ge_storage_t type is exactly 64 byte, use its
         * representation inside secp256k1_pubkey_t, as conversion is very fast.
         * Note that secp256k1_pubkey_save must use the same representation. */
        secp256k1_ge_storage_t s;
        memcpy(&s, &pubkey->data[0], 64);
        secp256k1_ge_from_storage(ge, &s);
        DEBUG_CHECK(!secp256k1_fe_is_zero(&ge->x));
    } else {
        /* Otherwise, fall back to 32-byte big endian for X and Y. */
        secp256k1_fe_t x, y;
        secp256k1_fe_set_b32(&x, pubkey->data);
        DEBUG_CHECK(!secp256k1_fe_is_zero(&x));
        secp256k1_fe_set_b32(&y, pubkey->data + 32);
        secp256k1_ge_set_xy(ge, &x, &y);
    }
}

static void secp256k1_pubkey_save(secp256k1_pubkey_t* pubkey, secp256k1_ge_t* ge) {
    if (sizeof(secp256k1_ge_storage_t) == 64) {
        secp256k1_ge_storage_t s;
        secp256k1_ge_to_storage(&s, ge);
        memcpy(&pubkey->data[0], &s, 64);
    } else {
        VERIFY_CHECK(!secp256k1_ge_is_infinity(ge));
        secp256k1_fe_normalize_var(&ge->x);
        secp256k1_fe_normalize_var(&ge->y);
        secp256k1_fe_get_b32(pubkey->data, &ge->x);
        secp256k1_fe_get_b32(pubkey->data + 32, &ge->y);
    }
}

int secp256k1_ec_pubkey_parse(const secp256k1_context_t* ctx, secp256k1_pubkey_t* pubkey, const unsigned char *input, int inputlen) {
    secp256k1_ge_t Q;

    (void)ctx;
    if (!secp256k1_eckey_pubkey_parse(&Q, input, inputlen)) {
        memset(pubkey, 0, sizeof(*pubkey));
        return 0;
    }
    secp256k1_pubkey_save(pubkey, &Q);
    secp256k1_ge_clear(&Q);
    return 1;
}

int secp256k1_ec_pubkey_serialize(const secp256k1_context_t* ctx, unsigned char *output, int *outputlen, const secp256k1_pubkey_t* pubkey, int compressed) {
    secp256k1_ge_t Q;

    (void)ctx;
    secp256k1_pubkey_load(&Q, pubkey);
    return secp256k1_eckey_pubkey_serialize(&Q, output, outputlen, compressed);
}

int secp256k1_ecdsa_verify(const secp256k1_context_t* ctx, const unsigned char *msg32, const unsigned char *sig, int siglen, const secp256k1_pubkey_t *pubkey) {
    secp256k1_ge_t q;
    secp256k1_ecdsa_sig_t s;
    secp256k1_scalar_t m;
    int ret = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig != NULL);
    DEBUG_CHECK(pubkey != NULL);

    secp256k1_scalar_set_b32(&m, msg32, NULL);

    secp256k1_pubkey_load(&q, pubkey);
    if (secp256k1_ecdsa_sig_parse(&s, sig, siglen)) {
        if (secp256k1_ecdsa_sig_verify(&ctx->ecmult_ctx, &s, &q, &m)) {
            /* success is 1, all other values are fail */
            ret = 1;
        }
    }

    return ret;
}

static int nonce_function_rfc6979(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, unsigned int counter, const void *data) {
   unsigned char keydata[96];
   secp256k1_rfc6979_hmac_sha256_t rng;
   unsigned int i;
   /* We feed a byte array to the PRNG as input, consisting of:
    * - the private key (32 bytes) and message (32 bytes), see RFC 6979 3.2d.
    * - optionally 32 extra bytes of data, see RFC 6979 3.6 Additional Data.
    */
   memcpy(keydata, key32, 32);
   memcpy(keydata + 32, msg32, 32);
   if (data != NULL) {
       memcpy(keydata + 64, data, 32);
   }
   secp256k1_rfc6979_hmac_sha256_initialize(&rng, keydata, data != NULL ? 96 : 64);
   memset(keydata, 0, sizeof(keydata));
   for (i = 0; i <= counter; i++) {
       secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
   }
   secp256k1_rfc6979_hmac_sha256_finalize(&rng);
   return 1;
}

const secp256k1_nonce_function_t secp256k1_nonce_function_rfc6979 = nonce_function_rfc6979;
const secp256k1_nonce_function_t secp256k1_nonce_function_default = nonce_function_rfc6979;

int secp256k1_ecdsa_sign(const secp256k1_context_t* ctx, const unsigned char *msg32, unsigned char *signature, int *signaturelen, const unsigned char *seckey, secp256k1_nonce_function_t noncefp, const void* noncedata) {
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(signature != NULL);
    DEBUG_CHECK(signaturelen != NULL);
    DEBUG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    /* Fail if the secret key is invalid. */
    if (!overflow && !secp256k1_scalar_is_zero(&sec)) {
        secp256k1_scalar_set_b32(&msg, msg32, NULL);
        while (1) {
            unsigned char nonce32[32];
            ret = noncefp(nonce32, msg32, seckey, count, noncedata);
            if (!ret) {
                break;
            }
            secp256k1_scalar_set_b32(&non, nonce32, &overflow);
            memset(nonce32, 0, 32);
            if (!secp256k1_scalar_is_zero(&non) && !overflow) {
                if (secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &sig, &sec, &msg, &non, NULL)) {
                    break;
                }
            }
            count++;
        }
        if (ret) {
            ret = secp256k1_ecdsa_sig_serialize(signature, signaturelen, &sig);
        }
        secp256k1_scalar_clear(&msg);
        secp256k1_scalar_clear(&non);
        secp256k1_scalar_clear(&sec);
    }
    if (!ret) {
        *signaturelen = 0;
    }
    return ret;
}

int secp256k1_ecdsa_sign_compact(const secp256k1_context_t* ctx, const unsigned char *msg32, unsigned char *sig64, const unsigned char *seckey, secp256k1_nonce_function_t noncefp, const void* noncedata, int *recid) {
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(seckey != NULL);
    if (noncefp == NULL) {
        noncefp = secp256k1_nonce_function_default;
    }

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    /* Fail if the secret key is invalid. */
    if (!overflow && !secp256k1_scalar_is_zero(&sec)) {
        secp256k1_scalar_set_b32(&msg, msg32, NULL);
        while (1) {
            unsigned char nonce32[32];
            ret = noncefp(nonce32, msg32, seckey, count, noncedata);
            if (!ret) {
                break;
            }
            secp256k1_scalar_set_b32(&non, nonce32, &overflow);
            memset(nonce32, 0, 32);
            if (!secp256k1_scalar_is_zero(&non) && !overflow) {
                if (secp256k1_ecdsa_sig_sign(&ctx->ecmult_gen_ctx, &sig, &sec, &msg, &non, recid)) {
                    break;
                }
            }
            count++;
        }
        if (ret) {
            secp256k1_scalar_get_b32(sig64, &sig.r);
            secp256k1_scalar_get_b32(sig64 + 32, &sig.s);
        }
        secp256k1_scalar_clear(&msg);
        secp256k1_scalar_clear(&non);
        secp256k1_scalar_clear(&sec);
    }
    if (!ret) {
        memset(sig64, 0, 64);
    }
    return ret;
}

int secp256k1_ecdsa_recover_compact(const secp256k1_context_t* ctx, const unsigned char *msg32, const unsigned char *sig64, secp256k1_pubkey_t *pubkey, int recid) {
    secp256k1_ge_t q;
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t m;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(recid >= 0 && recid <= 3);

    secp256k1_scalar_set_b32(&sig.r, sig64, &overflow);
    if (!overflow) {
        secp256k1_scalar_set_b32(&sig.s, sig64 + 32, &overflow);
        if (!overflow) {
            secp256k1_scalar_set_b32(&m, msg32, NULL);

            ret = secp256k1_ecdsa_sig_recover(&ctx->ecmult_ctx, &sig, &q, &m, recid);
            if (ret) {
                secp256k1_pubkey_save(pubkey, &q);
            } else {
                memset(pubkey, 0, sizeof(*pubkey));
            }
        }
    }
    return ret;
}

int secp256k1_ec_seckey_verify(const secp256k1_context_t* ctx, const unsigned char *seckey) {
    secp256k1_scalar_t sec;
    int ret;
    int overflow;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(seckey != NULL);
    (void)ctx;

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = !secp256k1_scalar_is_zero(&sec) && !overflow;
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ec_pubkey_create(const secp256k1_context_t* ctx, secp256k1_pubkey_t *pubkey, const unsigned char *seckey) {
    secp256k1_gej_t pj;
    secp256k1_ge_t p;
    secp256k1_scalar_t sec;
    int overflow;
    int ret = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(seckey != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = !overflow & !secp256k1_scalar_is_zero(&sec);
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pj, &sec);
    secp256k1_ge_set_gej(&p, &pj);
    secp256k1_pubkey_save(pubkey, &p);
    secp256k1_scalar_clear(&sec);
    if (!ret) {
        memset(pubkey, 0, sizeof(*pubkey));
    }
    return ret;
}

int secp256k1_ec_privkey_tweak_add(const secp256k1_context_t* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar_t term;
    secp256k1_scalar_t sec;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);
    (void)ctx;

    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    secp256k1_scalar_set_b32(&sec, seckey, NULL);

    ret = secp256k1_eckey_privkey_tweak_add(&sec, &term) && !overflow;
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&term);
    return ret;
}

int secp256k1_ec_pubkey_tweak_add(const secp256k1_context_t* ctx, secp256k1_pubkey_t *pubkey, const unsigned char *tweak) {
    secp256k1_ge_t p;
    secp256k1_scalar_t term;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    secp256k1_pubkey_load(&p, pubkey);
    if (!overflow) {
        ret = secp256k1_eckey_pubkey_tweak_add(&ctx->ecmult_ctx, &p, &term);
        if (ret) {
            secp256k1_pubkey_save(pubkey, &p);
        } else {
            memset(pubkey, 0, sizeof(*pubkey));
        }
    }

    return ret;
}

int secp256k1_ec_privkey_tweak_mul(const secp256k1_context_t* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar_t factor;
    secp256k1_scalar_t sec;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);
    (void)ctx;

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    ret = secp256k1_eckey_privkey_tweak_mul(&sec, &factor) && !overflow;
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&factor);
    return ret;
}

int secp256k1_ec_pubkey_tweak_mul(const secp256k1_context_t* ctx, secp256k1_pubkey_t *pubkey, const unsigned char *tweak) {
    secp256k1_ge_t p;
    secp256k1_scalar_t factor;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    secp256k1_pubkey_load(&p, pubkey);
    if (!overflow) {
        ret = secp256k1_eckey_pubkey_tweak_mul(&ctx->ecmult_ctx, &p, &factor);
        if (ret) {
            secp256k1_pubkey_save(pubkey, &p);
        } else {
            memset(pubkey, 0, sizeof(*pubkey));
        }
    }

    return ret;
}

int secp256k1_ec_privkey_export(const secp256k1_context_t* ctx, const unsigned char *seckey, unsigned char *privkey, int *privkeylen, int compressed) {
    secp256k1_scalar_t key;
    int ret = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);
    DEBUG_CHECK(privkeylen != NULL);
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));

    secp256k1_scalar_set_b32(&key, seckey, NULL);
    ret = secp256k1_eckey_privkey_serialize(&ctx->ecmult_gen_ctx, privkey, privkeylen, &key, compressed);
    secp256k1_scalar_clear(&key);
    return ret;
}

int secp256k1_ec_privkey_import(const secp256k1_context_t* ctx, unsigned char *seckey, const unsigned char *privkey, int privkeylen) {
    secp256k1_scalar_t key;
    int ret = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);
    (void)ctx;

    ret = secp256k1_eckey_privkey_parse(&key, privkey, privkeylen);
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &key);
    }
    secp256k1_scalar_clear(&key);
    return ret;
}

int secp256k1_context_randomize(secp256k1_context_t* ctx, const unsigned char *seed32) {
    DEBUG_CHECK(ctx != NULL);
    DEBUG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    secp256k1_ecmult_gen_blind(&ctx->ecmult_gen_ctx, seed32);
    return 1;
}
