/***********************************************************************
 * Copyright (c) 2025 Fabian Jahr                                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_FULLAGG_MAIN_H
#define SECP256K1_MODULE_FULLAGG_MAIN_H

#include <string.h>

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_fullagg.h"

#include "../nonce_common.h"
#include "../../eckey.h"
#include "../../ecmult.h"
#include "../../group.h"
#include "../../hash.h"
#include "../../scalar.h"
#include "../../util.h"

static const unsigned char secp256k1_fullagg_secnonce_magic[4] = { 0xf1, 0x1a, 0x99, 0x01 };

static void secp256k1_fullagg_secnonce_save(secp256k1_fullagg_secnonce *secnonce, const secp256k1_scalar *k, const secp256k1_ge *pk) {
    secp256k1_secnonce_save(secnonce->data, secp256k1_fullagg_secnonce_magic, k, pk);
}

static int secp256k1_fullagg_secnonce_load(const secp256k1_context* ctx, secp256k1_scalar *k, secp256k1_ge *pk, const secp256k1_fullagg_secnonce *secnonce) {
    return secp256k1_secnonce_load(ctx, k, pk, secnonce->data, secp256k1_fullagg_secnonce_magic);
}

static void secp256k1_fullagg_secnonce_invalidate(const secp256k1_context* ctx, secp256k1_fullagg_secnonce *secnonce, int flag) {
    secp256k1_secnonce_invalidate(ctx, secnonce->data, flag);
}

static const unsigned char secp256k1_fullagg_pubnonce_magic[4] = { 0xf1, 0x1a, 0x99, 0x02 };

static void secp256k1_fullagg_pubnonce_save(secp256k1_fullagg_pubnonce* nonce, const secp256k1_ge* ges) {
    secp256k1_nonce_points_save(nonce->data, secp256k1_fullagg_pubnonce_magic, ges);
}

static int secp256k1_fullagg_pubnonce_load(const secp256k1_context* ctx, secp256k1_ge* ges, const secp256k1_fullagg_pubnonce* nonce) {
    return secp256k1_nonce_points_load(ctx, ges, nonce->data, secp256k1_fullagg_pubnonce_magic);
}

static const unsigned char secp256k1_fullagg_aggnonce_magic[4] = { 0xf1, 0x1a, 0x99, 0x03 };

/* Save aggregate nonce. The points are guaranteed not to be the point at
 * infinity because nonce aggregation fails in that case. */
static void secp256k1_fullagg_aggnonce_save(secp256k1_fullagg_aggnonce* nonce, const secp256k1_ge* ges) {
    secp256k1_nonce_points_save(nonce->data, secp256k1_fullagg_aggnonce_magic, ges);
}

/* Load aggregate nonce */
static int secp256k1_fullagg_aggnonce_load(const secp256k1_context* ctx, secp256k1_ge* ges, const secp256k1_fullagg_aggnonce* nonce) {
    return secp256k1_nonce_points_load(ctx, ges, nonce->data, secp256k1_fullagg_aggnonce_magic);
}

static const unsigned char secp256k1_fullagg_partial_sig_magic[4] = { 0xf1, 0x1a, 0x99, 0x05 };

static void secp256k1_fullagg_partial_sig_save(secp256k1_fullagg_partial_sig* sig, secp256k1_scalar *s) {
    secp256k1_partial_sig_save(sig->data, secp256k1_fullagg_partial_sig_magic, s);
}

static int secp256k1_fullagg_partial_sig_load(const secp256k1_context* ctx, secp256k1_scalar *s, const secp256k1_fullagg_partial_sig* sig) {
    return secp256k1_partial_sig_load(ctx, s, sig->data, secp256k1_fullagg_partial_sig_magic);
}

/* Parse/serialize functions for public interface */
int secp256k1_fullagg_pubnonce_parse(const secp256k1_context* ctx, secp256k1_fullagg_pubnonce* nonce, const unsigned char *in66) {
    secp256k1_ge ges[2];

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(nonce != NULL);
    ARG_CHECK(in66 != NULL);

    if (!secp256k1_nonce_points_parse(ges, in66)) {
        return 0;
    }
    secp256k1_fullagg_pubnonce_save(nonce, ges);
    return 1;
}

int secp256k1_fullagg_pubnonce_serialize(const secp256k1_context* ctx, unsigned char *out66, const secp256k1_fullagg_pubnonce* nonce) {
    secp256k1_ge ges[2];

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out66 != NULL);
    ARG_CHECK(nonce != NULL);
    memset(out66, 0, 66);

    if (!secp256k1_fullagg_pubnonce_load(ctx, ges, nonce)) {
        return 0;
    }
    secp256k1_nonce_points_serialize(out66, ges);
    return 1;
}

int secp256k1_fullagg_aggnonce_parse(const secp256k1_context* ctx, secp256k1_fullagg_aggnonce* nonce, const unsigned char *in66) {
    secp256k1_ge ges[2];

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(nonce != NULL);
    ARG_CHECK(in66 != NULL);

    /* The aggregate nonce consists of two compressed points. There is no
     * encoding for the point at infinity because nonce aggregation fails in
     * that case. */
    if (!secp256k1_nonce_points_parse(ges, in66)) {
        return 0;
    }
    secp256k1_fullagg_aggnonce_save(nonce, ges);
    return 1;
}

int secp256k1_fullagg_aggnonce_serialize(const secp256k1_context* ctx, unsigned char *out66, const secp256k1_fullagg_aggnonce* nonce) {
    secp256k1_ge ges[2];

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out66 != NULL);
    ARG_CHECK(nonce != NULL);
    memset(out66, 0, 66);

    if (!secp256k1_fullagg_aggnonce_load(ctx, ges, nonce)) {
        return 0;
    }
    secp256k1_nonce_points_serialize(out66, ges);
    return 1;
}

int secp256k1_fullagg_partial_sig_parse(const secp256k1_context* ctx, secp256k1_fullagg_partial_sig* sig, const unsigned char *in32) {
    secp256k1_scalar tmp;
    int overflow;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(in32 != NULL);

    memset(sig, 0, sizeof(*sig));

    secp256k1_scalar_set_b32(&tmp, in32, &overflow);
    if (overflow) {
        return 0;
    }
    secp256k1_fullagg_partial_sig_save(sig, &tmp);
    return 1;
}

int secp256k1_fullagg_partial_sig_serialize(const secp256k1_context* ctx, unsigned char *out32, const secp256k1_fullagg_partial_sig* sig) {
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out32 != NULL);
    ARG_CHECK(sig != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&sig->data[0], secp256k1_fullagg_partial_sig_magic, 4) == 0);

    memcpy(out32, &sig->data[4], 32);
    return 1;
}

/* Initializes SHA256 with fixed midstate for "FullAgg/aux" */
static void secp256k1_fullagg_sha256_tagged_aux(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0x4cb4139bul;
    sha->s[1] = 0xac6dd715ul;
    sha->s[2] = 0x46eb898bul;
    sha->s[3] = 0xc13797e2ul;
    sha->s[4] = 0xa7c1aea6ul;
    sha->s[5] = 0x21aab077ul;
    sha->s[6] = 0x6f1746b2ul;
    sha->s[7] = 0x5c2bedd8ul;
    sha->bytes = 64;
}

/* Initializes SHA256 with fixed midstate for "FullAgg/nonce" */
static void secp256k1_fullagg_sha256_tagged_nonce(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0x742a20a9ul;
    sha->s[1] = 0x2c939aaeul;
    sha->s[2] = 0xf8f0f6c0ul;
    sha->s[3] = 0x9b975422ul;
    sha->s[4] = 0xbf5a4f08ul;
    sha->s[5] = 0xe5fa99eeul;
    sha->s[6] = 0xa64c241ful;
    sha->s[7] = 0x5b12ebccul;
    sha->bytes = 64;
}

/* FullAgg nonce generation function:
 * r_i = int(hash_{FullAgg/nonce}(rand || extra_in || i)) mod n */
static void secp256k1_fullagg_nonce_function(const secp256k1_hash_ctx *hash_ctx, secp256k1_scalar *k,
                                             const unsigned char *session_secrand, const unsigned char *seckey32,
                                             const unsigned char *extra_input32) {
    secp256k1_sha256 sha;
    unsigned char rand[32];
    unsigned char i;

    /* Bind nonce to secret key if it was provided, otherwise use secrand directly. */
    if (seckey32 != NULL) {
        secp256k1_fullagg_sha256_tagged_aux(&sha);
        secp256k1_sha256_write(hash_ctx, &sha, session_secrand, 32);
        secp256k1_sha256_finalize(hash_ctx, &sha, rand);
        for (i = 0; i < 32; i++) {
            rand[i] ^= seckey32[i];
        }
    } else {
        memcpy(rand, session_secrand, sizeof(rand));
    }

    /* Write all relevant data into hash for nonce. */
    secp256k1_fullagg_sha256_tagged_nonce(&sha);
    secp256k1_sha256_write(hash_ctx, &sha, rand, sizeof(rand));
    if (extra_input32 != NULL) {
        secp256k1_sha256_write(hash_ctx, &sha, extra_input32, 32);
    }

    /* Generate the two nonces. */
    for (i = 0; i < 2; i++) {
        unsigned char buf[32];
        secp256k1_sha256 sha_tmp = sha;
        secp256k1_sha256_write(hash_ctx, &sha_tmp, &i, 1);
        secp256k1_sha256_finalize(hash_ctx, &sha_tmp, buf);
        secp256k1_scalar_set_b32(&k[i], buf, NULL);

        secp256k1_memclear_explicit(buf, sizeof(buf));
        secp256k1_sha256_clear(&sha_tmp);
    }
    secp256k1_memclear_explicit(rand, sizeof(rand));
    secp256k1_sha256_clear(&sha);
}

/* Internal nonce generation */
static int secp256k1_fullagg_nonce_gen_internal(const secp256k1_context* ctx, secp256k1_fullagg_secnonce *secnonce,
                                                secp256k1_fullagg_pubnonce *pubnonce, const unsigned char *input_nonce,
                                                const unsigned char *seckey, const secp256k1_xonly_pubkey *pubkey,
                                                const unsigned char *extra_input32) {
    secp256k1_scalar k[2];
    secp256k1_ge nonce_pts[2];
    secp256k1_gej nonce_ptj[2];
    int i;
    secp256k1_ge pk;
    int ret = 1;

    ARG_CHECK(pubnonce != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    memset(pubnonce, 0, sizeof(*pubnonce));

    /* Check that the seckey is valid to be able to sign for it later. */
    if (seckey != NULL) {
        secp256k1_scalar sk;
        ret &= secp256k1_scalar_set_b32_seckey(&sk, seckey);
        secp256k1_scalar_clear(&sk);
    }

    if (!secp256k1_xonly_pubkey_load(ctx, &pk, pubkey)) {
        return 0;
    }

    /* Get secret nonce */
    secp256k1_fullagg_nonce_function(&ctx->hash_ctx, k, input_nonce, seckey, extra_input32);
    VERIFY_CHECK(!secp256k1_scalar_is_zero(&k[0]));
    VERIFY_CHECK(!secp256k1_scalar_is_zero(&k[1]));
    secp256k1_fullagg_secnonce_save(secnonce, k, &pk);
    secp256k1_fullagg_secnonce_invalidate(ctx, secnonce, !ret);

    /* Compute pubnonce as R1_i = k[0]*G, R2_i = k[1]*G */
    for (i = 0; i < 2; i++) {
        secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &nonce_ptj[i], &k[i]);
        secp256k1_scalar_clear(&k[i]);
    }

    /* Convert pubnonce from jacobian to affine and mark as non-secret */
    secp256k1_ge_set_all_gej(nonce_pts, nonce_ptj, 2);
    for (i = 0; i < 2; i++) {
        secp256k1_gej_clear(&nonce_ptj[i]);
        secp256k1_declassify(ctx, &nonce_pts[i], sizeof(nonce_pts[i]));
    }

    secp256k1_fullagg_pubnonce_save(pubnonce, nonce_pts);
    return ret;
}

int secp256k1_fullagg_nonce_gen(const secp256k1_context* ctx, secp256k1_fullagg_secnonce *secnonce,
                                secp256k1_fullagg_pubnonce *pubnonce, unsigned char *session_secrand32,
                                const unsigned char *seckey, const secp256k1_xonly_pubkey *pubkey,
                                const unsigned char *extra_input32) {
    int ret = 1;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secnonce != NULL);
    ARG_CHECK(session_secrand32 != NULL);
    memset(secnonce, 0, sizeof(*secnonce));

    ret &= !secp256k1_is_zero_array(session_secrand32, 32);
    secp256k1_declassify(ctx, &ret, sizeof(ret));
    if (ret == 0) {
        secp256k1_fullagg_secnonce_invalidate(ctx, secnonce, 1);
        return 0;
    }

    ret &= secp256k1_fullagg_nonce_gen_internal(ctx, secnonce, pubnonce, session_secrand32,
                                                seckey, pubkey, extra_input32);
    secp256k1_memczero(session_secrand32, 32, ret);
    return ret;
}

int secp256k1_fullagg_nonce_gen_counter(const secp256k1_context* ctx, secp256k1_fullagg_secnonce *secnonce,
                                        secp256k1_fullagg_pubnonce *pubnonce, uint64_t nonrepeating_cnt,
                                        const secp256k1_keypair *keypair,
                                        const unsigned char *extra_input32) {
    unsigned char buf[32] = { 0 };
    unsigned char seckey[32];
    secp256k1_xonly_pubkey pubkey;
    int ret;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secnonce != NULL);
    ARG_CHECK(keypair != NULL);
    memset(secnonce, 0, sizeof(*secnonce));

    secp256k1_write_be64(buf, nonrepeating_cnt);
    /* keypair_sec and keypair_xonly_pub do not fail if the arguments are not NULL */
    ret = secp256k1_keypair_sec(ctx, seckey, keypair);
    VERIFY_CHECK(ret);
    ret = secp256k1_keypair_xonly_pub(ctx, &pubkey, NULL, keypair);
    VERIFY_CHECK(ret);
#ifndef VERIFY
    (void) ret;
#endif

    ret = secp256k1_fullagg_nonce_gen_internal(ctx, secnonce, pubnonce, buf, seckey,
                                               &pubkey, extra_input32);
    secp256k1_memclear_explicit(seckey, sizeof(seckey));
    return ret;
}

static int secp256k1_fullagg_sum_pubnonces(const secp256k1_context* ctx, secp256k1_gej *summed_pubnonces, const secp256k1_fullagg_pubnonce * const* pubnonces, size_t n_pubnonces) {
    size_t i;
    int j;

    secp256k1_gej_set_infinity(&summed_pubnonces[0]);
    secp256k1_gej_set_infinity(&summed_pubnonces[1]);

    for (i = 0; i < n_pubnonces; i++) {
        secp256k1_ge nonce_pts[2];
        if (!secp256k1_fullagg_pubnonce_load(ctx, nonce_pts, pubnonces[i])) {
            return 0;
        }
        for (j = 0; j < 2; j++) {
            secp256k1_gej_add_ge_var(&summed_pubnonces[j], &summed_pubnonces[j], &nonce_pts[j], NULL);
        }
    }
    return 1;
}

/* Aggregate nonces from all signers */
int secp256k1_fullagg_nonce_agg(const secp256k1_context* ctx, secp256k1_fullagg_aggnonce *aggnonce,
                                const secp256k1_fullagg_pubnonce * const* pubnonces, size_t n_pubnonces) {
    secp256k1_gej aggnonce_ptsj[2];
    secp256k1_ge aggnonce_pts[2];
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(aggnonce != NULL);
    ARG_CHECK(pubnonces != NULL);
    ARG_CHECK(n_pubnonces > 0);
    for (i = 0; i < n_pubnonces; i++) {
        ARG_CHECK(pubnonces[i] != NULL);
    }

    if (!secp256k1_fullagg_sum_pubnonces(ctx, aggnonce_ptsj, pubnonces, n_pubnonces)) {
        return 0;
    }
    if (secp256k1_gej_is_infinity(&aggnonce_ptsj[0]) || secp256k1_gej_is_infinity(&aggnonce_ptsj[1])) {
        return 0;
    }

    secp256k1_ge_set_all_gej_var(aggnonce_pts, aggnonce_ptsj, 2);
    secp256k1_fullagg_aggnonce_save(aggnonce, aggnonce_pts);
    return 1;
}

/* Initializes SHA256 with fixed midstate for "FullAgg/noncecoef" */
static void secp256k1_fullagg_sha256_tagged_noncecoef(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0xe7a1238aul;
    sha->s[1] = 0x8d0cb445ul;
    sha->s[2] = 0x82ea69faul;
    sha->s[3] = 0x6cc8a517ul;
    sha->s[4] = 0xfce019a0ul;
    sha->s[5] = 0xdc36828ful;
    sha->s[6] = 0x727f042bul;
    sha->s[7] = 0xf325ff8eul;
    sha->bytes = 64;
}

/* Compute hash_noncecoef: b = H_noncecoef(cbytes(R1) || cbytes(R2) || pk_i || m_i || cbytes(R2_i) for all signers).
 * aggnonce_ser66 is the serialized aggregate nonce cbytes(R1) || cbytes(R2). */
static int secp256k1_fullagg_compute_noncehash(const secp256k1_context* ctx,
                                               unsigned char *noncehash,
                                               const unsigned char *aggnonce_ser66,
                                               const secp256k1_xonly_pubkey * const *pubkeys,
                                               const unsigned char * const *messages,
                                               const secp256k1_fullagg_pubnonce * const *pubnonces,
                                               size_t n_signers) {
    const secp256k1_hash_ctx *hash_ctx = &ctx->hash_ctx;
    unsigned char buf[33];
    size_t i;
    secp256k1_sha256 sha;
    secp256k1_fullagg_sha256_tagged_noncecoef(&sha);

    secp256k1_sha256_write(hash_ctx, &sha, aggnonce_ser66, 66);

    /* Write pk_i || m_i || cbytes(R2_i) for all signers */
    for (i = 0; i < n_signers; i++) {
        secp256k1_ge pk_ge, nonce_pts[2];

        /* Load and write pk_i */
        if (!secp256k1_xonly_pubkey_load(ctx, &pk_ge, pubkeys[i])) {
            return 0;
        }
        secp256k1_fe_get_b32(buf, &pk_ge.x);
        secp256k1_sha256_write(hash_ctx, &sha, buf, 32);

        /* Write m_i */
        secp256k1_sha256_write(hash_ctx, &sha, messages[i], 32);

        /* Load and write R2_i. The pubnonce points are never the point at
         * infinity because parsing and generation do not allow it. */
        if (!secp256k1_fullagg_pubnonce_load(ctx, nonce_pts, pubnonces[i])) {
            return 0;
        }
        secp256k1_ge_serialize33(&nonce_pts[1], buf);
        secp256k1_sha256_write(hash_ctx, &sha, buf, 33);
    }

    secp256k1_sha256_finalize(hash_ctx, &sha, noncehash);
    return 1;
}

/* Initializes SHA256 with fixed midstate for "FullAgg/sig" */
static void secp256k1_fullagg_sha256_tagged_sig(secp256k1_sha256 *sha) {
    secp256k1_sha256_initialize(sha);
    sha->s[0] = 0xd25dfffbul;
    sha->s[1] = 0xd0479fb3ul;
    sha->s[2] = 0x32e0d40eul;
    sha->s[3] = 0x9c4f065aul;
    sha->s[4] = 0xf9bf9e14ul;
    sha->s[5] = 0x8f22cce6ul;
    sha->s[6] = 0x24f00eaeul;
    sha->s[7] = 0xed749b73ul;
    sha->bytes = 64;
}

/* Compute hash_sig: c_i = H_sig(L || bytes(R) || pk_i || m_i) where L is the
 * list of all pk_i || m_i pairs */
static int secp256k1_fullagg_compute_sighash(const secp256k1_context* ctx,
                                             secp256k1_scalar *c_i,
                                             const secp256k1_xonly_pubkey * const *pubkeys,
                                             const unsigned char * const *messages,
                                             size_t n_signers,
                                             const unsigned char *r32,
                                             size_t signer_index) {
    const secp256k1_hash_ctx *hash_ctx = &ctx->hash_ctx;
    unsigned char buf[32];
    unsigned char hash[32];
    size_t i;
    secp256k1_ge pk_ge;
    secp256k1_sha256 sha;
    secp256k1_fullagg_sha256_tagged_sig(&sha);

    /* Write L (list of all pk_i || m_i) */
    for (i = 0; i < n_signers; i++) {
        if (!secp256k1_xonly_pubkey_load(ctx, &pk_ge, pubkeys[i])) {
            return 0;
        }
        secp256k1_fe_get_b32(buf, &pk_ge.x);
        secp256k1_sha256_write(hash_ctx, &sha, buf, 32);
        secp256k1_sha256_write(hash_ctx, &sha, messages[i], 32);
    }

    /* Write bytes(R) */
    secp256k1_sha256_write(hash_ctx, &sha, r32, 32);

    /* Write pk_i for this signer */
    if (!secp256k1_xonly_pubkey_load(ctx, &pk_ge, pubkeys[signer_index])) {
        return 0;
    }
    secp256k1_fe_get_b32(buf, &pk_ge.x);
    secp256k1_sha256_write(hash_ctx, &sha, buf, 32);

    /* Write m_i for this signer */
    secp256k1_sha256_write(hash_ctx, &sha, messages[signer_index], 32);

    secp256k1_sha256_finalize(hash_ctx, &sha, hash);
    secp256k1_scalar_set_b32(c_i, hash, NULL);

    return 1;
}

static const unsigned char secp256k1_fullagg_session_magic[4] = { 0xf1, 0x1a, 0x99, 0x04 };

static void secp256k1_fullagg_session_save(secp256k1_fullagg_session *session,
                                           const unsigned char *fin_nonce,
                                           int fin_nonce_parity,
                                           const secp256k1_scalar *noncecoef,
                                           const unsigned char *aggnonce_ser66,
                                           size_t n_signers) {
    unsigned char *ptr = session->data;

    memcpy(ptr, secp256k1_fullagg_session_magic, 4);
    ptr += 4;
    memcpy(ptr, fin_nonce, 32);
    ptr += 32;
    *ptr = (unsigned char)fin_nonce_parity;
    ptr += 1;
    secp256k1_scalar_get_b32(ptr, noncecoef);
    ptr += 32;
    secp256k1_write_be64(ptr, (uint64_t)n_signers);
    ptr += 8;
    memcpy(ptr, aggnonce_ser66, 66);
}

static int secp256k1_fullagg_session_load(const secp256k1_context* ctx,
                                          unsigned char *fin_nonce,
                                          int *fin_nonce_parity,
                                          secp256k1_scalar *noncecoef,
                                          unsigned char *aggnonce_ser66,
                                          size_t *n_signers,
                                          const secp256k1_fullagg_session *session) {
    const unsigned char *ptr = session->data;

    ARG_CHECK(secp256k1_memcmp_var(ptr, secp256k1_fullagg_session_magic, 4) == 0);
    ptr += 4;
    memcpy(fin_nonce, ptr, 32);
    ptr += 32;
    *fin_nonce_parity = (int)*ptr;
    ptr += 1;
    secp256k1_scalar_set_b32(noncecoef, ptr, NULL);
    ptr += 32;
    *n_signers = (size_t)secp256k1_read_be64(ptr);
    ptr += 8;
    memcpy(aggnonce_ser66, ptr, 66);

    return 1;
}


/* Initialize a FullAgg session. This implements GetSessionValues. */
int secp256k1_fullagg_session_init(const secp256k1_context* ctx, secp256k1_fullagg_session *session,
                                   const secp256k1_fullagg_aggnonce *aggnonce,
                                   const secp256k1_xonly_pubkey * const *pubkeys,
                                   const unsigned char * const *messages,
                                   const secp256k1_fullagg_pubnonce * const *pubnonces,
                                   size_t n_signers) {
    secp256k1_ge aggnonce_pts[2];
    secp256k1_ge r;
    secp256k1_gej rj;
    unsigned char noncehash[32];
    unsigned char fin_nonce[32];
    unsigned char aggnonce_ser[66];
    int fin_nonce_parity;
    secp256k1_scalar noncecoef;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(session != NULL);
    ARG_CHECK(aggnonce != NULL);
    ARG_CHECK(pubkeys != NULL);
    ARG_CHECK(messages != NULL);
    ARG_CHECK(pubnonces != NULL);
    ARG_CHECK(n_signers > 0);
    for (i = 0; i < n_signers; i++) {
        ARG_CHECK(pubkeys[i] != NULL);
        ARG_CHECK(messages[i] != NULL);
        ARG_CHECK(pubnonces[i] != NULL);
    }

    if (!secp256k1_fullagg_aggnonce_load(ctx, aggnonce_pts, aggnonce)) {
        return 0;
    }
    secp256k1_ge_serialize33(&aggnonce_pts[0], &aggnonce_ser[0]);
    secp256k1_ge_serialize33(&aggnonce_pts[1], &aggnonce_ser[33]);

    /* Compute nonce coefficient b */
    if (!secp256k1_fullagg_compute_noncehash(ctx, noncehash, aggnonce_ser,
                                             pubkeys, messages, pubnonces, n_signers)) {
        return 0;
    }
    secp256k1_scalar_set_b32(&noncecoef, noncehash, NULL);

    /* Compute final nonce R = R1 + b*R2 and fail if it is the point at
     * infinity */
    secp256k1_effective_nonce(&rj, aggnonce_pts, &noncecoef);
    secp256k1_ge_set_gej(&r, &rj);
    if (secp256k1_ge_is_infinity(&r)) {
        return 0;
    }

    secp256k1_fe_normalize_var(&r.x);
    secp256k1_fe_normalize_var(&r.y);
    secp256k1_fe_get_b32(fin_nonce, &r.x);
    fin_nonce_parity = secp256k1_fe_is_odd(&r.y);

    secp256k1_fullagg_session_save(session, fin_nonce, fin_nonce_parity, &noncecoef, aggnonce_ser, n_signers);
    return 1;
}

/* Create a partial signature */
int secp256k1_fullagg_partial_sign(const secp256k1_context* ctx, secp256k1_fullagg_partial_sig *partial_sig,
                                   secp256k1_fullagg_secnonce *secnonce, const secp256k1_keypair *keypair,
                                   const unsigned char *msg32,
                                   const secp256k1_fullagg_session *session,
                                   const secp256k1_xonly_pubkey * const *pubkeys,
                                   const unsigned char * const *messages,
                                   const secp256k1_fullagg_pubnonce * const *pubnonces,
                                   size_t n_signers,
                                   size_t signer_index) {
    secp256k1_scalar sk;
    secp256k1_ge pk, keypair_pk;
    secp256k1_scalar k[2];
    secp256k1_scalar s, c_i;
    unsigned char fin_nonce[32];
    int fin_nonce_parity;
    secp256k1_scalar noncecoef;
    unsigned char aggnonce_ser[66];
    size_t n_session_signers;
    size_t i;
    int ret;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secnonce != NULL);
    ARG_CHECK(partial_sig != NULL);
    ARG_CHECK(keypair != NULL);
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(session != NULL);
    ARG_CHECK(pubkeys != NULL);
    ARG_CHECK(messages != NULL);
    ARG_CHECK(pubnonces != NULL);
    ARG_CHECK(n_signers > 0);
    for (i = 0; i < n_signers; i++) {
        ARG_CHECK(pubkeys[i] != NULL);
        ARG_CHECK(messages[i] != NULL);
        ARG_CHECK(pubnonces[i] != NULL);
    }
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));

    /* Load and invalidate secnonce */
    ret = secp256k1_fullagg_secnonce_load(ctx, k, &pk, secnonce);
    /* Always clear the secnonce to avoid nonce reuse */
    secp256k1_memzero_explicit(secnonce, sizeof(*secnonce));
    if (!ret) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }

    if (!secp256k1_keypair_load(ctx, &sk, &keypair_pk, keypair)) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }

    /* Verify the keypair matches the secnonce. The secnonce stores the point
     * with even Y, so only the X coordinates are compared. */
    if (!secp256k1_fe_equal(&pk.x, &keypair_pk.x)) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }

    /* d' = d if has_even_y(P), otherwise d' = n - d */
    secp256k1_fe_normalize_var(&keypair_pk.y);
    if (secp256k1_fe_is_odd(&keypair_pk.y)) {
        secp256k1_scalar_negate(&sk, &sk);
    }

    if (!secp256k1_fullagg_session_load(ctx, fin_nonce, &fin_nonce_parity, &noncecoef, aggnonce_ser, &n_session_signers, session)) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }
    ARG_CHECK(n_signers == n_session_signers);

    if (signer_index >= n_signers) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }

    /* Verify that the public key at the signer's position matches the
     * signer's key and that the message at the signer's position is the
     * message the signer intends to sign. Omitting the message check would
     * let a malicious coordinator substitute a different message at the
     * signer's position. */
    {
        secp256k1_ge signer_pk;
        if (!secp256k1_xonly_pubkey_load(ctx, &signer_pk, pubkeys[signer_index])) {
            secp256k1_partial_sign_clear(&sk, k);
            return 0;
        }
        if (!secp256k1_fe_equal(&keypair_pk.x, &signer_pk.x)) {
            secp256k1_partial_sign_clear(&sk, k);
            return 0;
        }
    }
    if (secp256k1_memcmp_var(messages[signer_index], msg32, 32) != 0) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }

    /* Verify this signer's R2 appears exactly once at the correct index. The
     * uniqueness check is security-critical: if the signer's R2 appears at a
     * second index, signing enables a key extraction attack (see Section 4.2
     * of the DahLIAS paper). */
    {
        secp256k1_ge nonce_pts[2];
        secp256k1_gej r2j;
        secp256k1_ge r2_self;
        int found_count = 0;
        int found_index = -1;
        size_t j;

        /* Compute our R2 from k[1]. The point is public because it has been
         * broadcast as part of the pubnonce, so it can be declassified. */
        secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &r2j, &k[1]);
        secp256k1_ge_set_gej(&r2_self, &r2j);
        secp256k1_declassify(ctx, &r2_self, sizeof(r2_self));
        secp256k1_gej_clear(&r2j);

        /* Check all pubnonces for our R2. The pubnonce points are never the
         * point at infinity, so a match requires r2_self not to be infinity
         * either. */
        for (j = 0; j < n_signers; j++) {
            if (!secp256k1_fullagg_pubnonce_load(ctx, nonce_pts, pubnonces[j])) {
                secp256k1_partial_sign_clear(&sk, k);
                return 0;
            }

            if (!secp256k1_ge_is_infinity(&r2_self)
                && secp256k1_fe_equal(&r2_self.x, &nonce_pts[1].x)
                && secp256k1_fe_equal(&r2_self.y, &nonce_pts[1].y)) {
                found_count++;
                found_index = (int)j;
            }
        }

        /* R2 must appear exactly once and at the signer's index */
        if (found_count != 1 || found_index != (int)signer_index) {
            secp256k1_partial_sign_clear(&sk, k);
            return 0;
        }
    }

    /* Verify that the session was initialized with the same aggregate nonce
     * and the same lists of public keys, messages and pubnonces by
     * recomputing the nonce coefficient */
    {
        unsigned char noncehash[32];
        secp256k1_scalar noncecoef_check;
        if (!secp256k1_fullagg_compute_noncehash(ctx, noncehash, aggnonce_ser, pubkeys,
                                                 messages, pubnonces, n_signers)) {
            secp256k1_partial_sign_clear(&sk, k);
            return 0;
        }
        secp256k1_scalar_set_b32(&noncecoef_check, noncehash, NULL);
        if (!secp256k1_scalar_eq(&noncecoef_check, &noncecoef)) {
            secp256k1_partial_sign_clear(&sk, k);
            return 0;
        }
    }

    /* e = 1 if has_even_y(R), otherwise e = n - 1. Multiply the secret
     * nonces by e. */
    if (fin_nonce_parity) {
        secp256k1_scalar_negate(&k[0], &k[0]);
        secp256k1_scalar_negate(&k[1], &k[1]);
    }

    /* Compute signer's challenge c_i */
    if (!secp256k1_fullagg_compute_sighash(ctx, &c_i, pubkeys, messages, n_signers,
                                           fin_nonce, signer_index)) {
        secp256k1_partial_sign_clear(&sk, k);
        return 0;
    }

    /* Compute s_i = e*(k1_i + b*k2_i) + c_i*d' */
    secp256k1_scalar_mul(&s, &noncecoef, &k[1]);  /* b*k2_i */
    secp256k1_scalar_add(&s, &s, &k[0]);          /* + k1_i */
    secp256k1_scalar_mul(&k[0], &c_i, &sk);       /* c_i*d' (reuse k[0]) */
    secp256k1_scalar_add(&s, &s, &k[0]);          /* + c_i*d' */

    secp256k1_fullagg_partial_sig_save(partial_sig, &s);

    /* Clear sensitive data */
    secp256k1_partial_sign_clear(&sk, k);
    secp256k1_scalar_clear(&s);

    return 1;
}

/* Verify a partial signature. The aggregate nonce and the session values are
 * recomputed from the pubnonces so that the result cannot be influenced by
 * the coordinator. */
int secp256k1_fullagg_partial_sig_verify(const secp256k1_context* ctx, const secp256k1_fullagg_partial_sig *partial_sig,
                                         const secp256k1_fullagg_pubnonce * const *pubnonces,
                                         const secp256k1_xonly_pubkey * const *pubkeys,
                                         const unsigned char * const *messages,
                                         size_t n_signers,
                                         size_t signer_index) {
    secp256k1_gej aggnonce_ptsj[2];
    secp256k1_ge aggnonce_pts[2];
    unsigned char noncehash[32];
    unsigned char fin_nonce[32];
    unsigned char aggnonce_ser[66];
    int fin_nonce_parity;
    secp256k1_scalar noncecoef;
    secp256k1_scalar s, c_i;
    secp256k1_ge pk, r;
    secp256k1_ge nonce_pts[2];
    secp256k1_gej rj, pkj, tmp;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(partial_sig != NULL);
    ARG_CHECK(pubnonces != NULL);
    ARG_CHECK(pubkeys != NULL);
    ARG_CHECK(messages != NULL);
    ARG_CHECK(n_signers > 0);
    for (i = 0; i < n_signers; i++) {
        ARG_CHECK(pubnonces[i] != NULL);
        ARG_CHECK(pubkeys[i] != NULL);
        ARG_CHECK(messages[i] != NULL);
    }

    if (signer_index >= n_signers) {
        return 0;
    }

    if (!secp256k1_fullagg_partial_sig_load(ctx, &s, partial_sig)) {
        return 0;
    }

    /* aggnonce = NonceAgg(pubnonce_0..u-1) */
    if (!secp256k1_fullagg_sum_pubnonces(ctx, aggnonce_ptsj, pubnonces, n_signers)) {
        return 0;
    }
    if (secp256k1_gej_is_infinity(&aggnonce_ptsj[0]) || secp256k1_gej_is_infinity(&aggnonce_ptsj[1])) {
        return 0;
    }
    secp256k1_ge_set_all_gej_var(aggnonce_pts, aggnonce_ptsj, 2);
    secp256k1_ge_serialize33(&aggnonce_pts[0], &aggnonce_ser[0]);
    secp256k1_ge_serialize33(&aggnonce_pts[1], &aggnonce_ser[33]);

    /* (R, b) = GetSessionValues(aggnonce, pk_0..u-1, m_0..u-1, pubnonce_0..u-1) */
    if (!secp256k1_fullagg_compute_noncehash(ctx, noncehash, aggnonce_ser,
                                             pubkeys, messages, pubnonces, n_signers)) {
        return 0;
    }
    secp256k1_scalar_set_b32(&noncecoef, noncehash, NULL);
    secp256k1_effective_nonce(&rj, aggnonce_pts, &noncecoef);
    secp256k1_ge_set_gej(&r, &rj);
    if (secp256k1_ge_is_infinity(&r)) {
        return 0;
    }
    secp256k1_fe_normalize_var(&r.x);
    secp256k1_fe_normalize_var(&r.y);
    secp256k1_fe_get_b32(fin_nonce, &r.x);
    fin_nonce_parity = secp256k1_fe_is_odd(&r.y);

    /* Compute effective nonce of this signer: R'_i = R1_i + b*R2_i */
    if (!secp256k1_fullagg_pubnonce_load(ctx, nonce_pts, pubnonces[signer_index])) {
        return 0;
    }
    secp256k1_effective_nonce(&rj, nonce_pts, &noncecoef);

    /* Multiply by e: negate if R has odd Y */
    if (fin_nonce_parity) {
        secp256k1_gej_neg(&rj, &rj);
    }

    /* P_i = lift_x(int(pk_i)) */
    if (!secp256k1_xonly_pubkey_load(ctx, &pk, pubkeys[signer_index])) {
        return 0;
    }

    /* Compute signer's challenge c_i */
    if (!secp256k1_fullagg_compute_sighash(ctx, &c_i, pubkeys, messages, n_signers,
                                           fin_nonce, signer_index)) {
        return 0;
    }

    /* Verify: s_i*G = e*R'_i + c_i*P_i */
    secp256k1_scalar_negate(&s, &s);
    secp256k1_gej_set_ge(&pkj, &pk);
    secp256k1_ecmult(&tmp, &pkj, &c_i, &s);
    secp256k1_gej_add_var(&tmp, &tmp, &rj, NULL);

    return secp256k1_gej_is_infinity(&tmp);
}

/* Aggregate partial signatures */
int secp256k1_fullagg_partial_sig_agg(const secp256k1_context* ctx, unsigned char *sig64,
                                      const secp256k1_fullagg_session *session,
                                      const secp256k1_fullagg_partial_sig * const *partial_sigs,
                                      size_t n_sigs) {
    unsigned char fin_nonce[32];
    unsigned char aggnonce_ser[66];
    int fin_nonce_parity;
    secp256k1_scalar noncecoef;
    size_t n_signers;
    secp256k1_scalar s_agg;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(session != NULL);
    ARG_CHECK(partial_sigs != NULL);
    ARG_CHECK(n_sigs > 0);
    for (i = 0; i < n_sigs; i++) {
        ARG_CHECK(partial_sigs[i] != NULL);
    }

    if (!secp256k1_fullagg_session_load(ctx, fin_nonce, &fin_nonce_parity, &noncecoef, aggnonce_ser, &n_signers, session)) {
        return 0;
    }

    ARG_CHECK(n_sigs == n_signers);

    /* Aggregate all the s values */
    secp256k1_scalar_set_int(&s_agg, 0);
    for (i = 0; i < n_sigs; i++) {
        secp256k1_scalar s_i;
        if (!secp256k1_fullagg_partial_sig_load(ctx, &s_i, partial_sigs[i])) {
            return 0;
        }
        secp256k1_scalar_add(&s_agg, &s_agg, &s_i);
    }

    /* Output aggregate signature */
    memcpy(sig64, fin_nonce, 32);
    secp256k1_scalar_get_b32(&sig64[32], &s_agg);

    return 1;
}

typedef struct {
    const secp256k1_context *ctx;
    /* Hash state after writing the tagged hash midstate and L, the list of
     * all pk_i || m_i */
    secp256k1_sha256 sha_l;
    const unsigned char *sig64;
    const secp256k1_xonly_pubkey * const *pubkeys;
    const unsigned char * const *messages;
} secp256k1_fullagg_verify_ecmult_data;

/* Provides the c_i*P_i terms for the verification multi-exponentiation */
static int secp256k1_fullagg_verify_ecmult_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_fullagg_verify_ecmult_data *ecmult_data = (secp256k1_fullagg_verify_ecmult_data *)data;
    const secp256k1_hash_ctx *hash_ctx = &ecmult_data->ctx->hash_ctx;
    secp256k1_sha256 sha = ecmult_data->sha_l;
    unsigned char buf[32];

    if (!secp256k1_xonly_pubkey_load(ecmult_data->ctx, pt, ecmult_data->pubkeys[idx])) {
        return 0;
    }
    /* c_i = hash_sig(L || bytes(R) || pk_i || m_i) */
    secp256k1_sha256_write(hash_ctx, &sha, ecmult_data->sig64, 32);
    secp256k1_fe_get_b32(buf, &pt->x);
    secp256k1_sha256_write(hash_ctx, &sha, buf, 32);
    secp256k1_sha256_write(hash_ctx, &sha, ecmult_data->messages[idx], 32);
    secp256k1_sha256_finalize(hash_ctx, &sha, buf);
    secp256k1_scalar_set_b32(sc, buf, NULL);
    return 1;
}

/* Verify a FullAgg aggregate signature */
int secp256k1_fullagg_verify(const secp256k1_context* ctx, const unsigned char *sig64,
                             const secp256k1_xonly_pubkey * const *pubkeys,
                             const unsigned char * const *messages,
                             size_t n_signers) {
    const secp256k1_hash_ctx *hash_ctx;
    secp256k1_fullagg_verify_ecmult_data ecmult_data;
    secp256k1_scalar s;
    secp256k1_ge r;
    secp256k1_gej resj;
    unsigned char buf[32];
    size_t i;
    int overflow;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(pubkeys != NULL);
    ARG_CHECK(messages != NULL);
    ARG_CHECK(n_signers > 0);
    for (i = 0; i < n_signers; i++) {
        ARG_CHECK(pubkeys[i] != NULL);
        ARG_CHECK(messages[i] != NULL);
    }

    /* Parse signature */
    if (!secp256k1_fe_set_b32_limit(&r.x, sig64)) {
        return 0;
    }
    secp256k1_scalar_set_b32(&s, &sig64[32], &overflow);
    if (overflow) {
        return 0;
    }

    /* R = lift_x(int(r)) */
    if (!secp256k1_ge_set_xo_var(&r, &r.x, 0)) {
        return 0;
    }

    /* Precompute the hash state after writing L, the list of all pk_i || m_i.
     * Computing it once here instead of once per challenge avoids hashing
     * quadratic in the number of signers. */
    hash_ctx = &ctx->hash_ctx;
    secp256k1_fullagg_sha256_tagged_sig(&ecmult_data.sha_l);
    for (i = 0; i < n_signers; i++) {
        secp256k1_ge pk_ge;
        if (!secp256k1_xonly_pubkey_load(ctx, &pk_ge, pubkeys[i])) {
            return 0;
        }
        secp256k1_fe_get_b32(buf, &pk_ge.x);
        secp256k1_sha256_write(hash_ctx, &ecmult_data.sha_l, buf, 32);
        secp256k1_sha256_write(hash_ctx, &ecmult_data.sha_l, messages[i], 32);
    }
    ecmult_data.ctx = ctx;
    ecmult_data.sig64 = sig64;
    ecmult_data.pubkeys = pubkeys;
    ecmult_data.messages = messages;

    /* Compute resj = -s*G + sum(c_i*P_i). Note that the public keys are never
     * the point at infinity because they are the result of lift_x. */
    /* TODO: actually use optimized ecmult_multi algorithms by providing a
     * scratch space */
    secp256k1_scalar_negate(&s, &s);
    if (!secp256k1_ecmult_multi_var(&ctx->error_callback, NULL, &resj, &s, secp256k1_fullagg_verify_ecmult_callback, (void *) &ecmult_data, n_signers)) {
        return 0;
    }

    /* Verify: s*G = R + sum(c_i*P_i) */
    secp256k1_gej_add_ge_var(&resj, &resj, &r, NULL);
    return secp256k1_gej_is_infinity(&resj);
}

#endif /* SECP256K1_MODULE_FULLAGG_MAIN_H */
