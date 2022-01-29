/**********************************************************************
 * Copyright (c) 2018 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_BULLETPROOF_MAIN_IMPL
#define SECP256K1_MODULE_BULLETPROOF_MAIN_IMPL

#include "group.h"
#include "scalar.h"

#include "modules/commitment/main_impl.h"

struct secp256k1_bulletproof_generators {
    size_t n;
    /* `G_i`, `H_i` generators, `n` each of them which are generated when creating this struct */
    secp256k1_ge *gens;
    /* `H` "alternate" generator, used in Pedersen commitments. Passed in by caller to
     * `secp256k1_bulletproof_generators_create`; stored in this structure to allow consistent
     * generators between functions using `secp256k1_bulletproof_generators` and functions
     * using the Pedersen commitment module. */
    secp256k1_ge *blinding_gen;
};

#include "modules/bulletproofs/inner_product_impl.h"
#include "modules/bulletproofs/rangeproof_impl.h"
#include "modules/bulletproofs/util.h"

secp256k1_bulletproof_generators *secp256k1_bulletproof_generators_create(const secp256k1_context *ctx, const secp256k1_generator *blinding_gen, size_t n) {
    secp256k1_bulletproof_generators *ret;
    secp256k1_rfc6979_hmac_sha256 rng;
    unsigned char seed[64];
    secp256k1_gej precompj;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(blinding_gen != NULL);

    ret = (secp256k1_bulletproof_generators *)checked_malloc(&ctx->error_callback, sizeof(*ret));
    if (ret == NULL) {
        return NULL;
    }
    ret->gens = (secp256k1_ge *)checked_malloc(&ctx->error_callback, (n + 1) * sizeof(*ret->gens));
    if (ret->gens == NULL) {
        free(ret);
        return NULL;
    }
    ret->blinding_gen = &ret->gens[n];
    ret->n = n;

    secp256k1_fe_get_b32(&seed[0], &secp256k1_ge_const_g.x);
    secp256k1_fe_get_b32(&seed[32], &secp256k1_ge_const_g.y);

    secp256k1_rfc6979_hmac_sha256_initialize(&rng, seed, 64);
    for (i = 0; i < n; i++) {
        unsigned char tmp[32] = { 0 };
        secp256k1_generator gen;
        secp256k1_rfc6979_hmac_sha256_generate(&rng, tmp, 32);
        CHECK(secp256k1_generator_generate(ctx, &gen, tmp));
        secp256k1_generator_load(&ret->gens[i], &gen);

        secp256k1_gej_set_ge(&precompj, &ret->gens[i]);
    }

    secp256k1_generator_load(&ret->blinding_gen[0], blinding_gen);
    secp256k1_gej_set_ge(&precompj, &ret->blinding_gen[0]);

    return ret;
}

void secp256k1_bulletproof_generators_destroy(const secp256k1_context* ctx, secp256k1_bulletproof_generators *gens) {
    (void) ctx;
    if (gens != NULL) {
        free(gens->gens);
        free(gens);
    }
}

int secp256k1_bulletproof_rangeproof_verify(const secp256k1_context* ctx, secp256k1_scratch_space *scratch, const secp256k1_bulletproof_generators *gens, const unsigned char *proof, size_t plen,
 const uint64_t *min_value, const secp256k1_pedersen_commitment* commit, size_t n_commits, size_t nbits, const secp256k1_generator *value_gen, const unsigned char *extra_commit, size_t extra_commit_len) {
    int ret;
    size_t i;
    secp256k1_ge *commitp;
    secp256k1_ge value_genp;
    const secp256k1_ge *commitp_ptr;
    const uint64_t *minvalue_ptr;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(scratch != NULL);
    ARG_CHECK(gens != NULL);
    ARG_CHECK(gens->n >= 2 * nbits * n_commits);
    ARG_CHECK(proof != NULL);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(n_commits > 0);
    ARG_CHECK(nbits > 0);
    ARG_CHECK(nbits <= 64);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK(extra_commit != NULL || extra_commit_len == 0);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));

    if (!secp256k1_scratch_allocate_frame(scratch, 2 * n_commits * sizeof(secp256k1_ge), 1)) {
        return 0;
    }

    commitp = (secp256k1_ge *)secp256k1_scratch_alloc(scratch, n_commits * sizeof(secp256k1_ge));
    for (i = 0; i < n_commits; i++) {
        secp256k1_pedersen_commitment_load(&commitp[i], &commit[i]);
    }
    secp256k1_generator_load(&value_genp, value_gen);

    commitp_ptr = commitp;
    minvalue_ptr = min_value;
    ret = secp256k1_bulletproof_rangeproof_verify_impl(&ctx->ecmult_ctx, scratch, &proof, 1, plen, nbits, &minvalue_ptr, &commitp_ptr, n_commits, &value_genp, gens, &extra_commit, &extra_commit_len);
    secp256k1_scratch_deallocate_frame(scratch);
    return ret;
}

int secp256k1_bulletproof_rangeproof_verify_multi(const secp256k1_context* ctx, secp256k1_scratch_space *scratch, const secp256k1_bulletproof_generators *gens, const unsigned char* const* proof, size_t n_proofs, size_t plen, const uint64_t* const* min_value, const secp256k1_pedersen_commitment* const* commit, size_t n_commits, size_t nbits, const secp256k1_generator *value_gen, const unsigned char* const* extra_commit, size_t *extra_commit_len) {
    int ret;
    secp256k1_ge **commitp;
    secp256k1_ge *value_genp;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(scratch != NULL);
    ARG_CHECK(gens != NULL);
    ARG_CHECK(gens->n >= 2 * nbits * n_commits);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(proof != NULL);
    ARG_CHECK(n_proofs > 0);
    ARG_CHECK(n_commits > 0);
    ARG_CHECK(nbits > 0);
    ARG_CHECK(nbits <= 64);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK((extra_commit_len == NULL) == (extra_commit == NULL));
    if (extra_commit != NULL) {
        for (i = 0; i < n_proofs; i++) {
            ARG_CHECK(extra_commit[i] != NULL || extra_commit_len[i] == 0);
        }
    }
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));

    if (!secp256k1_scratch_allocate_frame(scratch, n_proofs * (sizeof(*value_genp) + sizeof(*commitp) + n_commits * sizeof(**commitp)), 2 + n_proofs)) {
        return 0;
    }

    commitp = (secp256k1_ge **)secp256k1_scratch_alloc(scratch, n_proofs * sizeof(*commitp));
    value_genp = (secp256k1_ge *)secp256k1_scratch_alloc(scratch, n_proofs * sizeof(*value_genp));
    for (i = 0; i < n_proofs; i++) {
        size_t j;
        commitp[i] = (secp256k1_ge *)secp256k1_scratch_alloc(scratch, n_commits * sizeof(*commitp[i]));
        for (j = 0; j < n_commits; j++) {
            secp256k1_pedersen_commitment_load(&commitp[i][j], &commit[i][j]);
        }
        secp256k1_generator_load(&value_genp[i], &value_gen[i]);
    }

    ret = secp256k1_bulletproof_rangeproof_verify_impl(&ctx->ecmult_ctx, scratch, proof, n_proofs, plen, nbits, min_value, (const secp256k1_ge **) commitp, n_commits, value_genp, gens, extra_commit, extra_commit_len);
    secp256k1_scratch_deallocate_frame(scratch);
    return ret;
}

int secp256k1_bulletproof_rangeproof_rewind(const secp256k1_context* ctx, uint64_t *value, unsigned char *blind, const unsigned char *proof, size_t plen, uint64_t min_value, const secp256k1_pedersen_commitment* commit, const secp256k1_generator *value_gen, const unsigned char *nonce, const unsigned char *extra_commit, size_t extra_commit_len, unsigned char *message) {
    secp256k1_scalar blinds;
    int ret;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(value != NULL);
    ARG_CHECK(blind != NULL);
    ARG_CHECK(proof != NULL);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK(nonce != NULL);
    ARG_CHECK(extra_commit != NULL || extra_commit_len == 0);

    ret = secp256k1_bulletproof_rangeproof_rewind_impl(value, &blinds, proof, plen, min_value, commit, value_gen, nonce, extra_commit, extra_commit_len, message);
    if (ret == 1) {
        secp256k1_scalar_get_b32(blind, &blinds);
    }
    return ret;
}

int secp256k1_bulletproof_rangeproof_prove(
    const secp256k1_context* ctx, secp256k1_scratch_space* scratch, const secp256k1_bulletproof_generators* gens, 
    unsigned char* proof, size_t* plen, 
    unsigned char* tau_x, secp256k1_pubkey* t_one, secp256k1_pubkey* t_two, 
    const uint64_t* value, const uint64_t* min_value, 
    const unsigned char* const* blind, const secp256k1_pedersen_commitment* const* commits, size_t n_commits, 
    const secp256k1_generator* value_gen, size_t nbits, 
    const unsigned char* nonce, const unsigned char* private_nonce, 
    const unsigned char* extra_commit, size_t extra_commit_len, const unsigned char* message
) {
    int ret;
    secp256k1_ge *commitp;
    secp256k1_scalar *blinds;
    secp256k1_ge value_genp;
    size_t i;
    const unsigned char *secondary_nonce;
    secp256k1_ge *tge = NULL;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(scratch != NULL);
    ARG_CHECK(gens != NULL);
    ARG_CHECK(gens->n >= 2 * nbits * n_commits);
    ARG_CHECK(
        (proof != NULL && plen != NULL && tau_x == NULL && t_one == NULL && t_two == NULL && commits == NULL) ||
        (proof == NULL && plen == NULL && tau_x == NULL && t_one != NULL && t_two != NULL && commits != NULL && private_nonce != NULL) ||
        (proof == NULL && plen == NULL && tau_x != NULL && t_one != NULL && t_two != NULL && commits != NULL && private_nonce != NULL) ||
        (proof != NULL && plen != NULL && tau_x != NULL && t_one != NULL && t_two != NULL && commits != NULL && private_nonce != NULL)
    ); /* 1) normal BP, 2) multi-party BP step 1, 3) multi-party BP step 2, 4) multi-party BP step 3 */
    ARG_CHECK(value != NULL);
    ARG_CHECK(blind != NULL);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK(nonce != NULL);
    ARG_CHECK(n_commits > 0 && n_commits);
    ARG_CHECK(nbits <= 64);
    if (nbits < 64) {
        for (i = 0; i < n_commits; i++) {
            ARG_CHECK(value[i] < (1ull << nbits));
            ARG_CHECK(blind[i] != NULL);
        }
    }
    ARG_CHECK(extra_commit != NULL || extra_commit_len == 0);
    ARG_CHECK(secp256k1_ecmult_context_is_built(&ctx->ecmult_ctx));
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));

    if (!secp256k1_scratch_allocate_frame(scratch, n_commits * (sizeof(*commitp) + sizeof(*blinds)), 2)) {
        return 0;
    }
    commitp = (secp256k1_ge *)secp256k1_scratch_alloc(scratch, n_commits * sizeof(*commitp));
    blinds = (secp256k1_scalar *)secp256k1_scratch_alloc(scratch, n_commits * sizeof(*blinds));

    secp256k1_generator_load(&value_genp, value_gen);
    for (i = 0; i < n_commits; i++) {
        int overflow;
        secp256k1_scalar_set_b32(&blinds[i], blind[i], &overflow);
        if (overflow || secp256k1_scalar_is_zero(&blinds[i])) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        
        if (commits == NULL) {
            /* Calculate commitment from blinding factor */
            secp256k1_gej commitj;
            secp256k1_pedersen_ecmult(&commitj, &blinds[i], value[i], &value_genp, &gens->blinding_gen[0]);
            secp256k1_ge_set_gej(&commitp[i], &commitj);
        }
        else {
            /* Multi-party bulletproof: total blinding factor unknown. Input commitment(s) */
            secp256k1_pedersen_commitment_load(&commitp[i], commits[i]);
        }
    }

    if (private_nonce == NULL) {
        secondary_nonce = nonce;
    }
    else {
        secondary_nonce = private_nonce;
    }

    if (t_one != NULL) {
        tge = malloc(2*sizeof(secp256k1_ge));
        if (tge == NULL){
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        if (tau_x != NULL) {
            if (!secp256k1_pubkey_load(ctx, &tge[0], t_one)) {
                secp256k1_scratch_deallocate_frame(scratch);
                return 0;
            }
            if (!secp256k1_pubkey_load(ctx, &tge[1], t_two)) {
                secp256k1_scratch_deallocate_frame(scratch);
                return 0;
            }
        }
    }

    ret = secp256k1_bulletproof_rangeproof_prove_impl(&ctx->ecmult_ctx, scratch, proof, plen, tau_x, tge, nbits, value, min_value, blinds, commitp, n_commits, &value_genp, gens, nonce, secondary_nonce, extra_commit, extra_commit_len, message);

    if (t_one != NULL && tau_x == NULL) {
        secp256k1_pubkey_save(t_one, &tge[0]);
        secp256k1_pubkey_save(t_two, &tge[1]);
    }
    secp256k1_scratch_deallocate_frame(scratch);
    return ret;
}

#endif
