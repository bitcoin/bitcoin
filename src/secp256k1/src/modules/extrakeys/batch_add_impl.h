#ifndef SECP256K1_MODULE_EXTRAKEYS_BATCH_ADD_IMPL_H
#define SECP256K1_MODULE_EXTRAKEYS_BATCH_ADD_IMPL_H

#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_tweak_check_batch.h"
#include "../batch/main_impl.h"

/* The number of scalar-point pairs allocated on the scratch space
 * by `secp256k1_batch_add_xonlypub_tweak_check` */
#define BATCH_TWEAK_CHECK_SCRATCH_OBJS 1

/** Computes a 16-byte deterministic randomizer by
 *  SHA256(batch_add_tag || tweaked pubkey || parity || tweak || internal pubkey) */
static void secp256k1_batch_xonlypub_tweak_randomizer_gen(unsigned char *randomizer32, secp256k1_sha256 *sha256, const unsigned char *tweaked_pubkey32, const unsigned char *tweaked_pk_parity, const unsigned char *internal_pk33, const unsigned char *tweak32) {
    secp256k1_sha256 sha256_cpy;
    unsigned char batch_add_type = (unsigned char) tweak_check;

    secp256k1_sha256_write(sha256, &batch_add_type, sizeof(batch_add_type));
    /* add tweaked pubkey check data to sha object */
    secp256k1_sha256_write(sha256, tweaked_pubkey32, 32);
    secp256k1_sha256_write(sha256, tweaked_pk_parity, 1);
    secp256k1_sha256_write(sha256, tweak32, 32);
    secp256k1_sha256_write(sha256, internal_pk33, 33);

    /* generate randomizer */
    sha256_cpy = *sha256;
    secp256k1_sha256_finalize(&sha256_cpy, randomizer32);
    /* 16 byte randomizer is sufficient */
    memset(randomizer32, 0, 16);
}

static int secp256k1_batch_xonlypub_tweak_randomizer_set(const secp256k1_context* ctx, secp256k1_batch *batch, secp256k1_scalar *r, const unsigned char *tweaked_pubkey32, int tweaked_pk_parity, const secp256k1_xonly_pubkey *internal_pubkey,const unsigned char *tweak32) {
    unsigned char randomizer[32];
    unsigned char internal_buf[33];
    size_t internal_buflen = sizeof(internal_buf);
    unsigned char parity = (unsigned char) tweaked_pk_parity;
    int overflow;
    /* t = 2^127 */
    secp256k1_scalar t = SECP256K1_SCALAR_CONST(0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000);

    /* We use compressed serialization here. If we would use
    * xonly_pubkey serialization and a user would wrongly memcpy
    * normal secp256k1_pubkeys into xonly_pubkeys then the randomizer
    * would be the same for two different pubkeys. */
    if (!secp256k1_ec_pubkey_serialize(ctx, internal_buf, &internal_buflen, (const secp256k1_pubkey *) internal_pubkey, SECP256K1_EC_COMPRESSED)) {
        return 0;
    }

    secp256k1_batch_xonlypub_tweak_randomizer_gen(randomizer, &batch->sha256, tweaked_pubkey32, &parity, internal_buf, tweak32);
    secp256k1_scalar_set_b32(r, randomizer, &overflow);
    /* Shift scalar to range [-2^127, 2^127-1] */
    secp256k1_scalar_negate(&t, &t);
    secp256k1_scalar_add(r, r, &t);
    VERIFY_CHECK(overflow == 0);

    return 1;
}

/** Adds the given x-only tweaked public key check to the batch.
 *
 *  Updates the batch object by:
 *     1. adding the point P-Q to the scratch space
 *          -> the point is of type `secp256k1_gej`
 *     2. adding the scalar ai to the scratch space
 *          -> ai is the scalar coefficient of P-Q (in multi multiplication)
 *     3. incrementing sc_g (scalar of G) by ai.tweak
 *
 *  Conventions used above:
 *     -> Q (tweaked pubkey)   = EC point where parity(y) = tweaked_pk_parity
 *                               and x = tweaked_pubkey32
 *     -> P (internal pubkey)  = internal pubkey
 *     -> ai (randomizer)      = sha256_tagged(batch_add_tag || tweaked_pubkey32  ||
 *                                             tweaked_pk_parity || tweak32 || pubkey)
 *     -> tweak (challenge)    = tweak32
 *
 * This function is based on `secp256k1_xonly_pubkey_tweak_add_check`.
 */
int secp256k1_batch_add_xonlypub_tweak_check(const secp256k1_context* ctx, secp256k1_batch *batch, const unsigned char *tweaked_pubkey32, int tweaked_pk_parity, const secp256k1_xonly_pubkey *internal_pubkey,const unsigned char *tweak32) {
    secp256k1_scalar tweak;
    secp256k1_scalar ai;
    secp256k1_ge pk;
    secp256k1_ge q;
    secp256k1_gej tmpj;
    secp256k1_fe qx;
    int overflow;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(batch != NULL);
    ARG_CHECK(internal_pubkey != NULL);
    ARG_CHECK(tweaked_pubkey32 != NULL);
    ARG_CHECK(tweak32 != NULL);

    if(batch->result == 0) {
        return 0;
    }

    if (!secp256k1_fe_set_b32_limit(&qx, tweaked_pubkey32)) {
        return 0;
    }

    secp256k1_scalar_set_b32(&tweak, tweak32, &overflow);
    if (overflow) {
        return 0;
    }

    if (!secp256k1_xonly_pubkey_load(ctx, &pk, internal_pubkey)) {
        return 0;
    }

    /* if insufficient space in batch, verify the inputs (stored in curr batch) and
     * save the result. This extends the batch capacity since `secp256k1_batch_verify`
     * clears the batch after verification. */
    if (batch->capacity - batch->len < BATCH_TWEAK_CHECK_SCRATCH_OBJS) {
        secp256k1_batch_verify(ctx, batch);
    }

    i = batch->len;
    /* append point P-Q to the scratch space */
    if (!secp256k1_ge_set_xo_var(&q, &qx, tweaked_pk_parity)) {
        return 0;
    }
    if (!secp256k1_ge_is_in_correct_subgroup(&q)) {
        return 0;
    }
    secp256k1_ge_neg(&q, &q);
    secp256k1_gej_set_ge(&tmpj, &q);
    secp256k1_gej_add_ge_var(&tmpj, &tmpj, &pk, NULL);
    batch->points[i] = tmpj;

    /* Compute ai (randomizer) */
    if (batch->len == 0) {
        /* set randomizer as 1 for the first term in batch */
        ai = secp256k1_scalar_one;
    } else if(!secp256k1_batch_xonlypub_tweak_randomizer_set(ctx, batch, &ai, tweaked_pubkey32, tweaked_pk_parity, internal_pubkey, tweak32)) {
        return 0;
    }

    /* append scalar ai to scratch space */
    batch->scalars[i] = ai;

    /* increment scalar of G by ai.tweak */
    secp256k1_scalar_mul(&tweak, &tweak, &ai);
    secp256k1_scalar_add(&batch->sc_g, &batch->sc_g, &tweak);

    batch->len += 1;

    return 1;
}

#endif /* SECP256K1_MODULE_EXTRAKEYS_BATCH_ADD_IMPL_H */
