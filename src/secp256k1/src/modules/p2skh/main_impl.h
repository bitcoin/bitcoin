/***********************************************************************
 * P2SKH (Pay-to-Schnorr-Key-Hash) scheme with hash160 key commitment. *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_P2SKH_MAIN_H
#define SECP256K1_MODULE_P2SKH_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_schnorrsig.h"
#include "../../../include/secp256k1_p2skh.h"
#include "../../hash.h"

/* Compute the challenge scalar:
 *   e = TaggedHash("P2SKH/challenge", rx32 || hash20 || msg32) mod n
 */
static void secp256k1_p2skh_challenge(secp256k1_scalar *e,
    const unsigned char *rx32,
    const unsigned char *hash20,
    const unsigned char *msg32)
{
    unsigned char buf[32];
    secp256k1_sha256 sha;
    static const unsigned char tag[] = "P2SKH/challenge";

    secp256k1_sha256_initialize_tagged(&sha, tag, sizeof(tag) - 1);
    secp256k1_sha256_write(&sha, rx32, 32);
    secp256k1_sha256_write(&sha, hash20, 20);
    secp256k1_sha256_write(&sha, msg32, 32);
    secp256k1_sha256_finalize(&sha, buf);
    secp256k1_scalar_set_b32(e, buf, NULL);
}

/* Nonce derivation:
 *   If aux_rand32 != NULL, mask the key: masked = key XOR SHA256("P2SKH/aux" || aux_rand32)
 *   k = TaggedHash("P2SKH/nonce", masked_key || pk_x32 || msg32)
 *
 * Using a different tag from BIP340 prevents nonce reuse across signing schemes.
 */
static void secp256k1_p2skh_nonce(unsigned char *nonce32,
    const unsigned char *msg32,
    const unsigned char *key32,
    const unsigned char *pk_x32,
    const unsigned char *aux_rand32)
{
    secp256k1_sha256 sha;
    unsigned char masked_key[32];
    static const unsigned char aux_tag[]   = "P2SKH/aux";
    static const unsigned char nonce_tag[] = "P2SKH/nonce";
    int i;

    if (aux_rand32 != NULL) {
        unsigned char aux_hash[32];
        secp256k1_sha256_initialize_tagged(&sha, aux_tag, sizeof(aux_tag) - 1);
        secp256k1_sha256_write(&sha, aux_rand32, 32);
        secp256k1_sha256_finalize(&sha, aux_hash);
        for (i = 0; i < 32; i++) {
            masked_key[i] = key32[i] ^ aux_hash[i];
        }
    } else {
        for (i = 0; i < 32; i++) {
            masked_key[i] = key32[i];
        }
    }

    secp256k1_sha256_initialize_tagged(&sha, nonce_tag, sizeof(nonce_tag) - 1);
    secp256k1_sha256_write(&sha, masked_key, 32);
    secp256k1_sha256_write(&sha, pk_x32, 32);
    secp256k1_sha256_write(&sha, msg32, 32);
    secp256k1_sha256_finalize(&sha, nonce32);

    secp256k1_memclear_explicit(masked_key, sizeof(masked_key));
}

int secp256k1_p2skh_sign(const secp256k1_context *ctx, unsigned char *sig64,
    const unsigned char *msg32, const secp256k1_keypair *keypair,
    const unsigned char *pubkey_hash20, const unsigned char *aux_rand32)
{
    secp256k1_scalar sk, e, k;
    secp256k1_gej rj;
    secp256k1_ge pk, r;
    unsigned char pk_x[32];
    unsigned char seckey[32];
    unsigned char nonce32[32];
    int ret = 1;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(keypair != NULL);
    ARG_CHECK(pubkey_hash20 != NULL);

    ret &= secp256k1_keypair_load(ctx, &sk, &pk, keypair);

    /* Even-y convention: if P.y is odd, negate sk so the effective pubkey has even y. */
    if (secp256k1_fe_is_odd(&pk.y)) {
        secp256k1_scalar_negate(&sk, &sk);
    }

    secp256k1_scalar_get_b32(seckey, &sk);
    secp256k1_fe_normalize_var(&pk.x);
    secp256k1_fe_get_b32(pk_x, &pk.x);

    /* Derive nonce k. */
    secp256k1_p2skh_nonce(nonce32, msg32, seckey, pk_x, aux_rand32);
    secp256k1_scalar_set_b32(&k, nonce32, NULL);
    ret &= !secp256k1_scalar_is_zero(&k);
    secp256k1_scalar_cmov(&k, &secp256k1_scalar_one, !ret);

    /* R = k*G. Even-y convention: if R.y is odd, negate k. */
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &rj, &k);
    secp256k1_ge_set_gej(&r, &rj);
    secp256k1_declassify(ctx, &r, sizeof(r));
    secp256k1_fe_normalize_var(&r.y);
    if (secp256k1_fe_is_odd(&r.y)) {
        secp256k1_scalar_negate(&k, &k);
    }
    secp256k1_fe_normalize_var(&r.x);
    secp256k1_fe_get_b32(&sig64[0], &r.x);

    /* e = TaggedHash("P2SKH/challenge", R.x || hash160(P.x) || msg32) */
    secp256k1_p2skh_challenge(&e, &sig64[0], pubkey_hash20, msg32);

    /* S = k + e * sk */
    secp256k1_scalar_mul(&e, &e, &sk);
    secp256k1_scalar_add(&e, &e, &k);
    secp256k1_scalar_get_b32(&sig64[32], &e);

    secp256k1_memczero(sig64, 64, !ret);
    secp256k1_scalar_clear(&k);
    secp256k1_scalar_clear(&sk);
    secp256k1_memclear_explicit(seckey, sizeof(seckey));
    secp256k1_memclear_explicit(nonce32, sizeof(nonce32));
    secp256k1_gej_clear(&rj);

    return ret;
}

int secp256k1_p2skh_verify(const secp256k1_context *ctx, unsigned char *out_px32,
    const unsigned char *sig64, const unsigned char *msg32,
    const unsigned char *pubkey_hash20)
{
    secp256k1_scalar s, e, e_inv, neg_e_inv, s_e_inv;
    secp256k1_fe rx;
    secp256k1_ge r, p;
    secp256k1_gej rj, pj;
    int overflow;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out_px32 != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(msg32 != NULL);
    ARG_CHECK(pubkey_hash20 != NULL);

    /* Parse R.x from sig64[0..31]; must be a valid field element. */
    if (!secp256k1_fe_set_b32_limit(&rx, &sig64[0])) {
        return 0;
    }

    /* Parse S from sig64[32..63]; must not overflow or be zero. */
    secp256k1_scalar_set_b32(&s, &sig64[32], &overflow);
    if (overflow || secp256k1_scalar_is_zero(&s)) {
        return 0;
    }

    /* Reconstruct R as the even-y point with x-coordinate rx. */
    if (!secp256k1_ge_set_xo_var(&r, &rx, 0 /* even y */)) {
        return 0;
    }

    /* e = TaggedHash("P2SKH/challenge", R.x || pubkey_hash20 || msg32) mod n */
    secp256k1_p2skh_challenge(&e, &sig64[0], pubkey_hash20, msg32);

    /* Guard: e must be nonzero (probability ~2^-128, so essentially never happens). */
    if (secp256k1_scalar_is_zero(&e)) {
        return 0;
    }

    /* e_inv = e^-1 mod n */
    secp256k1_scalar_inverse_var(&e_inv, &e);

    /* neg_e_inv = -e^-1,   s_e_inv = S * e^-1 */
    secp256k1_scalar_negate(&neg_e_inv, &e_inv);
    secp256k1_scalar_mul(&s_e_inv, &s, &e_inv);

    /* P = (-e^-1)*R + (S*e^-1)*G  =  e^-1 * (S*G - R)  =  P */
    secp256k1_gej_set_ge(&rj, &r);
    secp256k1_ecmult(&pj, &rj, &neg_e_inv, &s_e_inv);

    if (secp256k1_gej_is_infinity(&pj)) {
        return 0;
    }

    /* Extract P.x into out_px32; caller verifies hash160(out_px32) == pubkey_hash20. */
    secp256k1_ge_set_gej_var(&p, &pj);
    secp256k1_fe_normalize_var(&p.x);
    secp256k1_fe_get_b32(out_px32, &p.x);

    return 1;
}

#endif /* SECP256K1_MODULE_P2SKH_MAIN_H */
