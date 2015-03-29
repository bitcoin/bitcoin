/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
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

void secp256k1_start(unsigned int flags) {
    if (flags & SECP256K1_START_SIGN) {
        secp256k1_ecmult_gen_start();
    }
    if (flags & SECP256K1_START_VERIFY) {
        secp256k1_ecmult_start();
    }
}

void secp256k1_stop(void) {
    secp256k1_ecmult_stop();
    secp256k1_ecmult_gen_stop();
}

int secp256k1_ecdsa_verify(const unsigned char *msg32, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    secp256k1_ge_t q;
    secp256k1_ecdsa_sig_t s;
    secp256k1_scalar_t m;
    int ret = -3;
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig != NULL);
    DEBUG_CHECK(pubkey != NULL);

    secp256k1_scalar_set_b32(&m, msg32, NULL);

    if (secp256k1_eckey_pubkey_parse(&q, pubkey, pubkeylen)) {
        if (secp256k1_ecdsa_sig_parse(&s, sig, siglen)) {
            if (secp256k1_ecdsa_sig_verify(&s, &q, &m)) {
                /* success is 1, all other values are fail */
                ret = 1;
            } else {
                ret = 0;
            }
        } else {
            ret = -2;
        }
    } else {
        ret = -1;
    }

    return ret;
}

static int nonce_function_rfc6979(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, unsigned int counter, const void *data) {
   secp256k1_rfc6979_hmac_sha256_t rng;
   unsigned int i;
   secp256k1_rfc6979_hmac_sha256_initialize(&rng, key32, 32, msg32, 32, (const unsigned char*)data, data != NULL ? 32 : 0);
   for (i = 0; i <= counter; i++) {
       secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
   }
   secp256k1_rfc6979_hmac_sha256_finalize(&rng);
   return 1;
}

const secp256k1_nonce_function_t secp256k1_nonce_function_rfc6979 = nonce_function_rfc6979;
const secp256k1_nonce_function_t secp256k1_nonce_function_default = nonce_function_rfc6979;

int secp256k1_ecdsa_sign(const unsigned char *msg32, unsigned char *signature, int *signaturelen, const unsigned char *seckey, secp256k1_nonce_function_t noncefp, const void* noncedata) {
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    DEBUG_CHECK(secp256k1_ecmult_gen_consts != NULL);
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
                if (secp256k1_ecdsa_sig_sign(&sig, &sec, &msg, &non, NULL)) {
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

int secp256k1_ecdsa_sign_compact(const unsigned char *msg32, unsigned char *sig64, const unsigned char *seckey, secp256k1_nonce_function_t noncefp, const void* noncedata, int *recid) {
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t sec, non, msg;
    int ret = 0;
    int overflow = 0;
    unsigned int count = 0;
    DEBUG_CHECK(secp256k1_ecmult_gen_consts != NULL);
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
                if (secp256k1_ecdsa_sig_sign(&sig, &sec, &msg, &non, recid)) {
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

int secp256k1_ecdsa_recover_compact(const unsigned char *msg32, const unsigned char *sig64, unsigned char *pubkey, int *pubkeylen, int compressed, int recid) {
    secp256k1_ge_t q;
    secp256k1_ecdsa_sig_t sig;
    secp256k1_scalar_t m;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(msg32 != NULL);
    DEBUG_CHECK(sig64 != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    DEBUG_CHECK(recid >= 0 && recid <= 3);

    secp256k1_scalar_set_b32(&sig.r, sig64, &overflow);
    if (!overflow) {
        secp256k1_scalar_set_b32(&sig.s, sig64 + 32, &overflow);
        if (!overflow) {
            secp256k1_scalar_set_b32(&m, msg32, NULL);

            if (secp256k1_ecdsa_sig_recover(&sig, &q, &m, recid)) {
                ret = secp256k1_eckey_pubkey_serialize(&q, pubkey, pubkeylen, compressed);
            }
        }
    }
    return ret;
}

int secp256k1_ec_seckey_verify(const unsigned char *seckey) {
    secp256k1_scalar_t sec;
    int ret;
    int overflow;
    DEBUG_CHECK(seckey != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    ret = !secp256k1_scalar_is_zero(&sec) && !overflow;
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_ec_pubkey_verify(const unsigned char *pubkey, int pubkeylen) {
    secp256k1_ge_t q;
    DEBUG_CHECK(pubkey != NULL);

    return secp256k1_eckey_pubkey_parse(&q, pubkey, pubkeylen);
}

int secp256k1_ec_pubkey_create(unsigned char *pubkey, int *pubkeylen, const unsigned char *seckey, int compressed) {
    secp256k1_gej_t pj;
    secp256k1_ge_t p;
    secp256k1_scalar_t sec;
    int overflow;
    int ret = 0;
    DEBUG_CHECK(secp256k1_ecmult_gen_consts != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);
    DEBUG_CHECK(seckey != NULL);

    secp256k1_scalar_set_b32(&sec, seckey, &overflow);
    if (!overflow) {
        secp256k1_ecmult_gen(&pj, &sec);
        secp256k1_scalar_clear(&sec);
        secp256k1_ge_set_gej(&p, &pj);
        ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, pubkeylen, compressed);
    }
    if (!ret) {
        *pubkeylen = 0;
    }
    return ret;
}

int secp256k1_ec_pubkey_decompress(unsigned char *pubkey, int *pubkeylen) {
    secp256k1_ge_t p;
    int ret = 0;
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(pubkeylen != NULL);

    if (secp256k1_eckey_pubkey_parse(&p, pubkey, *pubkeylen)) {
        ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, pubkeylen, 0);
    }
    return ret;
}

int secp256k1_ec_privkey_tweak_add(unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar_t term;
    secp256k1_scalar_t sec;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);

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

int secp256k1_ec_pubkey_tweak_add(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    secp256k1_ge_t p;
    secp256k1_scalar_t term;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&term, tweak, &overflow);
    if (!overflow) {
        ret = secp256k1_eckey_pubkey_parse(&p, pubkey, pubkeylen);
        if (ret) {
            ret = secp256k1_eckey_pubkey_tweak_add(&p, &term);
        }
        if (ret) {
            int oldlen = pubkeylen;
            ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
            VERIFY_CHECK(pubkeylen == oldlen);
        }
    }

    return ret;
}

int secp256k1_ec_privkey_tweak_mul(unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar_t factor;
    secp256k1_scalar_t sec;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(tweak != NULL);

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

int secp256k1_ec_pubkey_tweak_mul(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak) {
    secp256k1_ge_t p;
    secp256k1_scalar_t factor;
    int ret = 0;
    int overflow = 0;
    DEBUG_CHECK(secp256k1_ecmult_consts != NULL);
    DEBUG_CHECK(pubkey != NULL);
    DEBUG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    if (!overflow) {
        ret = secp256k1_eckey_pubkey_parse(&p, pubkey, pubkeylen);
        if (ret) {
            ret = secp256k1_eckey_pubkey_tweak_mul(&p, &factor);
        }
        if (ret) {
            int oldlen = pubkeylen;
            ret = secp256k1_eckey_pubkey_serialize(&p, pubkey, &pubkeylen, oldlen <= 33);
            VERIFY_CHECK(pubkeylen == oldlen);
        }
    }

    return ret;
}

int secp256k1_ec_privkey_export(const unsigned char *seckey, unsigned char *privkey, int *privkeylen, int compressed) {
    secp256k1_scalar_t key;
    int ret = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);
    DEBUG_CHECK(privkeylen != NULL);

    secp256k1_scalar_set_b32(&key, seckey, NULL);
    ret = secp256k1_eckey_privkey_serialize(privkey, privkeylen, &key, compressed);
    secp256k1_scalar_clear(&key);
    return ret;
}

int secp256k1_ec_privkey_import(unsigned char *seckey, const unsigned char *privkey, int privkeylen) {
    secp256k1_scalar_t key;
    int ret = 0;
    DEBUG_CHECK(seckey != NULL);
    DEBUG_CHECK(privkey != NULL);

    ret = secp256k1_eckey_privkey_parse(&key, privkey, privkeylen);
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &key);
    }
    secp256k1_scalar_clear(&key);
    return ret;
}
