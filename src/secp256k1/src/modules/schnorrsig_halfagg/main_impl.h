#ifndef SECP256K1_MODULE_SCHNORRSIG_HALFAGG_MAIN_H
#define SECP256K1_MODULE_SCHNORRSIG_HALFAGG_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_schnorrsig.h"
#include "../../../include/secp256k1_schnorrsig_halfagg.h"
#include "../../hash.h"

/* Initializes SHA256 with fixed midstate. This midstate was computed by applying
 * SHA256 to SHA256("HalfAgg/randomizer")||SHA256("HalfAgg/randomizer"). */
static void secp256k1_schnorrsig_sha256_tagged_aggregation(secp256k1_sha256 *sha) {
   secp256k1_sha256_initialize(sha);
    sha->s[0] = 0xd11f5532ul;
    sha->s[1] = 0xfa57f70ful;
    sha->s[2] = 0x5db0d728ul;
    sha->s[3] = 0xf806ffe1ul;
    sha->s[4] = 0x1d4db069ul;
    sha->s[5] = 0xb4d587e1ul;
    sha->s[6] = 0x50451c2aul;
    sha->s[7] = 0x10fb63e9ul;

    sha->bytes = 64;
}

int secp256k1_schnorrsig_inc_aggregate(const secp256k1_context *ctx, unsigned char *aggsig, size_t *aggsig_len, const secp256k1_xonly_pubkey *all_pubkeys, const unsigned char *all_msgs32, const unsigned char *new_sigs64, size_t n_before, size_t n_new) {
    size_t i;
    size_t n;
    secp256k1_sha256 hash;
    secp256k1_scalar s;
    int overflow;
    const secp256k1_hash_ctx *hash_ctx;

    VERIFY_CHECK(ctx != NULL);
    hash_ctx = &ctx->hash_ctx;
    ARG_CHECK(aggsig != NULL);
    ARG_CHECK(aggsig_len != NULL);
    ARG_CHECK(new_sigs64 != NULL || n_new == 0);

    /* Check that aggsig_len is large enough, i.e. aggsig_len >= 32*(n+1) */
    n = n_before + n_new;
    ARG_CHECK(n >= n_before);
    ARG_CHECK(all_pubkeys != NULL || n == 0);
    ARG_CHECK(all_msgs32 != NULL || n == 0);
    if ((*aggsig_len / 32) <= 0 || ((*aggsig_len / 32) - 1) < n) {
        return 0;
    }

    /* Prepare hash with common prefix. The prefix is the tag and       */
    /* r_0 || pk_0 || m_0 || .... ||  r_{n'-1} || pk_{n'-1} || m_{n'-1} */
    /* where n' = n_before                                              */
    secp256k1_schnorrsig_sha256_tagged_aggregation(&hash);
    for (i = 0; i < n_before; ++i) {
        /* serialize pk_i */
        unsigned char pk_ser[32];
        if (!secp256k1_xonly_pubkey_serialize(ctx, pk_ser, &all_pubkeys[i])) {
            return 0;
        }
        /* write r_i */
        secp256k1_sha256_write(hash_ctx, &hash, &aggsig[i*32], 32);
        /* write pk_i */
        secp256k1_sha256_write(hash_ctx, &hash, pk_ser, 32);
        /* write m_i*/
        secp256k1_sha256_write(hash_ctx, &hash, &all_msgs32[i*32], 32);
    }

    /* Compute s = s_old + sum_{i = n_before}^{n} z_i*s_i */
    /* where s_old = 0 if n_before = 0 */
    secp256k1_scalar_set_int(&s, 0);
    if (n_before > 0) {
        /* Fail if the s in the input aggsig is out of range, i.e. >= n. */
        secp256k1_scalar_set_b32(&s, &aggsig[n_before*32], &overflow);
        if (overflow) {
            return 0;
        }
    }
    for (i = n_before; i < n; ++i) {
        unsigned char pk_ser[32];
        unsigned char hashoutput[32];
        secp256k1_sha256 hashcopy;
        secp256k1_scalar si;
        secp256k1_scalar zi;

        /* Step 0: Serialize pk_i into pk_ser   */
        if (!secp256k1_xonly_pubkey_serialize(ctx, pk_ser, &all_pubkeys[i])) {
            return 0;
        }

        /* Step 1: z_i = TaggedHash(...) */
        /* 1.a) Write into hash r_i, pk_i, m_i, r_i */
        secp256k1_sha256_write(hash_ctx, &hash, &new_sigs64[(i-n_before)*64], 32);
        secp256k1_sha256_write(hash_ctx, &hash, pk_ser, 32);
        secp256k1_sha256_write(hash_ctx, &hash, &all_msgs32[i*32], 32);
        /* 1.b) Copy the hash */
        hashcopy = hash;
        /* 1.c) Finalize the copy to get zi*/
        secp256k1_sha256_finalize(hash_ctx, &hashcopy, hashoutput);
        /* Note: No need to check overflow, comes from hash */
        secp256k1_scalar_set_b32(&zi, hashoutput, NULL);

        /* Step 2: s := s + zi*si */
        /* except if i == 0, then zi = 1 implicitly */
        /* Fail if the s in the input signature is out of range, i.e. >= n. */
        secp256k1_scalar_set_b32(&si, &new_sigs64[(i-n_before)*64+32], &overflow);
        if (overflow) {
            return 0;
        }
        if (i != 0) secp256k1_scalar_mul(&si, &si, &zi);
        secp256k1_scalar_add(&s, &s, &si);
    }

    /* copy new rs into aggsig */
    for (i = n_before; i < n; ++i) {
        memcpy(&aggsig[i*32], &new_sigs64[(i-n_before)*64], 32);
    }
    /* copy new s into aggsig */
    secp256k1_scalar_get_b32(&aggsig[n*32], &s);
    *aggsig_len = 32 * (1 + n);
    return 1;
}

int secp256k1_schnorrsig_aggregate(const secp256k1_context *ctx, unsigned char *aggsig, size_t *aggsig_len, const secp256k1_xonly_pubkey *pubkeys, const unsigned char *msgs32, const unsigned char *sigs64, size_t n) {
    return secp256k1_schnorrsig_inc_aggregate(ctx, aggsig, aggsig_len, pubkeys, msgs32, sigs64, 0, n);
}

int secp256k1_schnorrsig_aggverify(const secp256k1_context *ctx, const secp256k1_xonly_pubkey *pubkeys, const unsigned char *msgs32, size_t n, const unsigned char *aggsig, size_t aggsig_len) {
    size_t i;
    secp256k1_gej lhs, rhs;
    secp256k1_scalar s;
    secp256k1_sha256 hash;
    int overflow;
    const secp256k1_hash_ctx *hash_ctx;

    VERIFY_CHECK(ctx != NULL);
    hash_ctx = &ctx->hash_ctx;
    ARG_CHECK(pubkeys != NULL || n == 0);
    ARG_CHECK(msgs32 != NULL || n == 0);
    ARG_CHECK(aggsig != NULL);

    /* Check that aggsig_len is correct, i.e., aggsig_len = 32*(n+1) */
    if ((aggsig_len / 32) <= 0 || ((aggsig_len / 32)-1) != n || (aggsig_len % 32) != 0) {
        return 0;
    }

    /* Compute the rhs:               */
    /* Set rhs = 0                    */
    /* For each i in 0,.., n-1, do:   */
    /*     (1) z_i = TaggedHash(...)  */
    /*     (2) T_i = R_i+e_i*P_i      */
    /*     (3) rhs = rhs + z_i*T_i    */
    secp256k1_gej_set_infinity(&rhs);
    secp256k1_schnorrsig_sha256_tagged_aggregation(&hash);
    for (i = 0; i < n; ++i) {
        secp256k1_fe rx;
        secp256k1_ge rp, pp;
        secp256k1_scalar ei;
        secp256k1_gej ppj, ti;

        unsigned char pk_ser[32];
        unsigned char hashoutput[32];
        secp256k1_sha256 hashcopy;
        secp256k1_scalar zi;

        /* Step 0: Serialize pk_i into pk_ser   */
        /* We need that in Step 1 and in Step 2 */
        if (!secp256k1_xonly_pubkey_load(ctx, &pp, &pubkeys[i])) {
            return 0;
        }
        secp256k1_fe_get_b32(pk_ser, &pp.x);

        /* Step 1: z_i = TaggedHash(...) */
        /* 1.a) Write into hash r_i, pk_i, m_i, r_i */
        secp256k1_sha256_write(hash_ctx, &hash, &aggsig[i*32], 32);
        secp256k1_sha256_write(hash_ctx, &hash, pk_ser, 32);
        secp256k1_sha256_write(hash_ctx, &hash, &msgs32[i*32], 32);
        /* 1.b) Copy the hash */
        hashcopy = hash;
        /* 1.c) Finalize the copy to get zi*/
        secp256k1_sha256_finalize(hash_ctx, &hashcopy, hashoutput);
        secp256k1_scalar_set_b32(&zi, hashoutput, NULL);

        /* Step 2: T_i = R_i+e_i*P_i */
        /* 2.a) R_i = lift_x(int(r_i)); fail if that fails */
        if (!secp256k1_fe_set_b32_limit(&rx, &aggsig[i*32])) {
            return 0;
        }
        if (!secp256k1_ge_set_xo_var(&rp, &rx, 0)) {
            return 0;
        }

        /* 2.b) e_i = int(hash_{BIP0340/challenge}(bytes(r_i) || pk_i || m_i)) mod n */
        secp256k1_schnorrsig_challenge(hash_ctx, &ei, &aggsig[i*32], &msgs32[i*32], 32, pk_ser);
        secp256k1_gej_set_ge(&ppj, &pp);
        /* 2.c) T_i = R_i + e_i*P_i */
        secp256k1_ecmult(&ti, &ppj, &ei, NULL);
        secp256k1_gej_add_ge_var(&ti, &ti, &rp, NULL);

        /* Step 3: rhs = rhs + zi*T_i  */
        /* Note that if i == 0, then zi = 1 implicitly */
        if (i != 0) secp256k1_ecmult(&ti, &ti, &zi, NULL);
        secp256k1_gej_add_var(&rhs, &rhs, &ti, NULL);
    }

    /* Compute lhs = s*G. s is public (part of the aggregate signature), so we
     * use the variable-time secp256k1_ecmult with the precomputed G table
     * instead of constant-time secp256k1_ecmult_gen. This is faster and needs
     * no ecmult_gen context, so aggverify works with secp256k1_context_static. */
    secp256k1_scalar_set_b32(&s, &aggsig[n*32], &overflow);
    if (overflow) {
        return 0;
    }
    secp256k1_ecmult(&lhs, &rhs, &secp256k1_scalar_zero, &s);

    /* Check that lhs == rhs */
    secp256k1_gej_neg(&lhs, &lhs);
    secp256k1_gej_add_var(&lhs, &lhs, &rhs, NULL);
    return secp256k1_gej_is_infinity(&lhs);
}

#endif
