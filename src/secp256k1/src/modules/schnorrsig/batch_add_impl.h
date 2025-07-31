#ifndef SECP256K1_MODULE_SCHNORRSIG_BATCH_ADD_IMPL_H
#define SECP256K1_MODULE_SCHNORRSIG_BATCH_ADD_IMPL_H

#include "../../../include/secp256k1_schnorrsig.h"
#include "../../../include/secp256k1_schnorrsig_batch.h"
#include "../batch/main_impl.h"

/* The number of scalar-point pairs allocated on the scratch space
 * by `secp256k1_batch_add_schnorrsig` */
#define BATCH_SCHNORRSIG_SCRATCH_OBJS 2

/** Computes a 16-byte deterministic randomizer by
 *  SHA256(batch_add_tag || sig || msg || compressed pubkey) */
static void secp256k1_batch_schnorrsig_randomizer_gen(unsigned char *randomizer32, secp256k1_sha256 *sha256, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const unsigned char *compressed_pk33) {
    secp256k1_sha256 sha256_cpy;
    unsigned char batch_add_type = (unsigned char) schnorrsig;

    secp256k1_sha256_write(sha256, &batch_add_type, sizeof(batch_add_type));
    /* add schnorrsig data to sha256 object */
    secp256k1_sha256_write(sha256, sig64, 64);
    secp256k1_sha256_write(sha256, msg, msglen);
    secp256k1_sha256_write(sha256, compressed_pk33, 33);

    /* generate randomizer */
    sha256_cpy = *sha256;
    secp256k1_sha256_finalize(&sha256_cpy, randomizer32);
    /* 16 byte randomizer is sufficient */
    memset(randomizer32, 0, 16);
}

static int secp256k1_batch_schnorrsig_randomizer_set(const secp256k1_context *ctx, secp256k1_batch *batch, secp256k1_scalar *r, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const secp256k1_xonly_pubkey *pubkey) {
    unsigned char randomizer[32];
    unsigned char buf[33];
    size_t buflen = sizeof(buf);
    int overflow;
    /* t = 2^127 */
    secp256k1_scalar t = SECP256K1_SCALAR_CONST(0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000);

    /* We use compressed serialization here. If we would use
    * xonly_pubkey serialization and a user would wrongly memcpy
    * normal secp256k1_pubkeys into xonly_pubkeys then the randomizer
    * would be the same for two different pubkeys. */
    if (!secp256k1_ec_pubkey_serialize(ctx, buf, &buflen, (const secp256k1_pubkey *) pubkey, SECP256K1_EC_COMPRESSED)) {
        return 0;
    }

    secp256k1_batch_schnorrsig_randomizer_gen(randomizer, &batch->sha256, sig64, msg, msglen, buf);
    secp256k1_scalar_set_b32(r, randomizer, &overflow);
    /* Shift scalar to range [-2^127, 2^127-1] */
    secp256k1_scalar_negate(&t, &t);
    secp256k1_scalar_add(r, r, &t);
    VERIFY_CHECK(overflow == 0);

    return 1;
}

/** Adds the given schnorr signature to the batch.
 *
 *  Updates the batch object by:
 *     1. adding the points R and P to the scratch space
 *          -> both the points are of type `secp256k1_gej`
 *     2. adding the scalars ai and ai.e to the scratch space
 *          -> ai   is the scalar coefficient of R (in multi multiplication)
 *          -> ai.e is the scalar coefficient of P (in multi multiplication)
 *     3. incrementing sc_g (scalar of G) by -ai.s
 *
 *  Conventions used above:
 *     -> R (nonce commitment) = EC point whose y = even and x = sig64[0:32]
 *     -> P (public key)       = pubkey
 *     -> ai (randomizer)      = sha256_tagged(batch_add_tag || sig64 || msg || pubkey)
 *     -> e (challenge)        = sha256_tagged(sig64[0:32] || pk.x || msg)
 *     -> s                    = sig64[32:64]
 *
 * This function is based on `secp256k1_schnorrsig_verify`.
 */
int secp256k1_batch_add_schnorrsig(const secp256k1_context* ctx, secp256k1_batch *batch, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const secp256k1_xonly_pubkey *pubkey) {
    secp256k1_scalar s;
    secp256k1_scalar e;
    secp256k1_scalar ai;
    secp256k1_ge pk;
    secp256k1_fe rx;
    secp256k1_ge r;
    unsigned char buf[32];
    int overflow;
    size_t i;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(batch != NULL);
    ARG_CHECK(sig64 != NULL);
    ARG_CHECK(msg != NULL || msglen == 0);
    ARG_CHECK(pubkey != NULL);

    if (batch->result == 0) {
        return 0;
    }

    if (!secp256k1_fe_set_b32_limit(&rx, &sig64[0])) {
        return 0;
    }

    secp256k1_scalar_set_b32(&s, &sig64[32], &overflow);
    if (overflow) {
        return 0;
    }

    if (!secp256k1_xonly_pubkey_load(ctx, &pk, pubkey)) {
        return 0;
    }

    /* if insufficient space in batch, verify the inputs (stored in curr batch) and
     * save the result. This extends the batch capacity since `secp256k1_batch_verify`
     * clears the batch after verification. */
    if (batch->capacity - batch->len < BATCH_SCHNORRSIG_SCRATCH_OBJS) {
        secp256k1_batch_verify(ctx, batch);
    }

    i = batch->len;
    /* append point R to the scratch space */
    if (!secp256k1_ge_set_xo_var(&r, &rx, 0)) {
        return 0;
    }
    if (!secp256k1_ge_is_in_correct_subgroup(&r)) {
        return 0;
    }
    secp256k1_gej_set_ge(&batch->points[i], &r);

    /* append point P to the scratch space */
    secp256k1_gej_set_ge(&batch->points[i+1], &pk);

    /* compute e (challenge) */
    secp256k1_fe_get_b32(buf, &pk.x);
    secp256k1_schnorrsig_challenge(&e, &sig64[0], msg, msglen, buf);

    /* compute ai (randomizer) */
    if (batch->len == 0) {
        /* don't generate a randomizer for the first term in the batch to improve
         * the computation speed. hence, set the randomizer to 1. */
        ai = secp256k1_scalar_one;
    } else if (!secp256k1_batch_schnorrsig_randomizer_set(ctx, batch, &ai, sig64, msg, msglen, pubkey)) {
        return 0;
    }

    /* append scalars ai and ai.e to scratch space (order shouldn't change) */
    batch->scalars[i] = ai;
    secp256k1_scalar_mul(&e, &e, &ai);
    batch->scalars[i+1] = e;

    /* increment scalar of G by -ai.s */
    secp256k1_scalar_mul(&s, &s, &ai);
    secp256k1_scalar_negate(&s, &s);
    secp256k1_scalar_add(&batch->sc_g, &batch->sc_g, &s);

    batch->len += 2;

    return 1;
}

#endif /* SECP256K1_MODULE_SCHNORRSIG_BATCH_ADD_IMPL_H */
