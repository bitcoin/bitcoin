/**********************************************************************
 * Copyright (c) 2018 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_BULLETPROOF_RANGEPROOF_IMPL
#define SECP256K1_MODULE_BULLETPROOF_RANGEPROOF_IMPL

#include "modules/bulletproofs/inner_product_impl.h"
#include "modules/bulletproofs/util.h"
#include "group.h"

#define MAX_NBITS	64

typedef struct {
    secp256k1_scalar yinv;
    secp256k1_scalar yinvn;
    secp256k1_scalar z;
    secp256k1_scalar z_randomized;
    secp256k1_scalar zsq;
    secp256k1_scalar g_exponent;
    secp256k1_scalar negz;
    secp256k1_scalar x;
    secp256k1_ge a;
    secp256k1_ge s;
    size_t n;
    /* eq (61) stuff */
    size_t count;
    secp256k1_scalar randomizer61;
    secp256k1_scalar y;
    secp256k1_scalar t;
    const secp256k1_ge *asset;
    const secp256k1_ge *commit;
    const uint64_t *min_value;
    size_t n_commits;
    secp256k1_ge t1;
    secp256k1_ge t2;
} secp256k1_bulletproof_vfy_ecmult_context;

static int secp256k1_bulletproof_rangeproof_vfy_callback(secp256k1_scalar *sc, secp256k1_ge *pt, secp256k1_scalar *randomizer, size_t idx, void *data) {
    secp256k1_bulletproof_vfy_ecmult_context *ctx = (secp256k1_bulletproof_vfy_ecmult_context *) data;

    if (idx == 0) {
        secp256k1_scalar_mul(&ctx->g_exponent, &ctx->negz, randomizer);
        secp256k1_scalar_mul(&ctx->z_randomized, &ctx->z, randomizer);
    }

    if (idx < ctx->n) {
        *sc = ctx->g_exponent;
    } else if (idx < 2 * ctx->n) {
        const size_t nbits = ctx->n / ctx->n_commits;
        const size_t commit_idx = (idx - ctx->n) / nbits;
        const size_t bit_idx = (idx - ctx->n) % nbits;

        if (bit_idx == 0) {
            size_t i;
            secp256k1_scalar tmp;
            secp256k1_scalar_mul(&tmp, &ctx->z, &ctx->yinvn);
            secp256k1_scalar_sqr(&ctx->zsq, &ctx->z);
            for (i = 0; i < commit_idx; i++) {
                secp256k1_scalar_mul(&ctx->zsq, &ctx->zsq, &tmp);
            }
            secp256k1_scalar_mul(&ctx->zsq, &ctx->zsq, randomizer);
        }
        secp256k1_scalar_add(sc, &ctx->zsq, &ctx->z_randomized);

        secp256k1_scalar_mul(&ctx->zsq, &ctx->zsq, &ctx->yinv);
        secp256k1_scalar_add(&ctx->zsq, &ctx->zsq, &ctx->zsq);
    } else {
        switch(ctx->count) {
        /* S^x in eq (62) */
        case 2:
            *sc = ctx->x;
            *pt = ctx->s;
            break;
        /* A in eq (62) */
        case 1:
            *pt = ctx->a;
            secp256k1_scalar_set_int(sc, 1);
            break;
        /* G^[k(y, z) + sum_i y^i - t] from eq (61) */
        case 0: {
            size_t i;
            secp256k1_scalar yn;
            secp256k1_scalar twosum;
            secp256k1_scalar tmp;

            secp256k1_scalar_clear(&twosum);
            secp256k1_scalar_clear(&yn);
            secp256k1_scalar_set_int(&tmp, 1);

            secp256k1_scalar_sqr(&ctx->zsq, &ctx->z);  /* need to re-set this */
            secp256k1_scalar_negate(sc, &ctx->zsq);  /* -z^2 */
            secp256k1_scalar_add(sc, sc, &ctx->z);   /* z - z^2 */

            for (i = 0; i < ctx->n_commits; i++) {
                const size_t nbits = ctx->n / ctx->n_commits;
                secp256k1_scalar negzn;
                secp256k1_scalar twon;
                size_t j;

                secp256k1_scalar_clear(&twon);
                for (j = 0; j < nbits; j++) {
                    secp256k1_scalar_mul(&yn, &yn, &ctx->y);
                    secp256k1_scalar_add(&twon, &twon, &twon);

                    secp256k1_scalar_add(&yn, &yn, &tmp);
                    secp256k1_scalar_add(&twon, &twon, &tmp);
                }

                secp256k1_scalar_mul(&negzn, &ctx->zsq, &ctx->negz);
                for (j = 0; j < i; j++) {
                    secp256k1_scalar_mul(&negzn, &negzn, &ctx->z);
                }
                if (ctx->min_value != NULL) {
                    secp256k1_scalar mv;
                    secp256k1_scalar_set_int(&mv, ctx->min_value[i]);
                    secp256k1_scalar_mul(&mv, &mv, &ctx->negz);
                    secp256k1_scalar_mul(&mv, &mv, &ctx->z);
                    for (j = 0; j < i; j++) {
                        secp256k1_scalar_mul(&negzn, &negzn, &ctx->z);
                    }
                    secp256k1_scalar_add(&twosum, &twosum, &mv);
                }
                secp256k1_scalar_mul(&twon, &twon, &negzn);
                secp256k1_scalar_add(&twosum, &twosum, &twon);
            }  /* yn = 1 + y + ... + y^(n-1); twosum = (z^3 + ... + z^{2 + n_commits})(1 + 2 + ... + 2^(n-1)) */


            secp256k1_scalar_mul(sc, sc, &yn);    /* (z - z^2)(1 + ... + y^(n-1)) */
            secp256k1_scalar_add(sc, sc, &twosum);  /* (z - z^2)(1 + ... + y^(n-1)) - z^3(1 + ... + 2^(n-1)) */
            secp256k1_scalar_negate(&tmp, &ctx->t);
            secp256k1_scalar_add(sc, sc, &tmp);    /* (z - z^2)(1 + ... + y^n) - z^3(1 + ... + 2^n) - t */
            secp256k1_scalar_mul(sc, sc, &ctx->randomizer61);
            *pt = *ctx->asset;
            break;
        }
        /* T1^x in eq (61) */
        case 3:
            secp256k1_scalar_mul(sc, &ctx->x, &ctx->randomizer61);
            *pt = ctx->t1;
            break;
        /* T2^x^2 in eq (61) */
        case 4:
            secp256k1_scalar_sqr(sc, &ctx->x);
            secp256k1_scalar_mul(sc, sc, &ctx->randomizer61);
            *pt = ctx->t2;
            break;
        /* V^z^2 in eq (61) */
        default:
            VERIFY_CHECK(ctx->count < 5 + ctx->n_commits);

            secp256k1_scalar_mul(sc, &ctx->zsq, &ctx->randomizer61);
            secp256k1_scalar_mul(&ctx->zsq, &ctx->zsq, &ctx->z);
            *pt = ctx->commit[ctx->count - 5];
            break;
        }
        secp256k1_scalar_mul(sc, sc, randomizer);
        ctx->count++;
    }
    return 1;
}

static int secp256k1_bulletproof_rangeproof_verify_impl(const secp256k1_ecmult_context *ecmult_ctx, secp256k1_scratch *scratch, const unsigned char* const* proof, const size_t n_proofs, const size_t plen, size_t nbits, const uint64_t* const* min_value, const secp256k1_ge* const* commitp, size_t n_commits, const secp256k1_ge *value_gen, const secp256k1_bulletproof_generators *gens, const unsigned char* const* extra_commit, size_t *extra_commit_len) {
    secp256k1_bulletproof_vfy_ecmult_context *ecmult_data;
    secp256k1_bulletproof_innerproduct_context *innp_ctx;
    int ret;
    size_t i;
    int same_generators = 1;

    /* sanity-check input */
    if (secp256k1_popcountl(nbits) != 1 || nbits > MAX_NBITS) {
        return 0;
    }
    if (plen < 64 + 128 + 1 + 32) {  /* inner product argument will do a more precise check */
        return 0;
    }
    if (plen > SECP256K1_BULLETPROOF_MAX_PROOF) {
        return 0;
    }

    if (!secp256k1_scratch_allocate_frame(scratch, n_proofs * (sizeof(*ecmult_data) + sizeof(*innp_ctx)), 2)) {
        return 0;
    }
    ecmult_data = (secp256k1_bulletproof_vfy_ecmult_context *)secp256k1_scratch_alloc(scratch, n_proofs * sizeof(*ecmult_data));
    innp_ctx = (secp256k1_bulletproof_innerproduct_context *)secp256k1_scratch_alloc(scratch, n_proofs * sizeof(*innp_ctx));

    /* Check if all generators are equal. If so, we can amortize all their scalar multiplications
     * by them and save one scalar-ge multiplication per proof. */
    VERIFY_CHECK(!secp256k1_ge_is_infinity(&value_gen[0]));
    for (i = 1; i < n_proofs; i++) {
        VERIFY_CHECK(!secp256k1_ge_is_infinity(&value_gen[i]));
        if (!secp256k1_fe_equal_var(&value_gen[i].x, &value_gen[i - 1].x) ||
            !secp256k1_fe_equal_var(&value_gen[i].y, &value_gen[i - 1].y)) {
            same_generators = 0;
        }
    }

    for (i = 0; i < n_proofs; i++) {
        secp256k1_sha256 sha256;
        unsigned char commit[32] = {0};
        unsigned char randomizer61[32] = {0};  /* randomizer for eq (61) so we can add it to eq (62) to save a separate multiexp */
        secp256k1_scalar taux, mu;
        secp256k1_ge age, sge;
        int overflow;
        size_t j;

        /* Commit to all input data: min value, pedersen commit, asset generator, extra_commit */
        if (min_value != NULL && min_value[i] != NULL) {
            unsigned char len[4];
            secp256k1_sha256_initialize(&sha256);
            secp256k1_sha256_write(&sha256, commit, 32);
            len[0] = n_commits;
            len[1] = n_commits >> 8;
            len[2] = n_commits >> 16;
            len[3] = n_commits >> 24;
            secp256k1_sha256_write(&sha256, len, 4);
            for (j = 0; j < n_commits; j++) {
                unsigned char vbuf[8];
                vbuf[0] = min_value[i][j];
                vbuf[1] = min_value[i][j] >> 8;
                vbuf[2] = min_value[i][j] >> 16;
                vbuf[3] = min_value[i][j] >> 24;
                vbuf[4] = min_value[i][j] >> 32;
                vbuf[5] = min_value[i][j] >> 40;
                vbuf[6] = min_value[i][j] >> 48;
                vbuf[7] = min_value[i][j] >> 56;
                secp256k1_sha256_write(&sha256, vbuf, 8);
            }
            secp256k1_sha256_finalize(&sha256, commit);
        }
        for (j = 0; j < n_commits; j++) {
            secp256k1_bulletproof_update_commit(commit, &commitp[i][j], &value_gen[i]);
        }
        if (extra_commit != NULL && extra_commit[i] != NULL) {
            secp256k1_sha256_initialize(&sha256);
            secp256k1_sha256_write(&sha256, commit, 32);
            secp256k1_sha256_write(&sha256, extra_commit[i], extra_commit_len[i]);
            secp256k1_sha256_finalize(&sha256, commit);
        }

        /* Compute y, z, x */
        if (!secp256k1_bulletproof_deserialize_point(&age, &proof[i][64], 0, 4) ||
            !secp256k1_bulletproof_deserialize_point(&sge, &proof[i][64], 1, 4)) {
            return 0;
        }

        secp256k1_bulletproof_update_commit(commit, &age, &sge);
        secp256k1_scalar_set_b32(&ecmult_data[i].y, commit, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ecmult_data[i].y)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        secp256k1_bulletproof_update_commit(commit, &age, &sge);
        secp256k1_scalar_set_b32(&ecmult_data[i].z, commit, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ecmult_data[i].z)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }

        if (!secp256k1_bulletproof_deserialize_point(&ecmult_data[i].t1, &proof[i][64], 2, 4) ||
            !secp256k1_bulletproof_deserialize_point(&ecmult_data[i].t2, &proof[i][64], 3, 4)) {
            return 0;
        }

        secp256k1_bulletproof_update_commit(commit, &ecmult_data[i].t1, &ecmult_data[i].t2);
        secp256k1_scalar_set_b32(&ecmult_data[i].x, commit, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ecmult_data[i].x)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }

        /* compute exponent offsets */
        secp256k1_scalar_inverse_var(&ecmult_data[i].yinv, &ecmult_data[i].y);  /* TODO somehow batch this w the inner-product argument inverse */
        ecmult_data[i].yinvn = ecmult_data[i].yinv;
        for (j = 0; j < secp256k1_floor_lg(nbits); j++) {
            secp256k1_scalar_sqr(&ecmult_data[i].yinvn, &ecmult_data[i].yinvn);
        }
        secp256k1_scalar_sqr(&ecmult_data[i].zsq, &ecmult_data[i].z);
        secp256k1_scalar_negate(&ecmult_data[i].negz, &ecmult_data[i].z);

        /* Update commit with remaining data for the inner product proof */
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        secp256k1_sha256_write(&sha256, &proof[i][0], 64);
        secp256k1_sha256_finalize(&sha256, commit);

        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        secp256k1_sha256_finalize(&sha256, randomizer61);
        secp256k1_scalar_set_b32(&ecmult_data[i].randomizer61, randomizer61, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ecmult_data[i].randomizer61)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }

        /* Deserialize everything else */
        secp256k1_scalar_set_b32(&taux, &proof[i][0], &overflow);
        if (overflow || secp256k1_scalar_is_zero(&taux)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        secp256k1_scalar_set_b32(&mu, &proof[i][32], &overflow);
        if (overflow || secp256k1_scalar_is_zero(&mu)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        /* A little sketchy, we read t (l(x) . r(x)) off the front of the inner product proof,
         * which we otherwise treat as a black box */
        secp256k1_scalar_set_b32(&ecmult_data[i].t, &proof[i][64 + 128 + 1], &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ecmult_data[i].t)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }

        /* Verify inner product proof */
        ecmult_data[i].a = age;
        ecmult_data[i].s = sge;
        ecmult_data[i].n = nbits * n_commits;
        ecmult_data[i].count = 0;
        ecmult_data[i].asset = &value_gen[i];
        ecmult_data[i].min_value = min_value == NULL ? NULL : min_value[i];
        ecmult_data[i].commit = commitp[i];
        ecmult_data[i].n_commits = n_commits;
        secp256k1_scalar_mul(&taux, &taux, &ecmult_data[i].randomizer61);
        secp256k1_scalar_add(&mu, &mu, &taux);

        innp_ctx[i].proof = &proof[i][64 + 128 + 1];
        innp_ctx[i].p_offs = mu;
        memcpy(innp_ctx[i].commit, commit, 32);
        innp_ctx[i].yinv = ecmult_data[i].yinv;
        innp_ctx[i].rangeproof_cb = secp256k1_bulletproof_rangeproof_vfy_callback;
        innp_ctx[i].rangeproof_cb_data = (void *) &ecmult_data[i];
        innp_ctx[i].n_extra_rangeproof_points = 5 + n_commits;
    }

    ret = secp256k1_bulletproof_inner_product_verify_impl(ecmult_ctx, scratch, gens, nbits * n_commits, innp_ctx, n_proofs, plen - (64 + 128 + 1), same_generators);
    secp256k1_scratch_deallocate_frame(scratch);
    return ret;
}

typedef struct {
    const unsigned char *nonce;
    secp256k1_scalar y;
    secp256k1_scalar z;
    secp256k1_scalar yn;
    secp256k1_scalar z22n;
    const uint64_t *val;
    const uint64_t *min_val;
    size_t n_vals;
    size_t nbits;
    size_t count;
} secp256k1_bulletproof_lr_generator;

static void secp256k1_lr_generator_init(secp256k1_bulletproof_lr_generator *generator, const unsigned char *nonce, const secp256k1_scalar *y, const secp256k1_scalar *z, size_t nbits, const uint64_t *val, const uint64_t *min_val, size_t n_vals) {
    generator->nonce = nonce;
    generator->y = *y;
    generator->z = *z;
    secp256k1_scalar_set_int(&generator->yn, 1);
    generator->nbits = nbits;
    generator->val = val;
    generator->min_val = min_val;
    generator->n_vals = n_vals;
    generator->count = 0;
}

static void secp256k1_lr_generate(secp256k1_bulletproof_lr_generator *generator, secp256k1_scalar *lout, secp256k1_scalar *rout, const secp256k1_scalar *x) {
    const size_t commit_idx = generator->count / generator->nbits;
    const size_t bit_idx = generator->count % generator->nbits;
    const uint64_t mv = generator->min_val == NULL ? 0 : generator->min_val[commit_idx];
    const int bit = ((generator->val[commit_idx] - mv) >> bit_idx) & 1;
    secp256k1_scalar sl, sr;
    secp256k1_scalar negz;

    if (bit_idx == 0) {
        size_t i;
        secp256k1_scalar_sqr(&generator->z22n, &generator->z);
        for (i = 0; i < commit_idx; i++) {
            secp256k1_scalar_mul(&generator->z22n, &generator->z22n, &generator->z);
        }
    }

    secp256k1_scalar_chacha20(&sl, &sr, generator->nonce, generator->count + 2);
    secp256k1_scalar_mul(&sl, &sl, x);
    secp256k1_scalar_mul(&sr, &sr, x);

    secp256k1_scalar_set_int(lout, bit);
    secp256k1_scalar_negate(&negz, &generator->z);
    secp256k1_scalar_add(lout, lout, &negz);
    secp256k1_scalar_add(lout, lout, &sl);

    secp256k1_scalar_set_int(rout, 1 - bit);
    secp256k1_scalar_negate(rout, rout);
    secp256k1_scalar_add(rout, rout, &generator->z);
    secp256k1_scalar_add(rout, rout, &sr);
    secp256k1_scalar_mul(rout, rout, &generator->yn);
    secp256k1_scalar_add(rout, rout, &generator->z22n);

    generator->count++;
    secp256k1_scalar_mul(&generator->yn, &generator->yn, &generator->y);
    secp256k1_scalar_add(&generator->z22n, &generator->z22n, &generator->z22n);
}

typedef struct {
    secp256k1_scalar x;
    secp256k1_scalar cache;
    secp256k1_bulletproof_lr_generator lr_gen;
} secp256k1_bulletproof_abgh_data;

static int secp256k1_bulletproof_abgh_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_bulletproof_abgh_data *ctx = (secp256k1_bulletproof_abgh_data *) data;
    const int is_g = idx % 2 == 0;

    (void) pt;
    if (is_g) {
        secp256k1_lr_generate(&ctx->lr_gen, sc, &ctx->cache, &ctx->x);
    } else {
        *sc = ctx->cache;
    }

    return 1;
}

/* Proof format: t, tau_x, mu, a, b, A, S, T_1, T_2, {L_i}, {R_i}
 *               5 scalar + [4 + 2log(n)] ge
 *
 * The non-bold `h` in the Bulletproofs paper corresponds to our gens->blinding_gen
 * while the non-bold `g` corresponds to the asset type `value_gen`.
 */
static int secp256k1_bulletproof_rangeproof_prove_impl(
    const secp256k1_ecmult_context *ecmult_ctx, secp256k1_scratch *scratch, 
    unsigned char *proof, size_t *plen, 
    unsigned char *tauxc, secp256k1_ge *tge,
    const size_t nbits, const uint64_t *value, const uint64_t *min_value,
    const secp256k1_scalar *blind, const secp256k1_ge *commitp, size_t n_commits, 
    const secp256k1_ge *value_gen, const secp256k1_bulletproof_generators *gens, 
    const unsigned char *nonce, const unsigned char *private_nonce,
    const unsigned char *extra_commit, size_t extra_commit_len, const unsigned char *message) {
    secp256k1_bulletproof_lr_generator lr_gen;
    secp256k1_bulletproof_abgh_data abgh_data;
    secp256k1_scalar zero;
    secp256k1_sha256 sha256;
    unsigned char commit[32] = {0};
    secp256k1_scalar alpha, rho;
    secp256k1_scalar t0, t1, t2;
    secp256k1_scalar tau1, tau2, taux, mu;
    secp256k1_scalar y;
    secp256k1_scalar z, zsq;
    secp256k1_scalar x, xsq;
    secp256k1_scalar tmps;
    secp256k1_gej aj, sj;
    secp256k1_gej tmpj;
    size_t i, j;
    int overflow;
    unsigned char vals_bytes[32] = {0};
    /* inner product proof variables */
    secp256k1_ge out_pt[4];

    if (secp256k1_popcountl(nbits) != 1 || nbits > MAX_NBITS) {
        return 0;
    }
    for (i = 0; i < n_commits; i++) {
        uint64_t mv = min_value == NULL ? 0 : min_value[i];
        if (mv > value[i]) {
            return 0;
        }
        if (nbits < 64 && (value[i] - mv) >= (1ull << nbits)) {
            return 0;
        }
    }
    if (plen != NULL && *plen < 128 + 64 + 1) { /* inner product argument will check and assign plen */
        return 0;
    }

    secp256k1_scalar_clear(&zero);

    /* Commit to all input data: min value, pedersen commit, asset generator, extra_commit */
    if (min_value != NULL) {
        unsigned char len[4];
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        len[0] = n_commits;
        len[1] = n_commits >> 8;
        len[2] = n_commits >> 16;
        len[3] = n_commits >> 24;
        secp256k1_sha256_write(&sha256, len, 4);
        for (i = 0; i < n_commits; i++) {
            unsigned char vbuf[8];
            vbuf[0] = min_value[i];
            vbuf[1] = min_value[i] >> 8;
            vbuf[2] = min_value[i] >> 16;
            vbuf[3] = min_value[i] >> 24;
            vbuf[4] = min_value[i] >> 32;
            vbuf[5] = min_value[i] >> 40;
            vbuf[6] = min_value[i] >> 48;
            vbuf[7] = min_value[i] >> 56;
            secp256k1_sha256_write(&sha256, vbuf, 8);
        }
        secp256k1_sha256_finalize(&sha256, commit);
    }
    for (i = 0; i < n_commits; i++) {
        secp256k1_bulletproof_update_commit(commit, &commitp[i], value_gen); /* TODO be less stupid about this */
    }
    if (extra_commit != NULL) {
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        secp256k1_sha256_write(&sha256, extra_commit, extra_commit_len);
        secp256k1_sha256_finalize(&sha256, commit);
    }

    secp256k1_scalar_chacha20(&alpha, &rho, nonce, 0);
    secp256k1_scalar_chacha20(&tau1, &tau2, private_nonce, 1);

    if (proof == NULL && tauxc == NULL && tge != NULL) {
        /* Multi-party bulletproof: export tau1j*G and tau2j*G */
        secp256k1_ecmult_const(&tmpj, &gens->blinding_gen[0], &tau1, 256);
        secp256k1_ge_set_gej(&tge[0], &tmpj);

        secp256k1_ecmult_const(&tmpj, &gens->blinding_gen[0], &tau2, 256);
        secp256k1_ge_set_gej(&tge[1], &tmpj);

        return 1;
    }

    /* Encrypt value into alpha, so it will be recoverable from -mu by someone who knows `nonce` */
    if (n_commits == 1) {
        secp256k1_scalar vals;
        secp256k1_scalar_set_u64(&vals, value[0]);
        if (message != NULL) {
            /* Combine value with 20 bytes of optional message */
            secp256k1_scalar_get_b32(vals_bytes, &vals);
            for (i=0; i<20; i++) {
                vals_bytes[i+4] = message[i];
            }
            secp256k1_scalar_set_b32(&vals, vals_bytes, &overflow);
        }
        secp256k1_scalar_negate(&vals, &vals); /* Negate so it'll be positive in -mu */
        secp256k1_scalar_add(&alpha, &alpha, &vals);
    }

    /* Compute A and S */
    secp256k1_ecmult_const(&aj, &gens->blinding_gen[0], &alpha, 256);
    secp256k1_ecmult_const(&sj, &gens->blinding_gen[0], &rho, 256);
    for (i = 0; i < n_commits; i++) {
        for (j = 0; j < nbits; j++) {
            secp256k1_scalar sl, sr;
            uint64_t mv = min_value == NULL ? 0 : min_value[i];
            size_t al = !!((value[i] - mv) & (1ull << j));
            secp256k1_ge aterm = gens->gens[i * nbits + j + gens->n/2];
            secp256k1_ge sterm;
            secp256k1_gej stermj;

            secp256k1_scalar_chacha20(&sl, &sr, nonce, i * nbits + j + 2);

            secp256k1_ge_neg(&aterm, &aterm);
            secp256k1_fe_cmov(&aterm.x, &gens->gens[i * nbits + j].x, al);
            secp256k1_fe_cmov(&aterm.y, &gens->gens[i * nbits + j].y, al);

            secp256k1_gej_add_ge(&aj, &aj, &aterm);

            secp256k1_ecmult_const(&stermj, &gens->gens[i * nbits + j], &sl, 256);
            secp256k1_ge_set_gej(&sterm, &stermj);
            secp256k1_gej_add_ge(&sj, &sj, &sterm);
            secp256k1_ecmult_const(&stermj, &gens->gens[i * nbits + j + gens->n/2], &sr, 256);
            secp256k1_ge_set_gej(&sterm, &stermj);
            secp256k1_gej_add_ge(&sj, &sj, &sterm);
        }
    }

    /* get challenges y and z */
    secp256k1_ge_set_gej(&out_pt[0], &aj);
    secp256k1_ge_set_gej(&out_pt[1], &sj);

    secp256k1_bulletproof_update_commit(commit, &out_pt[0], &out_pt[1]);
    secp256k1_scalar_set_b32(&y, commit, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&y)) {
        return 0;
    }
    secp256k1_bulletproof_update_commit(commit, &out_pt[0], &out_pt[1]); /* TODO rehashing A and S to get a second challenge is overkill */
    secp256k1_scalar_set_b32(&z, commit, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&z)) {
        return 0;
    }
    secp256k1_scalar_sqr(&zsq, &z);

    /* Compute coefficients t0, t1, t2 of the <l, r> polynomial */
    /* t0 = l(0) dot r(0) */
    secp256k1_lr_generator_init(&lr_gen, nonce, &y, &z, nbits, value, min_value, n_commits);
    secp256k1_scalar_clear(&t0);
    for (i = 0; i < nbits * n_commits; i++) {
        secp256k1_scalar l, r;
        secp256k1_lr_generate(&lr_gen, &l, &r, &zero);
        secp256k1_scalar_mul(&l, &l, &r);
        secp256k1_scalar_add(&t0, &t0, &l);
    }

    /* A = t0 + t1 + t2 = l(1) dot r(1) */
    secp256k1_lr_generator_init(&lr_gen, nonce, &y, &z, nbits, value, min_value, n_commits);
    secp256k1_scalar_clear(&t1);
    for (i = 0; i < nbits * n_commits; i++) {
        secp256k1_scalar one;
        secp256k1_scalar l, r;
        secp256k1_scalar_set_int(&one, 1);
        secp256k1_lr_generate(&lr_gen, &l, &r, &one);
        secp256k1_scalar_mul(&l, &l, &r);
        secp256k1_scalar_add(&t1, &t1, &l);
    }

    /* B = t0 - t1 + t2 = l(-1) dot r(-1) */
    secp256k1_lr_generator_init(&lr_gen, nonce, &y, &z, nbits, value, min_value, n_commits);
    secp256k1_scalar_clear(&t2);
    for (i = 0; i < nbits * n_commits; i++) {
        secp256k1_scalar negone;
        secp256k1_scalar l, r;
        secp256k1_scalar_set_int(&negone, 1);
        secp256k1_scalar_negate(&negone, &negone);
        secp256k1_lr_generate(&lr_gen, &l, &r, &negone);
        secp256k1_scalar_mul(&l, &l, &r);
        secp256k1_scalar_add(&t2, &t2, &l);
    }

    /* t1 = (A - B)/2 */
    secp256k1_scalar_set_int(&tmps, 2);
    secp256k1_scalar_inverse_var(&tmps, &tmps);
    secp256k1_scalar_negate(&t2, &t2);
    secp256k1_scalar_add(&t1, &t1, &t2);
    secp256k1_scalar_mul(&t1, &t1, &tmps);

    /* t2 = -(-B + t0) + t1 */
    secp256k1_scalar_add(&t2, &t2, &t0);
    secp256k1_scalar_negate(&t2, &t2);
    secp256k1_scalar_add(&t2, &t2, &t1);

    /* Compute Ti = t_i*A + tau_i*G for i = 1,2 */
    secp256k1_ecmult_const(&tmpj, value_gen, &t1, 256);
    if (tge == NULL) {
        /* Normal bulletproof: T1=t1*A + tau1*G */
        secp256k1_ge_set_gej(&out_pt[2], &tmpj);
        secp256k1_ecmult_const(&tmpj, &gens->blinding_gen[0], &tau1, 256);
        secp256k1_gej_add_ge(&tmpj, &tmpj, &out_pt[2]);
    } else {
        /* Multi-party bulletproof: T1=t1*A + sumj tau1j*G */
        secp256k1_gej_add_ge(&tmpj, &tmpj, &tge[0]);
    }
    secp256k1_ge_set_gej(&out_pt[2], &tmpj);

    secp256k1_ecmult_const(&tmpj, value_gen, &t2, 256);
    if (tge == NULL) {
        /* Normal bulletproof: T1=t1*A + tau1*G */
        secp256k1_ge_set_gej(&out_pt[3], &tmpj);
        secp256k1_ecmult_const(&tmpj, &gens->blinding_gen[0], &tau2, 256);
        secp256k1_gej_add_ge(&tmpj, &tmpj, &out_pt[3]);
    } else {
        /* Multi-party bulletproof: T2=t2*A + sumj tau2j*G */
        secp256k1_gej_add_ge(&tmpj, &tmpj, &tge[1]);
    }
    secp256k1_ge_set_gej(&out_pt[3], &tmpj);

    secp256k1_bulletproof_update_commit(commit, &out_pt[2], &out_pt[3]);
    secp256k1_scalar_set_b32(&x, commit, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&x)) {
        return 0;
    }
    secp256k1_scalar_sqr(&xsq, &x);

    if (proof == NULL || tauxc == NULL) {
        /* compute tau_x and mu */
        secp256k1_scalar_mul(&taux, &tau1, &x);
        secp256k1_scalar_mul(&tmps, &tau2, &xsq);
        secp256k1_scalar_add(&taux, &taux, &tmps);
        for (i = 0; i < n_commits; i++) {
            secp256k1_scalar_mul(&tmps, &zsq, &blind[i]);
            secp256k1_scalar_add(&taux, &taux, &tmps);
            secp256k1_scalar_mul(&zsq, &zsq, &z);
        }
    }

    if (proof == NULL) {
        /* Multi-party bulletproof: export tauxj */
        secp256k1_scalar_get_b32(tauxc, &taux);
        return 1;
    }

    if (tauxc != NULL) {
        /* Multi-party bulletproof: taux = sumj tauxj */
        secp256k1_scalar_set_b32(&taux, tauxc, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&tmps)) {
            return 0;
        }
    }

    secp256k1_scalar_mul(&mu, &rho, &x);
    secp256k1_scalar_add(&mu, &mu, &alpha);

    /* Negate taux and mu so the verifier doesn't have to */
    secp256k1_scalar_negate(&taux, &taux);
    secp256k1_scalar_negate(&mu, &mu);

    /* Encode rangeproof stuff */
    secp256k1_scalar_get_b32(&proof[0], &taux);
    secp256k1_scalar_get_b32(&proof[32], &mu);
    secp256k1_bulletproof_serialize_points(&proof[64], out_pt, 4);

    /* Mix this into the hash so the input to the inner product proof is fixed */
    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, commit, 32);
    secp256k1_sha256_write(&sha256, proof, 64);
    secp256k1_sha256_finalize(&sha256, commit);

    /* Compute l and r, do inner product proof */
    abgh_data.x = x;
    secp256k1_lr_generator_init(&abgh_data.lr_gen, nonce, &y, &z, nbits, value, min_value, n_commits);
    *plen -= 64 + 128 + 1;
    secp256k1_scalar_inverse_var(&y, &y);
    if (secp256k1_bulletproof_inner_product_prove_impl(ecmult_ctx, scratch, &proof[64 + 128 + 1], plen, gens, &y, nbits * n_commits, secp256k1_bulletproof_abgh_callback, (void *) &abgh_data, commit) == 0) {
        return 0;
    }
    *plen += 64 + 128 + 1;

    return 1;
}

static int secp256k1_bulletproof_rangeproof_rewind_impl(uint64_t *value, secp256k1_scalar *blind, const unsigned char *proof, const size_t plen, uint64_t min_value, const secp256k1_pedersen_commitment *pcommit, const secp256k1_generator *value_gen, const unsigned char *nonce, const unsigned char *extra_commit, size_t extra_commit_len, unsigned char *message) {
    secp256k1_sha256 sha256;
    static const unsigned char zero4[4] = { 0 };
    unsigned char commit[32] = { 0 };
    unsigned char lrparity;
    secp256k1_scalar taux, mu;
    secp256k1_scalar alpha, rho, tau1, tau2;
    secp256k1_scalar x, z;
    secp256k1_ge commitp, value_genp;
    int overflow, i;

    if (plen < 64 + 128 + 1 || plen > SECP256K1_BULLETPROOF_MAX_PROOF) {
        return 0;
    }

    /* Extract data from beginning of proof */
    secp256k1_scalar_set_b32(&taux, &proof[0], &overflow);
    if (overflow || secp256k1_scalar_is_zero(&taux)) {
        return 0;
    }
    secp256k1_scalar_set_b32(&mu, &proof[32], &overflow);
    if (overflow || secp256k1_scalar_is_zero(&mu)) {
        return 0;
    }

    secp256k1_scalar_chacha20(&alpha, &rho, nonce, 0);
    secp256k1_scalar_chacha20(&tau1, &tau2, nonce, 1);

    if (min_value > 0) {
        unsigned char vbuf[8];
        const unsigned char len[4] = { 1, 0, 0, 0 };
        vbuf[0] = min_value;
        vbuf[1] = min_value >> 8;
        vbuf[2] = min_value >> 16;
        vbuf[3] = min_value >> 24;
        vbuf[4] = min_value >> 32;
        vbuf[5] = min_value >> 40;
        vbuf[6] = min_value >> 48;
        vbuf[7] = min_value >> 56;
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        secp256k1_sha256_write(&sha256, len, 4);
        secp256k1_sha256_write(&sha256, vbuf, 8);
        secp256k1_sha256_finalize(&sha256, commit);
    }

    secp256k1_pedersen_commitment_load(&commitp, pcommit);
    secp256k1_generator_load(&value_genp, value_gen);
    secp256k1_bulletproof_update_commit(commit, &commitp, &value_genp);

    if (extra_commit != NULL) {
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        secp256k1_sha256_write(&sha256, extra_commit, extra_commit_len);
        secp256k1_sha256_finalize(&sha256, commit);
    }

    /* Extract A and S to compute y and z */
    lrparity = 2 * !!(proof[64] & 1) + !!(proof[64] & 2);
    /* y */
    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, commit, 32);
    secp256k1_sha256_write(&sha256, &lrparity, 1);
    secp256k1_sha256_write(&sha256, &proof[65], 64);
    secp256k1_sha256_finalize(&sha256, commit);

    /* z */
    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, commit, 32);
    secp256k1_sha256_write(&sha256, &lrparity, 1);
    secp256k1_sha256_write(&sha256, &proof[65], 64);
    secp256k1_sha256_finalize(&sha256, commit);

    secp256k1_scalar_set_b32(&z, commit, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&z)) {
        return 0;
    }

    /* x */
    lrparity = 2 * !!(proof[64] & 4) + !!(proof[64] & 8);
    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, commit, 32);
    secp256k1_sha256_write(&sha256, &lrparity, 1);
    secp256k1_sha256_write(&sha256, &proof[129], 64);
    secp256k1_sha256_finalize(&sha256, commit);

    secp256k1_scalar_set_b32(&x, commit, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&x)) {
        return 0;
    }

    /* Compute candidate mu and add to (negated) mu from proof to get value */
    secp256k1_scalar_mul(&rho, &rho, &x);
    secp256k1_scalar_add(&mu, &mu, &rho);
    secp256k1_scalar_add(&mu, &mu, &alpha);

    secp256k1_scalar_get_b32(commit, &mu);
    if (memcmp(commit, zero4, 4) != 0) {
        return 0;
    }
    *value = commit[31] + ((uint64_t) commit[30] << 8) +
             ((uint64_t) commit[29] << 16) + ((uint64_t) commit[28] << 24) +
             ((uint64_t) commit[27] << 32) + ((uint64_t) commit[26] << 40) +
             ((uint64_t) commit[25] << 48) + ((uint64_t) commit[24] << 56);

    if (message != NULL) {
        for (i=23; i >= 4; i--) {
            message[i-4] = commit[i];
        }
    }

    /* Derive blinding factor */
    secp256k1_scalar_mul(&tau1, &tau1, &x);
    secp256k1_scalar_mul(&tau2, &tau2, &x);
    secp256k1_scalar_mul(&tau2, &tau2, &x);

    secp256k1_scalar_add(&taux, &taux, &tau1);
    secp256k1_scalar_add(&taux, &taux, &tau2);

    secp256k1_scalar_sqr(&z, &z);
    secp256k1_scalar_inverse_var(&z, &z);
    secp256k1_scalar_mul(blind, &taux, &z);
    secp256k1_scalar_negate(blind, blind);

    return 1;
}

#endif
