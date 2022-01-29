/**********************************************************************
 * Copyright (c) 2014-2015 Gregory Maxwell                            *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_COMMITMENT_MAIN
#define SECP256K1_MODULE_COMMITMENT_MAIN

#include "group.h"

#include "modules/commitment/pedersen_impl.h"

/** Alternative generator for secp256k1.
 *  This is the sha256 of 'g' after DER encoding (without compression),
 *  which happens to be a point on the curve.
 *  sage: G2 = EllipticCurve ([F (0), F (7)]).lift_x(F(int(hashlib.sha256('0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8'.decode('hex')).hexdigest(),16)))
 *  sage: '%x %x' % G2.xy()
 */
static const secp256k1_generator secp256k1_generator_h_internal = {{
    0x50, 0x92, 0x9b, 0x74, 0xc1, 0xa0, 0x49, 0x54, 0xb7, 0x8b, 0x4b, 0x60, 0x35, 0xe9, 0x7a, 0x5e,
    0x07, 0x8a, 0x5a, 0x0f, 0x28, 0xec, 0x96, 0xd5, 0x47, 0xbf, 0xee, 0x9a, 0xce, 0x80, 0x3a, 0xc0,
    0x31, 0xd3, 0xc6, 0x86, 0x39, 0x73, 0x92, 0x6e, 0x04, 0x9e, 0x63, 0x7c, 0xb1, 0xb5, 0xf4, 0x0a,
    0x36, 0xda, 0xc2, 0x8a, 0xf1, 0x76, 0x69, 0x68, 0xc3, 0x0c, 0x23, 0x13, 0xf3, 0xa3, 0x89, 0x04
}};

const secp256k1_generator *secp256k1_generator_h = &secp256k1_generator_h_internal;

static void secp256k1_pedersen_commitment_load(secp256k1_ge* ge, const secp256k1_pedersen_commitment* commit) {
    secp256k1_fe fe;
    secp256k1_fe_set_b32(&fe, &commit->data[1]);
    secp256k1_ge_set_xquad(ge, &fe);
    if (commit->data[0] & 1) {
        secp256k1_ge_neg(ge, ge);
    }
}

static void secp256k1_pedersen_commitment_save(secp256k1_pedersen_commitment* commit, secp256k1_ge* ge) {
    secp256k1_fe_normalize(&ge->x);
    secp256k1_fe_get_b32(&commit->data[1], &ge->x);
    commit->data[0] = 9 ^ secp256k1_fe_is_quad_var(&ge->y);
}

int secp256k1_pedersen_commitment_parse(const secp256k1_context* ctx, secp256k1_pedersen_commitment* commit, const unsigned char *input) {
    secp256k1_fe x;
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(input != NULL);
    (void) ctx;

    if ((input[0] & 0xFE) != 8 ||
        !secp256k1_fe_set_b32(&x, &input[1]) ||
        !secp256k1_ge_set_xquad(&ge, &x)) {
        return 0;
    }
    if (input[0] & 1) {
        secp256k1_ge_neg(&ge, &ge);
    }
    secp256k1_pedersen_commitment_save(commit, &ge);
    return 1;
}

int secp256k1_pedersen_commitment_serialize(const secp256k1_context* ctx, unsigned char *output, const secp256k1_pedersen_commitment* commit) {
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(commit != NULL);

    secp256k1_pedersen_commitment_load(&ge, commit);

    output[0] = 9 ^ secp256k1_fe_is_quad_var(&ge.y);
    secp256k1_fe_normalize_var(&ge.x);
    secp256k1_fe_get_b32(&output[1], &ge.x);
    return 1;
}

/* Generates a pedersen commitment: *commit = blind * G + value * G2. The blinding factor is 32 bytes.*/
int secp256k1_pedersen_commit(const secp256k1_context* ctx, secp256k1_pedersen_commitment *commit, const unsigned char *blind, uint64_t value, const secp256k1_generator* value_gen, const secp256k1_generator* blind_gen) {
    secp256k1_ge value_genp;
    secp256k1_ge blind_genp;
    secp256k1_gej rj;
    secp256k1_ge r;
    secp256k1_scalar sec;
    int overflow;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(blind != NULL);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK(blind_gen != NULL);
    secp256k1_generator_load(&value_genp, value_gen);
    secp256k1_generator_load(&blind_genp, blind_gen);
    secp256k1_scalar_set_b32(&sec, blind, &overflow);
    if (!overflow) {
        secp256k1_pedersen_ecmult(&rj, &sec, value, &value_genp, &blind_genp);
        if (!secp256k1_gej_is_infinity(&rj)) {
            secp256k1_ge_set_gej(&r, &rj);
            secp256k1_pedersen_commitment_save(commit, &r);
            ret = 1;
        }
        secp256k1_gej_clear(&rj);
        secp256k1_ge_clear(&r);
    }
    secp256k1_scalar_clear(&sec);
    return ret;
}

/* Generates a pedersen commitment: *commit = blind * G + value * G2. The blinding factor is 32 bytes.*/
int secp256k1_pedersen_blind_commit(const secp256k1_context* ctx, secp256k1_pedersen_commitment *commit, const unsigned char *blind, const unsigned char *value, const secp256k1_generator* value_gen, const secp256k1_generator* blind_gen) {
    secp256k1_ge value_genp;
    secp256k1_ge blind_genp;
    secp256k1_gej rj;
    secp256k1_ge r;
    secp256k1_scalar sec;
    secp256k1_scalar sec2;
    int overflow;
    int overflow2;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(commit != NULL);
    ARG_CHECK(blind != NULL);
    ARG_CHECK(value != NULL);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK(blind_gen != NULL);
    secp256k1_generator_load(&value_genp, value_gen);
    secp256k1_generator_load(&blind_genp, blind_gen);
    secp256k1_scalar_set_b32(&sec, blind, &overflow);
    secp256k1_scalar_set_b32(&sec2, value, &overflow2);
    if (!overflow && !overflow2) {
        secp256k1_pedersen_blind_ecmult(&rj, &sec, &sec2, &value_genp, &blind_genp);
        if (!secp256k1_gej_is_infinity(&rj)) {
            secp256k1_ge_set_gej(&r, &rj);
            secp256k1_pedersen_commitment_save(commit, &r);
            ret = 1;
        }
        secp256k1_gej_clear(&rj);
        secp256k1_ge_clear(&r);
    }
    secp256k1_scalar_clear(&sec);
    return ret;
}

int secp256k1_pedersen_commitment_to_pubkey(const secp256k1_context* ctx, secp256k1_pubkey* pubkey, const secp256k1_pedersen_commitment* commit) {
    secp256k1_ge Q;
    secp256k1_fe fe;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    memset(pubkey, 0, sizeof(*pubkey));
    ARG_CHECK(commit != NULL);

    secp256k1_fe_set_b32(&fe, &commit->data[1]);
    secp256k1_ge_set_xquad(&Q, &fe);
    if (commit->data[0] & 1) {
        secp256k1_ge_neg(&Q, &Q);
    }
    secp256k1_pubkey_save(pubkey, &Q);
    secp256k1_ge_clear(&Q);
    return 1;
}

/** Takes a list of n pointers to 32 byte blinding values, the first negs of which are treated with positive sign and the rest
 *  negative, then calculates an additional blinding value that adds to zero.
 */
int secp256k1_pedersen_blind_sum(const secp256k1_context* ctx, unsigned char *blind_out, const unsigned char * const *blinds, size_t n, size_t npositive) {
    secp256k1_scalar acc;
    secp256k1_scalar x;
    size_t i;
    int overflow;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(blind_out != NULL);
    ARG_CHECK(blinds != NULL);
    ARG_CHECK(npositive <= n);
    (void) ctx;
    secp256k1_scalar_set_int(&acc, 0);
    for (i = 0; i < n; i++) {
        secp256k1_scalar_set_b32(&x, blinds[i], &overflow);
        if (overflow) {
            return 0;
        }
        if (i >= npositive) {
            secp256k1_scalar_negate(&x, &x);
        }
        secp256k1_scalar_add(&acc, &acc, &x);
    }
    secp256k1_scalar_get_b32(blind_out, &acc);
    secp256k1_scalar_clear(&acc);
    secp256k1_scalar_clear(&x);
    return 1;
}

/* Takes two list of 33-byte commitments and sums the first set, subtracts the second and returns the resulting commitment. */
int secp256k1_pedersen_commit_sum(const secp256k1_context* ctx, secp256k1_pedersen_commitment *commit_out,
 const secp256k1_pedersen_commitment * const* commits, size_t pcnt, const secp256k1_pedersen_commitment * const* ncommits, size_t ncnt) {
    secp256k1_gej accj;
    secp256k1_ge add;
    size_t i;
    int ret = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(!pcnt || (commits != NULL));
    ARG_CHECK(!ncnt || (ncommits != NULL));
    ARG_CHECK(commit_out != NULL);
    (void) ctx;
    secp256k1_gej_set_infinity(&accj);
    for (i = 0; i < ncnt; i++) {
        secp256k1_pedersen_commitment_load(&add, ncommits[i]);
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    secp256k1_gej_neg(&accj, &accj);
    for (i = 0; i < pcnt; i++) {
        secp256k1_pedersen_commitment_load(&add, commits[i]);
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    if (!secp256k1_gej_is_infinity(&accj)) {
        secp256k1_ge acc;
        secp256k1_ge_set_gej(&acc, &accj);
        secp256k1_pedersen_commitment_save(commit_out, &acc);
        ret = 1;
    }
    return ret;
}

/* Takes two lists of commitments and sums the first set and subtracts the second and verifies that they sum to excess. */
int secp256k1_pedersen_verify_tally(const secp256k1_context* ctx, const secp256k1_pedersen_commitment * const* pos, size_t n_pos, const secp256k1_pedersen_commitment * const* neg, size_t n_neg) {
    secp256k1_gej accj;
    secp256k1_ge add;
    size_t i;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(!n_pos || (pos != NULL));
    ARG_CHECK(!n_neg || (neg != NULL));
    (void) ctx;
    secp256k1_gej_set_infinity(&accj);
    for (i = 0; i < n_neg; i++) {
        secp256k1_pedersen_commitment_load(&add, neg[i]);
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    secp256k1_gej_neg(&accj, &accj);
    for (i = 0; i < n_pos; i++) {
        secp256k1_pedersen_commitment_load(&add, pos[i]);
        secp256k1_gej_add_ge_var(&accj, &accj, &add, NULL);
    }
    return secp256k1_gej_is_infinity(&accj);
}

int secp256k1_pedersen_blind_generator_blind_sum(const secp256k1_context* ctx, const uint64_t *value, const unsigned char* const* generator_blind, unsigned char* const* blinding_factor, size_t n_total, size_t n_inputs) {
    secp256k1_scalar sum;
    secp256k1_scalar tmp;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(n_total == 0 || value != NULL);
    ARG_CHECK(n_total == 0 || generator_blind != NULL);
    ARG_CHECK(n_total == 0 || blinding_factor != NULL);
    ARG_CHECK(n_total > n_inputs);
    (void) ctx;

    if (n_total == 0) {
        return 1;
    }

    secp256k1_scalar_set_int(&sum, 0);
    for (i = 0; i < n_total; i++) {
        int overflow = 0;
        secp256k1_scalar addend;
        secp256k1_scalar_set_u64(&addend, value[i]);  /* s = v */

        secp256k1_scalar_set_b32(&tmp, generator_blind[i], &overflow);
        if (overflow == 1) {
            secp256k1_scalar_clear(&tmp);
            secp256k1_scalar_clear(&addend);
            secp256k1_scalar_clear(&sum);
            return 0;
        }
        secp256k1_scalar_mul(&addend, &addend, &tmp); /* s = vr */

        secp256k1_scalar_set_b32(&tmp, blinding_factor[i], &overflow);
        if (overflow == 1) {
            secp256k1_scalar_clear(&tmp);
            secp256k1_scalar_clear(&addend);
            secp256k1_scalar_clear(&sum);
            return 0;
        }
        secp256k1_scalar_add(&addend, &addend, &tmp); /* s = vr + r' */
        secp256k1_scalar_cond_negate(&addend, i < n_inputs);  /* s is negated if it's an input */
        secp256k1_scalar_add(&sum, &sum, &addend);    /* sum += s */
        secp256k1_scalar_clear(&addend);
    }

    /* Right now tmp has the last pedersen blinding factor. Subtract the sum from it. */
    secp256k1_scalar_negate(&sum, &sum);
    secp256k1_scalar_add(&tmp, &tmp, &sum);
    secp256k1_scalar_get_b32(blinding_factor[n_total - 1], &tmp);

    secp256k1_scalar_clear(&tmp);
    secp256k1_scalar_clear(&sum);
    return 1;
}

/* Generates a blinding key that contains a hashed switch commitment. */
int secp256k1_blind_switch(const secp256k1_context* ctx, unsigned char* blind_switch, const unsigned char* blind, uint64_t value, const secp256k1_generator* value_gen, const secp256k1_generator* blind_gen, const secp256k1_pubkey* switch_pubkey) {
    secp256k1_sha256 hasher;
    secp256k1_pedersen_commitment commit;
    unsigned char buf[33];
    size_t buflen = sizeof(buf);
    unsigned char hashed[32];
    int overflow;
    secp256k1_scalar blind_switch_scalar;
    secp256k1_pubkey tmp_pubkey;
    secp256k1_scalar tmp_scalar;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(blind_switch != NULL);
    ARG_CHECK(blind != NULL);
    ARG_CHECK(value_gen != NULL);
    ARG_CHECK(blind_gen != NULL);
    ARG_CHECK(switch_pubkey != NULL);

    secp256k1_sha256_initialize(&hasher);
    /* xG + vH */
    if (secp256k1_pedersen_commit(ctx, &commit, blind, value, value_gen, blind_gen) != 1) {
        return 0;
    }
    if (secp256k1_pedersen_commitment_serialize(ctx, buf, &commit) != 1) {
        return 0;
    }
    secp256k1_sha256_write(&hasher, buf, buflen);

    /* xJ */
    tmp_pubkey = *switch_pubkey;
    if (secp256k1_ec_pubkey_tweak_mul(ctx, &tmp_pubkey, blind) != 1) {
        return 0;
    }
    if (secp256k1_ec_pubkey_serialize(ctx, buf, &buflen, &tmp_pubkey, SECP256K1_EC_COMPRESSED) != 1) {
        return 0;
    }
    secp256k1_sha256_write(&hasher, buf, buflen);
    secp256k1_sha256_finalize(&hasher, hashed);
    secp256k1_scalar_set_b32(&blind_switch_scalar, hashed, &overflow); /* hash(xG+vH||xJ) */
    if (overflow) {
        secp256k1_scalar_clear(&blind_switch_scalar);
        return 0;
    }
    secp256k1_scalar_set_b32(&tmp_scalar, blind, &overflow);
    if (overflow) {
        secp256k1_scalar_clear(&blind_switch_scalar);
        secp256k1_scalar_clear(&tmp_scalar);
        return 0;
    }
    secp256k1_scalar_add(&blind_switch_scalar, &blind_switch_scalar, &tmp_scalar); /* x + hash(xG+vH||xJ) */
    secp256k1_scalar_get_b32(blind_switch, &blind_switch_scalar);
    secp256k1_scalar_clear(&blind_switch_scalar);
    secp256k1_scalar_clear(&tmp_scalar);
    return 1;
}

#endif
