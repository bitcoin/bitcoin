/**********************************************************************
 * Copyright (c) 2017 The Particl Core developers                     *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MLSAG_MAIN
#define SECP256K1_MLSAG_MAIN


static void pedersen_commitment_load(secp256k1_ge *ge, const uint8_t *commit) {
    secp256k1_fe fe;
    secp256k1_fe_set_b32(&fe, &commit[1]);
    secp256k1_ge_set_xquad(ge, &fe);
    if (commit[0] & 1) {
        secp256k1_ge_neg(ge, ge);
    }
}

static void pedersen_commitment_save(uint8_t *commit, secp256k1_ge *ge) {
    secp256k1_fe_normalize(&ge->x);
    secp256k1_fe_get_b32(&commit[1], &ge->x);
    commit[0] = 9 ^ secp256k1_fe_is_quad_var(&ge->y);
}

static int load_ge(secp256k1_ge *ge, const uint8_t *data, size_t len)
{
    if (len == 33 && (data[0] == 0x08 || data[0] == 0x09))
    {
        pedersen_commitment_load(ge, data);
        return 1;
    }
    return secp256k1_eckey_pubkey_parse(ge, data, len);
}

int secp256k1_prepare_mlsag(uint8_t *m, uint8_t *sk,
    size_t nOuts, size_t nBlinded, size_t nCols, size_t nRows,
    const uint8_t **pcm_in, const uint8_t **pcm_out, const uint8_t **blinds)
{
    /*
        Last matrix row is sum of input commitments - sum of output commitments

        Will return after summing commitments if sk or blinds is null

        m[col+(cols*row)]
        pcm_in[col+(cols*row)]
        pcm_out[nOuts]

        blinds[nBlinded]  array of pointers to 32byte blinding keys, inputs and outputs

        no. of inputs is nRows -1

        sum blinds up to nBlinded, pass fee commitment in pcm_out after nBlinded

    */

    secp256k1_gej accj;
    secp256k1_ge c, cno;
    size_t nIns = nRows -1;
    size_t s, i, k;
    int overflow;
    secp256k1_scalar accos, accis, ts;

    if (!m
        || nRows < 2
        || nCols < 1
        || nOuts < 1)
        return 1;

    /* sum output commitments */
    secp256k1_gej_set_infinity(&accj);
    for (k = 0; k < nOuts; ++k)
    {
        if (!load_ge(&c, pcm_out[k], 33))
            return 2;

        secp256k1_gej_add_ge_var(&accj, &accj, &c, NULL);
    };

    secp256k1_gej_neg(&accj, &accj);
    secp256k1_ge_set_gej(&cno, &accj);

    for (k = 0; k < nCols; ++k)
    {
        /* sum column input commitments */
        secp256k1_gej_set_infinity(&accj);
        for (i = 0; i < nIns; ++i)
        {
            if (!load_ge(&c, pcm_in[k+nCols*i], 33))
                return 3;

            secp256k1_gej_add_ge_var(&accj, &accj, &c, NULL);
        };

        /* subtract output commitments */
        secp256k1_gej_add_ge_var(&accj, &accj, &cno, NULL);

        /* store in last row, nRows -1 */
        if (secp256k1_gej_is_infinity(&accj))
        { /* With no blinds set, sum input commitments == sum output commitments */
            memset(&m[(k+nCols*nIns)*33], 0, 33); /* consistent infinity point */
            continue;
        };
        secp256k1_ge_set_gej(&c, &accj);
        secp256k1_eckey_pubkey_serialize(&c, &m[(k+nCols*nIns)*33], &s, 1);
        /* pedersen_commitment_save(&m[(k+nCols*nIns)*33], &c); */
    };

    if (!sk || !blinds)
        return 0;

    /* sum input blinds */
    secp256k1_scalar_clear(&accis);
    for (k = 0; k < nIns; ++k)
    {
        secp256k1_scalar_set_b32(&ts, blinds[k], &overflow);
        if (overflow)
            return 5;

        secp256k1_scalar_add(&accis, &accis, &ts);
    };

    /* sum output blinds */
    secp256k1_scalar_clear(&accos);
    for (k = 0; k < nBlinded; ++k)
    {
        secp256k1_scalar_set_b32(&ts, blinds[nIns+k], &overflow);
        if (overflow)
            return 5;

        secp256k1_scalar_add(&accos, &accos, &ts);
    };

    secp256k1_scalar_negate(&accos, &accos);

    /* subtract output blinds */
    secp256k1_scalar_add(&ts, &accis, &accos);

    secp256k1_scalar_get_b32(sk, &ts);

    return 0;
}

static int hash_to_curve(secp256k1_ge *ge, const uint8_t *pd, size_t len)
{
    secp256k1_fe x;
    uint8_t hash[32];
    size_t k, safety = 128;
    secp256k1_sha256_t sha256_m;

    secp256k1_sha256_initialize(&sha256_m);
    secp256k1_sha256_write(&sha256_m, pd, len);
    secp256k1_sha256_finalize(&sha256_m, hash);

    for (k = 0; k < safety; ++k)
    {
        if (secp256k1_fe_set_b32(&x, hash)
            && secp256k1_ge_set_xo_var(ge, &x, 0)
            && secp256k1_ge_is_valid_var(ge)) /* Is secp256k1_ge_is_valid_var necessary? */
            break;

        secp256k1_sha256_initialize(&sha256_m);
        secp256k1_sha256_write(&sha256_m, hash, 32);
        secp256k1_sha256_finalize(&sha256_m, hash);
    };

    if (k == safety)
        return 1; /* failed */

    return 0;
}

int secp256k1_get_keyimage(const secp256k1_context *ctx, uint8_t *ki, const uint8_t *pk, const uint8_t *sk)
{
    secp256k1_ge ge1;
    secp256k1_scalar s, zero;
    secp256k1_gej gej1, gej2;
    int overflow;
    size_t clen;

    secp256k1_scalar_set_int(&zero, 0);

    if (0 != hash_to_curve(&ge1, pk, 33)) /* H(pk) */
        return 1;

    secp256k1_scalar_set_b32(&s, sk, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&s))
        return 2;

    secp256k1_gej_set_ge(&gej1, &ge1);
    secp256k1_ecmult(&ctx->ecmult_ctx, &gej2, &gej1, &s, &zero);  /* gej2 = H(pk) * sk */
    secp256k1_ge_set_gej(&ge1, &gej2);
    secp256k1_eckey_pubkey_serialize(&ge1, ki, &clen, 1);

    return (clen == 33) ? 0 : 3;
}

#define MLSAG_MAX_ROWS 33 /* arbitrary max rows, max inputs 32 */
int secp256k1_generate_mlsag(const secp256k1_context *ctx,
    uint8_t *ki, uint8_t *pc, uint8_t *ps,
    const uint8_t *nonce, const uint8_t *preimage, size_t nCols,
    size_t nRows, size_t index, const uint8_t **sk, const uint8_t *pk)
{
    /* nRows == nInputs + 1, last row sums commitments
    */

    secp256k1_rfc6979_hmac_sha256_t rng;
    secp256k1_sha256_t sha256_m, sha256_pre;
    size_t dsRows = nRows-1; /* TODO: pass in dsRows explicitly? */
    /* secp256k1_scalar alpha[nRows]; */
    secp256k1_scalar alpha[MLSAG_MAX_ROWS]; /* To remove MLSAG_MAX_ROWS limit, malloc 32 * nRows for alpha  */
    secp256k1_scalar zero, clast, s, ss;
    secp256k1_pubkey pubkey;
    secp256k1_ge ge1;
    secp256k1_gej gej1, gej2, L, R;
    uint8_t tmp[32 + 32];
    size_t i, k, clen;
    int overflow;

    if (!pk
        || nRows < 2
        || nCols < 1
        || nRows > MLSAG_MAX_ROWS)
        return 1;

    secp256k1_scalar_set_int(&zero, 0);

    memcpy(tmp, nonce, 32);
    memcpy(tmp+32, preimage, 32);

    /* seed the random no. generator */
    secp256k1_rfc6979_hmac_sha256_initialize(&rng, tmp, 32 + 32);

    secp256k1_sha256_initialize(&sha256_m);
    secp256k1_sha256_write(&sha256_m, preimage, 32);
    sha256_pre = sha256_m;

    for (k = 0; k < dsRows; ++k)
    {
        do {
            secp256k1_rfc6979_hmac_sha256_generate(&rng, tmp, 32);
            secp256k1_scalar_set_b32(&alpha[k], tmp, &overflow);
        } while (overflow || secp256k1_scalar_is_zero(&alpha[k]));

        if (!secp256k1_ec_pubkey_create(ctx, &pubkey, tmp))  /* G * alpha[col] */
            return 1;
        clen = 33; /* must be set */
        if (!secp256k1_ec_pubkey_serialize(ctx, tmp, &clen, &pubkey, SECP256K1_EC_COMPRESSED)
            || clen != 33)
            return 1;

        secp256k1_sha256_write(&sha256_m, &pk[(index + k*nCols)*33], 33); /* pk_ind[col] */
        secp256k1_sha256_write(&sha256_m, tmp, 33); /* G * alpha[col] */

        if (0 != hash_to_curve(&ge1, &pk[(index + k*nCols)*33], 33)) /* H(pk_ind[col]) */
            return 1;

        secp256k1_gej_set_ge(&gej1, &ge1);
        secp256k1_ecmult(&ctx->ecmult_ctx, &gej2, &gej1, &alpha[k], &zero);  /* gej2 = H(pk_ind[col]) * alpha[col] */

        secp256k1_ge_set_gej(&ge1, &gej2);
        secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
        secp256k1_sha256_write(&sha256_m, tmp, 33); /* H(pk_ind[col]) * alpha[col] */

        secp256k1_scalar_set_b32(&s, sk[k], &overflow);
        if (overflow || secp256k1_scalar_is_zero(&s))
            return 1;
        secp256k1_ecmult(&ctx->ecmult_ctx, &gej2, &gej1, &s, &zero);  /* gej2 = H(pk_ind[col]) * sk_ind[col] */
        secp256k1_ge_set_gej(&ge1, &gej2);
        secp256k1_eckey_pubkey_serialize(&ge1, &ki[k * 33], &clen, 1);
    };

    for (k = dsRows; k < nRows; ++k)
    {
        do {
            secp256k1_rfc6979_hmac_sha256_generate(&rng, tmp, 32);
            secp256k1_scalar_set_b32(&alpha[k], tmp, &overflow);
        } while (overflow || secp256k1_scalar_is_zero(&alpha[k]));

        if (!secp256k1_ec_pubkey_create(ctx, &pubkey, tmp))  /* G * alpha[col] */
            return 1;
        clen = 33; /* must be set */
        if (!secp256k1_ec_pubkey_serialize(ctx, tmp, &clen, &pubkey, SECP256K1_EC_COMPRESSED)
            || clen != 33)
            return 1;

        secp256k1_sha256_write(&sha256_m, &pk[(index + k*nCols)*33], 33); /* pk_ind[col] */
        secp256k1_sha256_write(&sha256_m, tmp, 33); /* G * alpha[col] */
    };

    secp256k1_sha256_finalize(&sha256_m, tmp);
    secp256k1_scalar_set_b32(&clast, tmp, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&clast))
        return 1;

    i = (index + 1) % nCols;

    if (i == 0)
        memcpy(pc, tmp, 32); /* *pc = clast */

    while (i != index)
    {
        sha256_m = sha256_pre; /* set to after preimage hashed */

        for (k = 0; k < dsRows; ++k)
        {
            do {
                secp256k1_rfc6979_hmac_sha256_generate(&rng, tmp, 32);
                secp256k1_scalar_set_b32(&ss, tmp, &overflow);
            } while (overflow || secp256k1_scalar_is_zero(&ss));

            memcpy(&ps[(i + k*nCols)*32], tmp, 32);

            if (!secp256k1_eckey_pubkey_parse(&ge1, &pk[(i + k*nCols)*33], 33))
                return 1;
            secp256k1_gej_set_ge(&gej1, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &L, &gej1, &clast, &ss); /* L = G * ss + pk[k][i] * clast */


            /* R = H(pk[k][i]) * ss + ki[k] * clast */
            if (0 != hash_to_curve(&ge1, &pk[(i + k*nCols)*33], 33)) /* H(pk[k][i]) */
                return 1;
            secp256k1_gej_set_ge(&gej1, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &gej1, &gej1, &ss, &zero); /* gej1 = H(pk[k][i]) * ss */

            if (!secp256k1_eckey_pubkey_parse(&ge1, &ki[k * 33], 33))
                return 1;
            secp256k1_gej_set_ge(&gej2, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &gej2, &gej2, &clast, &zero); /* gej2 = ki[k] * clast */

            secp256k1_gej_add_var(&R, &gej1, &gej2, NULL);  /* R =  gej1 + gej2 */

            secp256k1_sha256_write(&sha256_m, &pk[(i + k*nCols)*33], 33); /* pk[k][i] */
            secp256k1_ge_set_gej(&ge1, &L);
            secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
            secp256k1_sha256_write(&sha256_m, tmp, 33); /* L */
            secp256k1_ge_set_gej(&ge1, &R);
            secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
            secp256k1_sha256_write(&sha256_m, tmp, 33); /* R */
        };

        for (k = dsRows; k < nRows; ++k)
        {
            do {
                secp256k1_rfc6979_hmac_sha256_generate(&rng, tmp, 32);
                secp256k1_scalar_set_b32(&ss, tmp, &overflow);
            } while (overflow || secp256k1_scalar_is_zero(&ss));

            memcpy(&ps[(i + k*nCols)*32], tmp, 32);

            /* L = G * ss + pk[k][i] * clast */
            if (!secp256k1_eckey_pubkey_parse(&ge1, &pk[(i + k*nCols)*33], 33))
                return 1;
            secp256k1_gej_set_ge(&gej1, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &L, &gej1, &clast, &ss);

            secp256k1_sha256_write(&sha256_m, &pk[(i + k*nCols)*33], 33); /* pk[k][i] */
            secp256k1_ge_set_gej(&ge1, &L);
            secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
            secp256k1_sha256_write(&sha256_m, tmp, 33); /* L */
        };

        secp256k1_sha256_finalize(&sha256_m, tmp);
        secp256k1_scalar_set_b32(&clast, tmp, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&clast))
            return 1;

        i = (i + 1) % nCols;

        if (i == 0)
            memcpy(pc, tmp, 32); /* *pc = clast */
    };


    for (k = 0; k < nRows; ++k)
    {
        /* ss[k][index] = alpha[k] - clast * sk[k] */

        secp256k1_scalar_set_b32(&ss, sk[k], &overflow);
        if (overflow || secp256k1_scalar_is_zero(&ss))
            return 1;

        secp256k1_scalar_mul(&s, &clast, &ss);

        secp256k1_scalar_negate(&s, &s);
        secp256k1_scalar_add(&ss, &alpha[k], &s);

        secp256k1_scalar_get_b32(&ps[(index + k*nCols)*32], &ss);
    };


    secp256k1_rfc6979_hmac_sha256_finalize(&rng);

    return 0;
}

int secp256k1_verify_mlsag(const secp256k1_context *ctx,
    const uint8_t *preimage, size_t nCols, size_t nRows,
    const uint8_t *pk, const uint8_t *ki, const uint8_t *pc, const uint8_t *ps)
{
    secp256k1_sha256_t sha256_m, sha256_pre;
    secp256k1_scalar zero, clast, cSig, ss;
    secp256k1_ge ge1;
    secp256k1_gej gej1, gej2, L, R;
    size_t dsRows = nRows-1; /* TODO: pass in dsRows explicitly? */
    uint8_t tmp[33];
    size_t i, k, clen;
    int overflow;

    secp256k1_scalar_set_int(&zero, 0);

    secp256k1_scalar_set_b32(&clast, pc, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&clast))
        return 1;

    cSig = clast;

    secp256k1_sha256_initialize(&sha256_m);
    secp256k1_sha256_write(&sha256_m, preimage, 32);
    sha256_pre = sha256_m;

    for (i = 0; i < nCols; ++i)
    {
        sha256_m = sha256_pre; /* set to after preimage hashed */

        for (k = 0; k < dsRows; ++k)
        {
            /* L = G * ss + pk[k][i] * clast */
            secp256k1_scalar_set_b32(&ss, &ps[(i + k*nCols)*32], &overflow);
            if (overflow || secp256k1_scalar_is_zero(&ss))
                return 1;
            if (!secp256k1_eckey_pubkey_parse(&ge1, &pk[(i + k*nCols)*33], 33))
                return 1;
            secp256k1_gej_set_ge(&gej1, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &L, &gej1, &clast, &ss);

            /* R = H(pk[k][i]) * ss + ki[k] * clast */
            if (0 != hash_to_curve(&ge1, &pk[(i + k*nCols)*33], 33)) /* H(pk[k][i]) */
                return 1;
            secp256k1_gej_set_ge(&gej1, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &gej1, &gej1, &ss, &zero); /* gej1 = H(pk[k][i]) * ss */

            if (!secp256k1_eckey_pubkey_parse(&ge1, &ki[k * 33], 33))
                return 1;
            secp256k1_gej_set_ge(&gej2, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &gej2, &gej2, &clast, &zero); /* gej2 = ki[k] * clast */

            secp256k1_gej_add_var(&R, &gej1, &gej2, NULL);  /* R =  gej1 + gej2 */

            secp256k1_sha256_write(&sha256_m, &pk[(i + k*nCols)*33], 33); /* pk[k][i] */
            secp256k1_ge_set_gej(&ge1, &L);
            secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
            secp256k1_sha256_write(&sha256_m, tmp, 33); /* L */
            secp256k1_ge_set_gej(&ge1, &R);
            secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
            secp256k1_sha256_write(&sha256_m, tmp, 33); /* R */
        };

        for (k = dsRows; k < nRows; ++k)
        {
            /* L = G * ss + pk[k][i] * clast */
            secp256k1_scalar_set_b32(&ss, &ps[(i + k*nCols)*32], &overflow);
            if (overflow || secp256k1_scalar_is_zero(&ss))
                return 1;

            if (!secp256k1_eckey_pubkey_parse(&ge1, &pk[(i + k*nCols)*33], 33))
                return 1;

            secp256k1_gej_set_ge(&gej1, &ge1);
            secp256k1_ecmult(&ctx->ecmult_ctx, &L, &gej1, &clast, &ss);

            secp256k1_sha256_write(&sha256_m, &pk[(i + k*nCols)*33], 33); /* pk[k][i] */
            secp256k1_ge_set_gej(&ge1, &L);
            secp256k1_eckey_pubkey_serialize(&ge1, tmp, &clen, 1);
            secp256k1_sha256_write(&sha256_m, tmp, 33); /* L */
        };

        secp256k1_sha256_finalize(&sha256_m, tmp);
        secp256k1_scalar_set_b32(&clast, tmp, &overflow);
        if (overflow || secp256k1_scalar_is_zero(&clast))
            return 1;
    };

    secp256k1_scalar_negate(&cSig, &cSig);
    secp256k1_scalar_add(&zero, &clast, &cSig);

    return secp256k1_scalar_is_zero(&zero) ? 0 : 2; /* return 0 on success, 2 on failure */
}

#endif
