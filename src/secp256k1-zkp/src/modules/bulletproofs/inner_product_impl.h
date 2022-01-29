/**********************************************************************
 * Copyright (c) 2018 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_BULLETPROOF_INNER_PRODUCT_IMPL
#define SECP256K1_MODULE_BULLETPROOF_INNER_PRODUCT_IMPL

#include "group.h"
#include "scalar.h"

#include "modules/bulletproofs/main_impl.h"
#include "modules/bulletproofs/util.h"

/* Number of scalars that should remain at the end of a recursive proof. The paper
 * uses 2, by reducing the scalars as far as possible. We stop one recursive step
 * early, trading two points (L, R) for two scalars, which reduces verification
 * and prover cost.
 *
 * For the most part, all comments assume this value is at 4.
 */
#define IP_AB_SCALARS	4

/* Bulletproof inner products consist of the four scalars and `2[log2(n) - 1]` points
 * `a_1`, `a_2`, `b_1`, `b_2`, `L_i` and `R_i`, where `i` ranges from 0 to `log2(n)-1`.
 *
 * The prover takes as input a point `P` and scalar `c`. It proves that it knows
 * scalars `a_i`, `b_i` for `i` ranging from 1 to `n`, such that
 *     `P = sum_i [a_i G_i + b_i H_i]` and `<{a_i}, {b_i}> = c`,
 * where `G_i` and `H_i` are standard NUMS generators.
 *
 * Verification of the proof comes down to a single multiexponentiation of the form
 *
 *   P + (c - a_1*b_1 - a_2*b_2)*x*G
 *     - sum_{i=1}^n [s'_i*G_i + s_i*H_i]
 *     + sum_{i=1}^log2(n) [x_i^-2 L_i + x_i^2 R_i]
 *
 * which will equal infinity if the inner product proof is correct. Here
 *   - `G` is the standard secp generator
 *   - `x` is a hash of `commit` and is used to rerandomize `c`. See Protocol 2 vs Protocol 1 in the paper.
 *   - `x_i = H(x_{i-1} || L_i || R_i)`, where `x_{-1}` is passed through the `commit` variable and
 *     must be a commitment to `P` and `c`.
 *   - `s_i` and `s'_i` are computed as follows.
 *
 * Letting `i_j` be defined as 1 if `i & 2^j == 1`, and -1 otherwise,
 *   - For `i` from       `1` to `n/2`, `s'_i = a_1 * prod_{j=1}^log2(n) x_j^i_j`
 *   - For `i` from `n/2 + 1` to   `n`, `s'_i = a_2 * prod_{j=1}^log2(n) x_j^i_j`
 *   - For `i` from       `1` to `n/2`, `s_i = b_1 * prod_{j=1}^log2(n) x_j^-i_j`
 *   - For `i` from `n/2 + 1` to   `n`, `s_i = b_2 * prod_{j=1}^log2(n) x_j^-i_j`
 *
 * Observe that these can be computed iteratively by labelling the coefficients `s_i` for `i`
 * from `0` to `2n-1` rather than 1-indexing and distinguishing between `s_i'`s and `s_i`s:
 *
 * Start with `s_0 = a_1 * prod_{j=1}^log2(n) x_j^-1`, then for later `s_i`s,
 *   - For `i` from `1` to `n/2 - 1`, multiply some earlier `s'_j` by some `x_k^2`
 *   - For `i = n/2`, multiply `s_{i-1} by `a_2/a_1`.
 *   - For `i` from `n/2 + 1` to `n - 1`, multiply some earlier `s'_j` by some `x_k^2`
 *   - For `i = n`, multiply `s'_{i-1}` by `b_1/a_2` to get `s_i`.
 *   - For `i` from `n + 1` to `3n/2 - 1`, multiply some earlier `s_j` by some `x_k^-2`
 *   - For `i = 3n/2`, multiply `s_{i-1}` by `b_2/b_1`.
 *   - For `i` from `3n/2 + 1` to `2n - 1`, multiply some earlier `s_j` by some `x_k^-2`
 * where of course, the indices `j` and `k` must be chosen carefully.
 *
 * The bulk of `secp256k1_bulletproof_innerproduct_vfy_ecmult_callback` involves computing
 * these indices, given `a_2/a_1`, `b_1/a_1`, `b_2/b_1`, and the `x_k^2`s as input. It
 * computes `x_k^-2` as a side-effect of its other computation.
 */

typedef int (secp256k1_bulletproof_vfy_callback)(secp256k1_scalar *sc, secp256k1_ge *pt, secp256k1_scalar *randomizer, size_t idx, void *data);

/* used by callers to wrap a proof with surrounding context */
typedef struct {
    const unsigned char *proof;
    secp256k1_scalar p_offs;
    secp256k1_scalar yinv;
    unsigned char commit[32];
    secp256k1_bulletproof_vfy_callback *rangeproof_cb;
    void *rangeproof_cb_data;
    size_t n_extra_rangeproof_points;
} secp256k1_bulletproof_innerproduct_context;

/* used internally */
typedef struct {
    const secp256k1_bulletproof_innerproduct_context *proof;
    secp256k1_scalar abinv[IP_AB_SCALARS];
    secp256k1_scalar xsq[SECP256K1_BULLETPROOF_MAX_DEPTH + 1];
    secp256k1_scalar xsqinv[SECP256K1_BULLETPROOF_MAX_DEPTH + 1];
    secp256k1_scalar xsqinvy[SECP256K1_BULLETPROOF_MAX_DEPTH + 1];
    secp256k1_scalar xcache[SECP256K1_BULLETPROOF_MAX_DEPTH + 1];
    secp256k1_scalar xsqinv_mask;
    const unsigned char *serialized_lr;
} secp256k1_bulletproof_innerproduct_vfy_data;

/* used by callers to modify the multiexp */
typedef struct {
    size_t n_proofs;
    secp256k1_scalar p_offs;
    const secp256k1_ge *g;
    const secp256k1_ge *geng;
    const secp256k1_ge *genh;
    size_t vec_len;
    size_t lg_vec_len;
    int shared_g;
    secp256k1_scalar *randomizer;
    secp256k1_bulletproof_innerproduct_vfy_data *proof;
} secp256k1_bulletproof_innerproduct_vfy_ecmult_context;

size_t secp256k1_bulletproof_innerproduct_proof_length(size_t n) {
    if (n < IP_AB_SCALARS / 2) {
        return 32 * (1 + 2 * n);
    } else {
        size_t bit_count = secp256k1_popcountl(n);
        size_t log = secp256k1_floor_lg(2 * n / IP_AB_SCALARS);
        return 32 * (1 + 2 * (bit_count - 1 + log) + IP_AB_SCALARS) + (2*log + 7) / 8;
    }
}

/* Our ecmult_multi function takes `(c - a*b)*x` directly and multiplies this by `G`. For every other
 * (scalar, point) pair it calls the following callback function, which takes an index and outputs a
 * pair. The function therefore has three regimes:
 *
 * For the first `n` invocations, it returns `(s'_i, G_i)` for `i` from 1 to `n`.
 * For the next `n` invocations, it returns `(s_i, H_i)` for `i` from 1 to `n`.
 * For the next `2*log2(n)` invocations it returns `(x_i^-2, L_i)` and `(x_i^2, R_i)`,
 *   alternating between the two choices, for `i` from 1 to `log2(n)`.
 *
 * For the remaining invocations it passes through to another callback, `rangeproof_cb_data` which
 * computes `P`. The reason for this is that in practice `P` is usually defined by another multiexp
 * rather than being a known point, and it is more efficient to compute one exponentiation.
 *
 * Inline we refer to the first `2n` coefficients as `s_i` for `i` from 0 to `2n-1`, since that
 * is the more convenient indexing. In particular we describe (a) how the indices `j` and `k`,
 * from the big comment block above, are chosen; and (b) when/how each `x_k^-2` is computed.
 */
static int secp256k1_bulletproof_innerproduct_vfy_ecmult_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_bulletproof_innerproduct_vfy_ecmult_context *ctx = (secp256k1_bulletproof_innerproduct_vfy_ecmult_context *) data;

    /* First 2N points use the standard Gi, Hi generators, and the scalars can be aggregated across proofs.
     * Inside this if clause, `idx` corresponds to the index `i` in the big comment, and runs from 0 to `2n-1`.
     * Also `ctx->vec_len` corresponds to `n`. */
    if (idx < 2 * ctx->vec_len) {
        /* Number of `a` scalars in the proof (same as number of `b` scalars in the proof). Will
         * be 2 except for very small proofs that have fewer than 2 scalars as input. */
        const size_t grouping = ctx->vec_len < IP_AB_SCALARS / 2 ? ctx->vec_len : IP_AB_SCALARS / 2;
        const size_t lg_grouping = secp256k1_floor_lg(grouping);
        size_t i;
        VERIFY_CHECK(lg_grouping == 0 || lg_grouping == 1); /* TODO support higher IP_AB_SCALARS */

        /* Determine whether we're multiplying by `G_i`s or `H_i`s. */
        if (idx < ctx->vec_len) {
            *pt = ctx->geng[idx];
        } else {
            *pt = ctx->genh[idx - ctx->vec_len];
        }

        secp256k1_scalar_clear(sc);
        /* Loop over all the different inner product proofs we might be doing at once. Since they
         * share generators `G_i` and `H_i`, we compute all of their scalars at once and add them.
         * For each proof we start with the "seed value" `ctx->proof[i].xcache[0]` (see next comment
         * for its meaning) from which every other scalar derived. We expect the caller to have
         * randomized this to ensure that this wanton addition cannot enable cancellation attacks.
         */
        for (i = 0; i < ctx->n_proofs; i++) {
            /* To recall from the introductory comment: most `s_i` values are computed by taking an
             * earlier `s_j` value and multiplying it by some `x_k^2`.
             *
             * We now explain the index `j`: it is the largest number with one fewer 1-bits than `i`.
             * Alternately, the most recently returned `s_j` where `j` has one fewer 1-bits than `i`.
             *
             * To ensure that `s_j` is available when we need it, on each iteration we define the
             * variable `cache_idx` which simply counts the 1-bits in `i`; before returning `s_i`
             * we store it in `ctx->proof[i].xcache[cache_idx]`. Then later, when we want "most
             * recently returned `s_j` with one fewer 1-bits than `i`, it'll be sitting right
             * there in `ctx->proof[i].xcache[cache_idx - 1]`.
             *
             * Note that `ctx->proof[i].xcache[0]` will always equal `-a_1 * prod_{i=1}^{n-1} x_i^-2`,
             * and we expect the caller to have set this.
             */
            const size_t cache_idx = secp256k1_popcountl(idx);
            secp256k1_scalar term;
            VERIFY_CHECK(cache_idx < SECP256K1_BULLETPROOF_MAX_DEPTH);
            /* For the special case `cache_idx == 0` (which is true iff `idx == 0`) there is nothing to do. */
            if (cache_idx > 0) {
                /* Otherwise, check if this is one of the special indices where we transition from `a_1` to `a_2`,
                 * from `a_2` to `b_1`, or from `b_1` to `b_2`. (For small proofs there is only one transition,
                 * from `a` to `b`.) */
                if (idx % (ctx->vec_len / grouping) == 0) {
                    const size_t abinv_idx = idx / (ctx->vec_len / grouping) - 1;
                    size_t prev_cache_idx;
                    /* Check if it's the even specialer index where we're transitioning from `a`s to `b`s, from
                     * `G`s to `H`s, and from `x_k^2`s to `x_k^-2`s. In rangeproof and circuit applications,
                     * the caller secretly has a variable `y` such that `H_i` is really `y^-i H_i` for `i` ranging
                     * from 0 to `n-1`. Rather than forcing the caller to tweak every `H_i` herself, which would
                     * be very slow and prevent precomputation, we instead multiply our cached `x_k^-2` values
                     * by `y^(-2^k)` respectively, which will ultimately result in every `s_i` we return having
                     * been multiplied by `y^-i`.
                     *
                     * This is an underhanded trick but the result is that all `n` powers of `y^-i` show up
                     * in the right place, and we only need log-many scalar squarings and multiplications.
                     */
                    if (idx == ctx->vec_len) {
                        secp256k1_scalar yinvn = ctx->proof[i].proof->yinv;
                        size_t j;
                        prev_cache_idx = secp256k1_popcountl(idx - 1);
                        for (j = 0; j < (size_t) secp256k1_ctzl(idx) - lg_grouping; j++) {
                            secp256k1_scalar_mul(&ctx->proof[i].xsqinvy[j], &ctx->proof[i].xsqinv[j], &yinvn);
                            secp256k1_scalar_sqr(&yinvn, &yinvn);
                        }
                        if (lg_grouping == 1) {
                            secp256k1_scalar_mul(&ctx->proof[i].abinv[2], &ctx->proof[i].abinv[2], &yinvn);
                            secp256k1_scalar_sqr(&yinvn, &yinvn);
                        }
                    } else {
                        prev_cache_idx = cache_idx - 1;
                    }
                    /* Regardless of specialness, we multiply by `a_2/a_1` or whatever the appropriate multiplier
                     * is. We expect the caller to have given these to us in the `ctx->proof[i].abinv` array. */
                    secp256k1_scalar_mul(
                        &ctx->proof[i].xcache[cache_idx],
                        &ctx->proof[i].xcache[prev_cache_idx],
                        &ctx->proof[i].abinv[abinv_idx]
                    );
                /* If it's *not* a special index, just multiply by the appropriate `x_k^2`, or `x_k^-2` in case
                 * we're in the `H_i` half of the multiexp. At this point we can explain the index `k`, which
                 * is computed in the variable `xsq_idx` (`xsqinv_idx` respectively). In light of our discussion
                 * of `j`, we see that this should be "the least significant bit that's 1 in `i` but not `i-1`."
                 * In other words, it is the number of trailing 0 bits in the index `i`. */
                } else if (idx < ctx->vec_len) {
                    const size_t xsq_idx = secp256k1_ctzl(idx);
                    secp256k1_scalar_mul(&ctx->proof[i].xcache[cache_idx], &ctx->proof[i].xcache[cache_idx - 1], &ctx->proof[i].xsq[xsq_idx]);
                } else {
                    const size_t xsqinv_idx = secp256k1_ctzl(idx);
                    secp256k1_scalar_mul(&ctx->proof[i].xcache[cache_idx], &ctx->proof[i].xcache[cache_idx - 1], &ctx->proof[i].xsqinvy[xsqinv_idx]);
                }
            }
            term = ctx->proof[i].xcache[cache_idx];

            /* One last trick: compute `x_k^-2` while computing the `G_i` scalars, so that they'll be
             * available when we need them for the `H_i` scalars. We can do this for every `i` value
             * that has exactly one 0-bit, i.e. which is a product of all `x_i`s and one `x_k^-1`. By
             * multiplying that by the special value `prod_{i=1}^n x_i^-1` we obtain simply `x_k^-2`.
             * We expect the caller to give us this special value in `ctx->proof[i].xsqinv_mask`. */
            if (idx < ctx->vec_len / grouping && secp256k1_popcountl(idx) == ctx->lg_vec_len - 1) {
                const size_t xsqinv_idx = secp256k1_ctzl(~idx);
                secp256k1_scalar_mul(&ctx->proof[i].xsqinv[xsqinv_idx], &ctx->proof[i].xcache[cache_idx], &ctx->proof[i].xsqinv_mask);
            }

            /* Finally, if the caller, in its computation of `P`, wants to multiply `G_i` or `H_i` by some scalar,
             * we add that to our sum as well. Again, we trust the randomization in `xcache[0]` to prevent any
             * cancellation attacks here. */
            if (ctx->proof[i].proof->rangeproof_cb != NULL) {
                secp256k1_scalar rangeproof_offset;
                if ((ctx->proof[i].proof->rangeproof_cb)(&rangeproof_offset, NULL, &ctx->randomizer[i], idx, ctx->proof[i].proof->rangeproof_cb_data) != 1) {
                    return 0;
                }
                secp256k1_scalar_add(&term, &term, &rangeproof_offset);
            }

            secp256k1_scalar_add(sc, sc, &term);
        }
    /* Next 2lgN points are the L and R vectors */
    } else if (idx < 2 * (ctx->vec_len + ctx->lg_vec_len * ctx->n_proofs)) {
        size_t real_idx = idx - 2 * ctx->vec_len;
        const size_t proof_idx = real_idx / (2 * ctx->lg_vec_len);
        real_idx = real_idx % (2 * ctx->lg_vec_len);
        if (!secp256k1_bulletproof_deserialize_point(
            pt,
            ctx->proof[proof_idx].serialized_lr,
            real_idx,
            2 * ctx->lg_vec_len
        )) {
            return 0;
        }
        if (idx % 2 == 0) {
            *sc = ctx->proof[proof_idx].xsq[real_idx / 2];
        } else {
            *sc = ctx->proof[proof_idx].xsqinv[real_idx / 2];
        }
        secp256k1_scalar_mul(sc, sc, &ctx->randomizer[proof_idx]);
    /* After the G's, H's, L's and R's, do the blinding_gen */
    } else if (idx == 2 * (ctx->vec_len + ctx->lg_vec_len * ctx->n_proofs)) {
        *sc = ctx->p_offs;
        *pt = *ctx->g;
    /* Remaining points are whatever the rangeproof wants */
    } else if (ctx->shared_g && idx == 2 * (ctx->vec_len + ctx->lg_vec_len * ctx->n_proofs) + 1) {
        /* Special case: the first extra point is independent of the proof, for both rangeproof and circuit */
        size_t i;
        secp256k1_scalar_clear(sc);
        for (i = 0; i < ctx->n_proofs; i++) {
            secp256k1_scalar term;
            if ((ctx->proof[i].proof->rangeproof_cb)(&term, pt, &ctx->randomizer[i], 2 * (ctx->vec_len + ctx->lg_vec_len), ctx->proof[i].proof->rangeproof_cb_data) != 1) {
                return 0;
            }
            secp256k1_scalar_add(sc, sc, &term);
        }
    } else {
        size_t proof_idx = 0;
        size_t real_idx = idx - 2 * (ctx->vec_len + ctx->lg_vec_len * ctx->n_proofs) - 1 - !!ctx->shared_g;
        while (real_idx >= ctx->proof[proof_idx].proof->n_extra_rangeproof_points - !!ctx->shared_g) {
            real_idx -= ctx->proof[proof_idx].proof->n_extra_rangeproof_points - !!ctx->shared_g;
            proof_idx++;
            VERIFY_CHECK(proof_idx < ctx->n_proofs);
        }
        if ((ctx->proof[proof_idx].proof->rangeproof_cb)(sc, pt, &ctx->randomizer[proof_idx], 2 * (ctx->vec_len + ctx->lg_vec_len), ctx->proof[proof_idx].proof->rangeproof_cb_data) != 1) {
            return 0;
        }
    }

    return 1;
}

/* nb For security it is essential that `commit_inp` already commit to all data
 *    needed to compute `P`. We do not hash it in during verification since `P`
 *    may be specified indirectly as a bunch of scalar offsets.
 */
static int secp256k1_bulletproof_inner_product_verify_impl(const secp256k1_ecmult_context *ecmult_ctx, secp256k1_scratch *scratch, const secp256k1_bulletproof_generators *gens, size_t vec_len, const secp256k1_bulletproof_innerproduct_context *proof, size_t n_proofs, size_t plen, int shared_g) {
    secp256k1_sha256 sha256;
    secp256k1_bulletproof_innerproduct_vfy_ecmult_context ecmult_data;
    unsigned char commit[32];
    size_t total_n_points = 2 * vec_len + !!shared_g + 1; /* +1 for shared G (value_gen), +1 for H (blinding_gen) */
    secp256k1_gej r;
    secp256k1_scalar zero;
    size_t i;

    if (plen != secp256k1_bulletproof_innerproduct_proof_length(vec_len)) {
        return 0;
    }

    if (n_proofs == 0) {
        return 1;
    }

    if (!secp256k1_scratch_allocate_frame(scratch, n_proofs * (sizeof(*ecmult_data.randomizer) + sizeof(*ecmult_data.proof)), 2)) {
        return 0;
    }

    secp256k1_scalar_clear(&zero);
    ecmult_data.n_proofs = n_proofs;
    ecmult_data.g = gens->blinding_gen;
    ecmult_data.geng = gens->gens;
    ecmult_data.genh = gens->gens + gens->n / 2;
    ecmult_data.vec_len = vec_len;
    ecmult_data.lg_vec_len = secp256k1_floor_lg(2 * vec_len / IP_AB_SCALARS);
    ecmult_data.shared_g = shared_g;
    ecmult_data.randomizer = (secp256k1_scalar *)secp256k1_scratch_alloc(scratch, n_proofs * sizeof(*ecmult_data.randomizer));
    ecmult_data.proof = (secp256k1_bulletproof_innerproduct_vfy_data *)secp256k1_scratch_alloc(scratch, n_proofs * sizeof(*ecmult_data.proof));
    /* Seed RNG for per-proof randomizers */
    secp256k1_sha256_initialize(&sha256);
    for (i = 0; i < n_proofs; i++) {
        secp256k1_sha256_write(&sha256, proof[i].proof, plen);
        secp256k1_sha256_write(&sha256, proof[i].commit, 32);
        secp256k1_scalar_get_b32(commit, &proof[i].p_offs);
        secp256k1_sha256_write(&sha256, commit, 32);
    }
    secp256k1_sha256_finalize(&sha256, commit);

    secp256k1_scalar_clear(&ecmult_data.p_offs);
    for (i = 0; i < n_proofs; i++) {
        const unsigned char *serproof = proof[i].proof;
        unsigned char proof_commit[32];
        secp256k1_scalar dot;
        secp256k1_scalar ab[IP_AB_SCALARS];
        secp256k1_scalar negprod;
        secp256k1_scalar x;
        int overflow;
        size_t j;
        const size_t n_ab = 2 * vec_len < IP_AB_SCALARS ? 2 * vec_len : IP_AB_SCALARS;

        total_n_points += 2 * ecmult_data.lg_vec_len + proof[i].n_extra_rangeproof_points - !!shared_g; /* -1 for shared G */

        /* Extract dot product, will always be the first 32 bytes */
        secp256k1_scalar_set_b32(&dot, serproof, &overflow);
        if (overflow) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        /* Commit to dot product */
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, proof[i].commit, 32);
        secp256k1_sha256_write(&sha256, serproof, 32);
        secp256k1_sha256_finalize(&sha256, proof_commit);
        serproof += 32;

        /* Extract a, b */
        for (j = 0; j < n_ab; j++) {
            secp256k1_scalar_set_b32(&ab[j], serproof, &overflow);
            if (overflow) {
                secp256k1_scratch_deallocate_frame(scratch);
                return 0;
            }
            /* TODO our verifier currently bombs out with zeros because it uses
             * scalar inverses gratuitously. Fix that. */
            if (secp256k1_scalar_is_zero(&ab[j])) {
                secp256k1_scratch_deallocate_frame(scratch);
                return 0;
            }
            serproof += 32;
        }
        secp256k1_scalar_dot_product(&negprod, &ab[0], &ab[n_ab / 2], n_ab / 2);

        ecmult_data.proof[i].proof = &proof[i];
        /* set per-proof randomizer */
        secp256k1_sha256_initialize(&sha256);
        secp256k1_sha256_write(&sha256, commit, 32);
        secp256k1_sha256_finalize(&sha256, commit);
        secp256k1_scalar_set_b32(&ecmult_data.randomizer[i], commit, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ecmult_data.randomizer[i])) {
            /* cryptographically unreachable */
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }

        /* Compute x*(dot - a*b) for each proof; add it and p_offs to the p_offs accumulator */
        secp256k1_scalar_set_b32(&x, proof_commit, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&x)) {
            secp256k1_scratch_deallocate_frame(scratch);
            return 0;
        }
        secp256k1_scalar_negate(&negprod, &negprod);
        secp256k1_scalar_add(&negprod, &negprod, &dot);
        secp256k1_scalar_mul(&x, &x, &negprod);
        secp256k1_scalar_add(&x, &x, &proof[i].p_offs);

        secp256k1_scalar_mul(&x, &x, &ecmult_data.randomizer[i]);
        secp256k1_scalar_add(&ecmult_data.p_offs, &ecmult_data.p_offs, &x);

        /* Special-case: trivial proofs are valid iff the explicitly revealed scalars
         *               dot to the explicitly revealed dot product. */
        if (2 * vec_len <= IP_AB_SCALARS) {
            if (!secp256k1_scalar_is_zero(&negprod)) {
                secp256k1_scratch_deallocate_frame(scratch);
                return 0;
            }
            /* remaining data does not (and cannot) be computed for proofs with no a's or b's. */
            if (vec_len == 0) {
                continue;
            }
        }

        /* Compute the inverse product and the array of squares; the rest will be filled
         * in by the callback during the multiexp. */
        ecmult_data.proof[i].serialized_lr = serproof; /* bookmark L/R location in proof */
        negprod = ab[n_ab - 1];
        ab[n_ab - 1] = ecmult_data.randomizer[i]; /* build r * x1 * x2 * ... * xn in last slot of `ab` array */
        for (j = 0; j < ecmult_data.lg_vec_len; j++) {
            secp256k1_scalar xi;
            const size_t lidx = 2 * j;
            const size_t ridx = 2 * j + 1;
            const size_t bitveclen = (2 * ecmult_data.lg_vec_len + 7) / 8;
            const unsigned char lrparity = 2 * !!(serproof[lidx / 8] & (1 << (lidx % 8))) + !!(serproof[ridx / 8] & (1 << (ridx % 8)));
            /* Map commit -> H(commit || LR parity || Lx || Rx), compute xi from it */
            secp256k1_sha256_initialize(&sha256);
            secp256k1_sha256_write(&sha256, proof_commit, 32);
            secp256k1_sha256_write(&sha256, &lrparity, 1);
            secp256k1_sha256_write(&sha256, &serproof[32 * lidx + bitveclen], 32);
            secp256k1_sha256_write(&sha256, &serproof[32 * ridx + bitveclen], 32);
            secp256k1_sha256_finalize(&sha256, proof_commit);

            secp256k1_scalar_set_b32(&xi, proof_commit, &overflow);
            if (overflow || secp256k1_scalar_is_zero(&xi)) {
                secp256k1_scratch_deallocate_frame(scratch);
                return 0;
            }
            secp256k1_scalar_mul(&ab[n_ab - 1], &ab[n_ab - 1], &xi);
            secp256k1_scalar_sqr(&ecmult_data.proof[i].xsq[j], &xi);
        }
        /* Compute inverse of all a's and b's, except the last b whose inverse is not needed.
         * Also compute the inverse of (-r * x1 * ... * xn) which will be needed */
        secp256k1_scalar_inverse_all_var(ecmult_data.proof[i].abinv, ab, n_ab);
        ab[n_ab - 1] = negprod;

        /* Compute (-a0 * r * x1 * ... * xn)^-1 which will be used to mask out individual x_i^-2's */
        secp256k1_scalar_negate(&ecmult_data.proof[i].xsqinv_mask, &ecmult_data.proof[i].abinv[0]);
        secp256k1_scalar_mul(&ecmult_data.proof[i].xsqinv_mask, &ecmult_data.proof[i].xsqinv_mask, &ecmult_data.proof[i].abinv[n_ab - 1]);

        /* Compute each scalar times the previous' inverse, which is used to switch between a's and b's */
        for (j = n_ab - 1; j > 0; j--) {
            size_t prev_idx;
            if (j == n_ab / 2) {
                prev_idx = j - 1; /* we go from a_{n-1} to b_0  */
            } else {
                prev_idx = j & (j - 1); /* but from a_i' to a_i, where i' is i with its lowest set bit unset */
            }
            secp256k1_scalar_mul(
                &ecmult_data.proof[i].abinv[j - 1],
                &ecmult_data.proof[i].abinv[prev_idx],
                &ab[j]
            );
        }

        /* Extract -a0 * r * (x1 * ... * xn)^-1 which is our first coefficient. Use negprod as a dummy */
        secp256k1_scalar_mul(&negprod, &ecmult_data.randomizer[i], &ab[0]); /* r*a */
        secp256k1_scalar_sqr(&negprod, &negprod); /* (r*a)^2 */
        secp256k1_scalar_mul(&ecmult_data.proof[i].xcache[0], &ecmult_data.proof[i].xsqinv_mask, &negprod);  /* -a * r * (x1 * x2 * ... * xn)^-1 */
    }

    /* Do the multiexp */
    if (secp256k1_ecmult_multi_var(ecmult_ctx, scratch, &r, NULL, secp256k1_bulletproof_innerproduct_vfy_ecmult_callback, (void *) &ecmult_data, total_n_points) != 1) {
        secp256k1_scratch_deallocate_frame(scratch);
        return 0;
    }
    secp256k1_scratch_deallocate_frame(scratch);
    return secp256k1_gej_is_infinity(&r);
}

typedef struct {
    secp256k1_scalar x[SECP256K1_BULLETPROOF_MAX_DEPTH];
    secp256k1_scalar xinv[SECP256K1_BULLETPROOF_MAX_DEPTH];
    secp256k1_scalar yinv;
    secp256k1_scalar yinvn;
    const secp256k1_ge *geng;
    const secp256k1_ge *genh;
    const secp256k1_ge *g;
    const secp256k1_scalar *a;
    const secp256k1_scalar *b;
    secp256k1_scalar g_sc;
    size_t grouping;
    size_t n;
} secp256k1_bulletproof_innerproduct_pf_ecmult_context;

/* At each level i of recursion (i from 0 upto lg(vector size) - 1)
 *   L = a_even . G_odd + b_odd . H_even (18)
 * which, by expanding the generators into the original G's and H's
 * and setting n = (1 << i), can be computed as follows:
 *
 * For j from 1 to [vector size],
 *    1. Use H[j] or G[j] as generator, starting with H and switching
 *       every n.
 *    2. Start with b1 with H and a0 with G, and increment by 2 each switch.
 *    3. For k = 1, 2, 4, ..., n/2, use the same algorithm to choose
 *       between a and b to choose between x and x^-1, except using
 *       k in place of n. With H's choose x then x^-1, with G's choose
 *       x^-1 then x.
 *
 * For R everything is the same except swap G/H and a/b and x/x^-1.
 */
static int secp256k1_bulletproof_innerproduct_pf_ecmult_callback_l(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_bulletproof_innerproduct_pf_ecmult_context *ctx = (secp256k1_bulletproof_innerproduct_pf_ecmult_context *) data;
    const size_t ab_idx = (idx / ctx->grouping) ^ 1;
    size_t i;

    /* Special-case the primary generator */
    if (idx == ctx->n) {
        *pt = *ctx->g;
        *sc = ctx->g_sc;
        return 1;
    }

    /* steps 1/2 */
    if ((idx / ctx->grouping) % 2 == 0) {
        *pt = ctx->genh[idx];
        *sc = ctx->b[ab_idx];
        /* Map h -> h' (eqn 59) */
        secp256k1_scalar_mul(sc, sc, &ctx->yinvn);
    } else {
        *pt = ctx->geng[idx];
        *sc = ctx->a[ab_idx];
    }

    /* step 3 */
    for (i = 0; (1u << i) < ctx->grouping; i++) {
        size_t grouping = (1u << i);
        if ((((idx / grouping) % 2) ^ ((idx / ctx->grouping) % 2)) == 0) {
            secp256k1_scalar_mul(sc, sc, &ctx->x[i]);
        } else {
            secp256k1_scalar_mul(sc, sc, &ctx->xinv[i]);
        }
    }

    secp256k1_scalar_mul(&ctx->yinvn, &ctx->yinvn, &ctx->yinv);
    return 1;
}

/* Identical code except `== 0` changed to `== 1` twice, and the
 * `+ 1` from Step 1/2 was moved to the other if branch. */
static int secp256k1_bulletproof_innerproduct_pf_ecmult_callback_r(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_bulletproof_innerproduct_pf_ecmult_context *ctx = (secp256k1_bulletproof_innerproduct_pf_ecmult_context *) data;
    const size_t ab_idx = (idx / ctx->grouping) ^ 1;
    size_t i;

    /* Special-case the primary generator */
    if (idx == ctx->n) {
        *pt = *ctx->g;
        *sc = ctx->g_sc;
        return 1;
    }

    /* steps 1/2 */
    if ((idx / ctx->grouping) % 2 == 1) {
        *pt = ctx->genh[idx];
        *sc = ctx->b[ab_idx];
        /* Map h -> h' (eqn 59) */
        secp256k1_scalar_mul(sc, sc, &ctx->yinvn);
    } else {
        *pt = ctx->geng[idx];
        *sc = ctx->a[ab_idx];
    }

    /* step 3 */
    for (i = 0; (1u << i) < ctx->grouping; i++) {
        size_t grouping = (1u << i);
        if ((((idx / grouping) % 2) ^ ((idx / ctx->grouping) % 2)) == 1) {
            secp256k1_scalar_mul(sc, sc, &ctx->x[i]);
        } else {
            secp256k1_scalar_mul(sc, sc, &ctx->xinv[i]);
        }
    }

    secp256k1_scalar_mul(&ctx->yinvn, &ctx->yinvn, &ctx->yinv);
    return 1;
}

static int secp256k1_bulletproof_innerproduct_pf_ecmult_callback_g(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_bulletproof_innerproduct_pf_ecmult_context *ctx = (secp256k1_bulletproof_innerproduct_pf_ecmult_context *) data;
    size_t i;

    *pt = ctx->geng[idx];
    secp256k1_scalar_set_int(sc, 1);
    for (i = 0; (1u << i) <= ctx->grouping; i++) {
        if (idx & (1u << i)) {
            secp256k1_scalar_mul(sc, sc, &ctx->x[i]);
        } else {
            secp256k1_scalar_mul(sc, sc, &ctx->xinv[i]);
        }
    }
    return 1;
}

static int secp256k1_bulletproof_innerproduct_pf_ecmult_callback_h(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data) {
    secp256k1_bulletproof_innerproduct_pf_ecmult_context *ctx = (secp256k1_bulletproof_innerproduct_pf_ecmult_context *) data;
    size_t i;

    *pt = ctx->genh[idx];
    secp256k1_scalar_set_int(sc, 1);
    for (i = 0; (1u << i) <= ctx->grouping; i++) {
        if (idx & (1u << i)) {
            secp256k1_scalar_mul(sc, sc, &ctx->xinv[i]);
        } else {
            secp256k1_scalar_mul(sc, sc, &ctx->x[i]);
        }
    }
    secp256k1_scalar_mul(sc, sc, &ctx->yinvn);
    secp256k1_scalar_mul(&ctx->yinvn, &ctx->yinvn, &ctx->yinv);
    return 1;
}

/* These proofs are not zero-knowledge. There is no need to worry about constant timeness.
 * `commit_inp` must contain 256 bits of randomness, it is used immediately as a randomizer.
 */
static int secp256k1_bulletproof_inner_product_real_prove_impl(const secp256k1_ecmult_context *ecmult_ctx, secp256k1_scratch *scratch, secp256k1_ge *out_pt, size_t *pt_idx, const secp256k1_ge *g, secp256k1_ge *geng, secp256k1_ge *genh, secp256k1_scalar *a_arr, secp256k1_scalar *b_arr, const secp256k1_scalar *yinv, const secp256k1_scalar *ux, const size_t n, unsigned char *commit) {
    size_t i;
    size_t halfwidth;

    secp256k1_bulletproof_innerproduct_pf_ecmult_context pfdata;
    pfdata.yinv = *yinv;
    pfdata.g = g;
    pfdata.geng = geng;
    pfdata.genh = genh;
    pfdata.a = a_arr;
    pfdata.b = b_arr;
    pfdata.n = n;

    /* Protocol 1: Iterate, halving vector size until it is 1 */
    for (halfwidth = n / 2, i = 0; halfwidth > IP_AB_SCALARS / 4; halfwidth /= 2, i++) {
        secp256k1_gej tmplj, tmprj;
        size_t j;
        int overflow;

        pfdata.grouping = 1u << i;

        /* L */
        secp256k1_scalar_clear(&pfdata.g_sc);
        for (j = 0; j < halfwidth; j++) {
            secp256k1_scalar prod;
            secp256k1_scalar_mul(&prod, &a_arr[2*j], &b_arr[2*j + 1]);
            secp256k1_scalar_add(&pfdata.g_sc, &pfdata.g_sc, &prod);
        }
        secp256k1_scalar_mul(&pfdata.g_sc, &pfdata.g_sc, ux);

        secp256k1_scalar_set_int(&pfdata.yinvn, 1);
        secp256k1_ecmult_multi_var(ecmult_ctx, scratch, &tmplj, NULL, &secp256k1_bulletproof_innerproduct_pf_ecmult_callback_l, (void *) &pfdata, n + 1);
        secp256k1_ge_set_gej(&out_pt[(*pt_idx)++], &tmplj);

        /* R */
        secp256k1_scalar_clear(&pfdata.g_sc);
        for (j = 0; j < halfwidth; j++) {
            secp256k1_scalar prod;
            secp256k1_scalar_mul(&prod, &a_arr[2*j + 1], &b_arr[2*j]);
            secp256k1_scalar_add(&pfdata.g_sc, &pfdata.g_sc, &prod);
        }
        secp256k1_scalar_mul(&pfdata.g_sc, &pfdata.g_sc, ux);

        secp256k1_scalar_set_int(&pfdata.yinvn, 1);
        secp256k1_ecmult_multi_var(ecmult_ctx, scratch, &tmprj, NULL, &secp256k1_bulletproof_innerproduct_pf_ecmult_callback_r, (void *) &pfdata, n + 1);
        secp256k1_ge_set_gej(&out_pt[(*pt_idx)++], &tmprj);

        /* x, x^2, x^-1, x^-2 */
        secp256k1_bulletproof_update_commit(commit, &out_pt[*pt_idx - 2], &out_pt[*pt_idx] - 1);
        secp256k1_scalar_set_b32(&pfdata.x[i], commit, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&pfdata.x[i])) {
            return 0;
        }
        secp256k1_scalar_inverse_var(&pfdata.xinv[i], &pfdata.x[i]);

        /* update scalar array */
        for (j = 0; j < halfwidth; j++) {
            secp256k1_scalar tmps;
            secp256k1_scalar_mul(&a_arr[2*j], &a_arr[2*j], &pfdata.x[i]);
            secp256k1_scalar_mul(&tmps, &a_arr[2*j + 1], &pfdata.xinv[i]);
            secp256k1_scalar_add(&a_arr[j], &a_arr[2*j], &tmps);

            secp256k1_scalar_mul(&b_arr[2*j], &b_arr[2*j], &pfdata.xinv[i]);
            secp256k1_scalar_mul(&tmps, &b_arr[2*j + 1], &pfdata.x[i]);
            secp256k1_scalar_add(&b_arr[j], &b_arr[2*j], &tmps);

        }

        /* Combine G generators and recurse, if that would be more optimal */
        if ((n > 2048 && i == 3) || (n > 128 && i == 2) || (n > 32 && i == 1)) {
            secp256k1_scalar yinv2;

            for (j = 0; j < halfwidth; j++) {
                secp256k1_gej rj;
                secp256k1_ecmult_multi_var(ecmult_ctx, scratch, &rj, NULL, &secp256k1_bulletproof_innerproduct_pf_ecmult_callback_g, (void *) &pfdata, 2u << i);
                pfdata.geng += 2u << i;
                secp256k1_ge_set_gej(&geng[j], &rj);
                secp256k1_scalar_set_int(&pfdata.yinvn, 1);
                secp256k1_ecmult_multi_var(ecmult_ctx, scratch, &rj, NULL, &secp256k1_bulletproof_innerproduct_pf_ecmult_callback_h, (void *) &pfdata, 2u << i);
                pfdata.genh += 2u << i;
                secp256k1_ge_set_gej(&genh[j], &rj);
            }

            secp256k1_scalar_sqr(&yinv2, yinv);
            for (j = 0; j < i; j++) {
                secp256k1_scalar_sqr(&yinv2, &yinv2);
            }
            if (!secp256k1_bulletproof_inner_product_real_prove_impl(ecmult_ctx, scratch, out_pt, pt_idx, g, geng, genh, a_arr, b_arr, &yinv2, ux, halfwidth, commit)) {
                return 0;
            }
            break;
        }
    }
    return 1;
}

static int secp256k1_bulletproof_inner_product_prove_impl(const secp256k1_ecmult_context *ecmult_ctx, secp256k1_scratch *scratch, unsigned char *proof, size_t *proof_len, const secp256k1_bulletproof_generators *gens, const secp256k1_scalar *yinv, const size_t n, secp256k1_ecmult_multi_callback *cb, void *cb_data, const unsigned char *commit_inp) {
    secp256k1_sha256 sha256;
    size_t i;
    unsigned char commit[32];
    secp256k1_scalar *a_arr;
    secp256k1_scalar *b_arr;
    secp256k1_ge *out_pt;
    secp256k1_ge *geng;
    secp256k1_ge *genh;
    secp256k1_scalar ux;
    int overflow;
    size_t pt_idx = 0;
    secp256k1_scalar dot;
    size_t half_n_ab = n < IP_AB_SCALARS / 2 ? n : IP_AB_SCALARS / 2;

    if (*proof_len < secp256k1_bulletproof_innerproduct_proof_length(n)) {
        return 0;
    }
    *proof_len = secp256k1_bulletproof_innerproduct_proof_length(n);

    /* Special-case lengths 0 and 1 whose proofs are just explicit lists of scalars */
    if (n <= IP_AB_SCALARS / 2) {
        secp256k1_scalar a[IP_AB_SCALARS / 2];
        secp256k1_scalar b[IP_AB_SCALARS / 2];

        for (i = 0; i < n; i++) {
            cb(&a[i], NULL, 2*i, cb_data);
            cb(&b[i], NULL, 2*i+1, cb_data);
        }

        secp256k1_scalar_dot_product(&dot, a, b, n);
        secp256k1_scalar_get_b32(proof, &dot);

        for (i = 0; i < n; i++) {
            secp256k1_scalar_get_b32(&proof[32 * (i + 1)], &a[i]);
            secp256k1_scalar_get_b32(&proof[32 * (i + n + 1)], &b[i]);
        }
        VERIFY_CHECK(*proof_len == 32 * (2 * n + 1));
        return 1;
    }

    /* setup for nontrivial proofs */
    if (!secp256k1_scratch_allocate_frame(scratch, 2 * n * (sizeof(secp256k1_scalar) + sizeof(secp256k1_ge)) + 2 * secp256k1_floor_lg(n) * sizeof(secp256k1_ge), 5)) {
        return 0;
    }

    a_arr = (secp256k1_scalar*)secp256k1_scratch_alloc(scratch, n * sizeof(secp256k1_scalar));
    b_arr = (secp256k1_scalar*)secp256k1_scratch_alloc(scratch, n * sizeof(secp256k1_scalar));
    geng = (secp256k1_ge*)secp256k1_scratch_alloc(scratch, n * sizeof(secp256k1_ge));
    genh = (secp256k1_ge*)secp256k1_scratch_alloc(scratch, n * sizeof(secp256k1_ge));
    out_pt = (secp256k1_ge*)secp256k1_scratch_alloc(scratch, 2 * secp256k1_floor_lg(n) * sizeof(secp256k1_ge));
    VERIFY_CHECK(a_arr != NULL);
    VERIFY_CHECK(b_arr != NULL);
    VERIFY_CHECK(gens != NULL);

    for (i = 0; i < n; i++) {
        cb(&a_arr[i], NULL, 2*i, cb_data);
        cb(&b_arr[i], NULL, 2*i+1, cb_data);
        geng[i] = gens->gens[i];
        genh[i] = gens->gens[i + gens->n/2];
    }

    /* Record final dot product */
    secp256k1_scalar_dot_product(&dot, a_arr, b_arr, n);
    secp256k1_scalar_get_b32(proof, &dot);

    /* Protocol 2: hash dot product to obtain G-randomizer */
    secp256k1_sha256_initialize(&sha256);
    secp256k1_sha256_write(&sha256, commit_inp, 32);
    secp256k1_sha256_write(&sha256, proof, 32);
    secp256k1_sha256_finalize(&sha256, commit);

    proof += 32;

    secp256k1_scalar_set_b32(&ux, commit, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&ux)) {
        /* cryptographically unreachable */
        secp256k1_scratch_deallocate_frame(scratch);
        return 0;
    }

    if (!secp256k1_bulletproof_inner_product_real_prove_impl(ecmult_ctx, scratch, out_pt, &pt_idx, gens->blinding_gen, geng, genh, a_arr, b_arr, yinv, &ux, n, commit)) {
        secp256k1_scratch_deallocate_frame(scratch);
        return 0;
    }

    /* Final a/b values */
    for (i = 0; i < half_n_ab; i++) {
        secp256k1_scalar_get_b32(&proof[32 * i], &a_arr[i]);
        secp256k1_scalar_get_b32(&proof[32 * (i + half_n_ab)], &b_arr[i]);
    }
    proof += 64 * half_n_ab;
    secp256k1_bulletproof_serialize_points(proof, out_pt, pt_idx);

    secp256k1_scratch_deallocate_frame(scratch);
    return 1;
}

#undef IP_AB_SCALARS

#endif
