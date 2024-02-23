/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_MAIN_H
#define SECP256K1_MODULE_SILENTPAYMENTS_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_ecdh.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_silentpayments.h"
#include "../../hash.h"

/** Set hash state to the BIP340 tagged hash midstate for "BIP0352/Inputs". */
static void secp256k1_silentpayments_sha256_init_inputs(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0xd4143ffcul;
    hash->s[1] = 0x012ea4b5ul;
    hash->s[2] = 0x36e21c8ful;
    hash->s[3] = 0xf7ec7b54ul;
    hash->s[4] = 0x4dd4e2acul;
    hash->s[5] = 0x9bcaa0a4ul;
    hash->s[6] = 0xe244899bul;
    hash->s[7] = 0xcd06903eul;

    hash->bytes = 64;
}

static void secp256k1_silentpayments_calculate_input_hash(unsigned char *input_hash, const unsigned char *outpoint_smallest36, secp256k1_ge *pubkey_sum) {
    secp256k1_sha256 hash;
    unsigned char pubkey_sum_ser[33];
    size_t ser_size;
    int ser_ret;

    secp256k1_silentpayments_sha256_init_inputs(&hash);
    secp256k1_sha256_write(&hash, outpoint_smallest36, 36);
    ser_ret = secp256k1_eckey_pubkey_serialize(pubkey_sum, pubkey_sum_ser, &ser_size, 1);
    VERIFY_CHECK(ser_ret && ser_size == sizeof(pubkey_sum_ser));
    (void)ser_ret;
    secp256k1_sha256_write(&hash, pubkey_sum_ser, sizeof(pubkey_sum_ser));
    secp256k1_sha256_finalize(&hash, input_hash);
}

int secp256k1_silentpayments_create_private_tweak_data(const secp256k1_context *ctx, unsigned char *a_sum, unsigned char *input_hash, const unsigned char * const *plain_seckeys, size_t n_plain_seckeys, const unsigned char * const *taproot_seckeys, size_t n_taproot_seckeys, const unsigned char *outpoint_smallest36) {
    size_t i;
    secp256k1_scalar a_sum_scalar, addend;
    secp256k1_ge A_sum_ge;
    secp256k1_gej A_sum_gej;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(a_sum != NULL);
    memset(a_sum, 0, 32);
    ARG_CHECK(input_hash != NULL);
    memset(input_hash, 0, 32);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(plain_seckeys == NULL || n_plain_seckeys >= 1);
    ARG_CHECK(taproot_seckeys == NULL || n_taproot_seckeys >= 1);
    ARG_CHECK((plain_seckeys != NULL) || (taproot_seckeys != NULL));
    ARG_CHECK((n_plain_seckeys + n_taproot_seckeys) >= 1);
    ARG_CHECK(outpoint_smallest36 != NULL);

    /* Compute input private keys sum: a_sum = a_1 + a_2 + ... + a_n */
    a_sum_scalar = secp256k1_scalar_zero;
    for (i = 0; i < n_plain_seckeys; i++) {
        int ret = secp256k1_scalar_set_b32_seckey(&addend, plain_seckeys[i]);
        VERIFY_CHECK(ret);
        (void)ret;

        secp256k1_scalar_add(&a_sum_scalar, &a_sum_scalar, &addend);
        VERIFY_CHECK(!secp256k1_scalar_is_zero(&a_sum_scalar));
    }
    /* private keys used for taproot outputs have to be negated if they resulted in an odd point */
    for (i = 0; i < n_taproot_seckeys; i++) {
        secp256k1_ge addend_point;
        int ret = secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &addend, &addend_point, taproot_seckeys[i]);
        VERIFY_CHECK(ret);
        (void)ret;
        /* declassify addend_point to allow using it as a branch point (this is fine because addend_point is not a secret) */
        secp256k1_declassify(ctx, &addend_point, sizeof(addend_point));
        secp256k1_fe_normalize_var(&addend_point.y);
        if (secp256k1_fe_is_odd(&addend_point.y)) {
            secp256k1_scalar_negate(&addend, &addend);
        }

        secp256k1_scalar_add(&a_sum_scalar, &a_sum_scalar, &addend);
        VERIFY_CHECK(!secp256k1_scalar_is_zero(&a_sum_scalar));
    }
    if (secp256k1_scalar_is_zero(&a_sum_scalar)) {
        /* TODO: do we need a special error return code for this case? */
        return 0;
    }
    secp256k1_scalar_get_b32(a_sum, &a_sum_scalar);

    /* Compute input_hash = hash(outpoint_L || (a_sum * G)) */
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &A_sum_gej, &a_sum_scalar);
    secp256k1_ge_set_gej(&A_sum_ge, &A_sum_gej);
    secp256k1_silentpayments_calculate_input_hash(input_hash, outpoint_smallest36, &A_sum_ge);

    return 1;
}

int secp256k1_silentpayments_create_public_tweak_data(const secp256k1_context *ctx, secp256k1_pubkey *A_sum, unsigned char *input_hash, const secp256k1_pubkey * const *plain_pubkeys, size_t n_plain_pubkeys, const secp256k1_xonly_pubkey * const *xonly_pubkeys, size_t n_xonly_pubkeys, const unsigned char *outpoint_smallest36) {
    size_t i;
    secp256k1_ge A_sum_ge, addend;
    secp256k1_gej A_sum_gej;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(A_sum != NULL);
    ARG_CHECK(input_hash != NULL);
    memset(input_hash, 0, 32);
    ARG_CHECK(plain_pubkeys == NULL || n_plain_pubkeys >= 1);
    ARG_CHECK(xonly_pubkeys == NULL || n_xonly_pubkeys >= 1);
    ARG_CHECK((plain_pubkeys != NULL) || (xonly_pubkeys != NULL));
    ARG_CHECK((n_plain_pubkeys + n_xonly_pubkeys) >= 1);
    ARG_CHECK(outpoint_smallest36 != NULL);

    /* Compute input public keys sum: A_sum = A_1 + A_2 + ... + A_n */
    secp256k1_gej_set_infinity(&A_sum_gej);
    for (i = 0; i < n_plain_pubkeys; i++) {
        secp256k1_pubkey_load(ctx, &addend, plain_pubkeys[i]);
        secp256k1_gej_add_ge(&A_sum_gej, &A_sum_gej, &addend);
    }
    for (i = 0; i < n_xonly_pubkeys; i++) {
        secp256k1_xonly_pubkey_load(ctx, &addend, xonly_pubkeys[i]);
        secp256k1_gej_add_ge(&A_sum_gej, &A_sum_gej, &addend);
    }
    if (secp256k1_gej_is_infinity(&A_sum_gej)) {
        /* TODO: do we need a special error return code for this case? */
        return 0;
    }
    secp256k1_ge_set_gej(&A_sum_ge, &A_sum_gej);
    secp256k1_pubkey_save(A_sum, &A_sum_ge);

    /* Compute input_hash = hash(outpoint_L || A_sum) */
    secp256k1_silentpayments_calculate_input_hash(input_hash, outpoint_smallest36, &A_sum_ge);

    return 1;
}

int secp256k1_silentpayments_create_tweaked_pubkey(const secp256k1_context *ctx, secp256k1_pubkey *A_tweaked, const secp256k1_pubkey *A_sum, const unsigned char *input_hash) {
    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(A_tweaked != NULL);
    ARG_CHECK(A_sum != NULL);
    ARG_CHECK(input_hash != NULL);

    /* Calculate A_tweaked = input_hash * A_sum */
    *A_tweaked = *A_sum;
    if (!secp256k1_ec_pubkey_tweak_mul(ctx, A_tweaked, input_hash)) {
        return 0;
    }

    return 1;
}

/* secp256k1_ecdh expects a hash function to be passed in or uses its default
 * hashing function. We don't want to hash the ECDH result, so we define a
 * custom function which simply returns the pubkey without hashing.
 */
static int secp256k1_silentpayments_ecdh_return_pubkey(unsigned char *output, const unsigned char *x32, const unsigned char *y32, void *data) {
    secp256k1_ge point;
    secp256k1_fe x, y;
    size_t ser_size;
    int ser_ret;

    (void)data;
    /* Parse point as group element */
    if (!secp256k1_fe_set_b32_limit(&x, x32) || !secp256k1_fe_set_b32_limit(&y, y32)) {
        return 0;
    }
    secp256k1_ge_set_xy(&point, &x, &y);

    /* Serialize as compressed pubkey */
    ser_ret = secp256k1_eckey_pubkey_serialize(&point, output, &ser_size, 1);
    VERIFY_CHECK(ser_ret && ser_size == 33);
    (void)ser_ret;

    return 1;
}

int secp256k1_silentpayments_create_shared_secret(const secp256k1_context *ctx, unsigned char *shared_secret33, const secp256k1_pubkey *public_component, const unsigned char *secret_component, const unsigned char *input_hash) {
    unsigned char tweaked_secret_component[32];

    /* Sanity check inputs */
    ARG_CHECK(shared_secret33 != NULL);
    memset(shared_secret33, 0, 33);
    ARG_CHECK(public_component != NULL);
    ARG_CHECK(secret_component != NULL);

    /* Tweak secret component with input hash, if available */
    memcpy(tweaked_secret_component, secret_component, 32);
    if (input_hash != NULL) {
        if (!secp256k1_ec_seckey_tweak_mul(ctx, tweaked_secret_component, input_hash)) {
            return 0;
        }
    }

    /* Compute shared_secret = tweaked_secret_component * Public_component */
    if (!secp256k1_ecdh(ctx, shared_secret33, public_component, tweaked_secret_component, secp256k1_silentpayments_ecdh_return_pubkey, NULL)) {
        return 0;
    }

    return 1;
}

/** Set hash state to the BIP340 tagged hash midstate for "BIP0352/Label". */
static void secp256k1_silentpayments_sha256_init_label(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0x26b95d63ul;
    hash->s[1] = 0x8bf1b740ul;
    hash->s[2] = 0x10a5986ful;
    hash->s[3] = 0x06a387a5ul;
    hash->s[4] = 0x2d1c1c30ul;
    hash->s[5] = 0xd035951aul;
    hash->s[6] = 0x2d7f0f96ul;
    hash->s[7] = 0x29e3e0dbul;

    hash->bytes = 64;
}

int secp256k1_silentpayments_create_label_tweak(const secp256k1_context *ctx, secp256k1_pubkey *label, unsigned char *label_tweak32, const unsigned char *receiver_scan_seckey, unsigned int m) {
    secp256k1_sha256 hash;
    unsigned char m_serialized[4];

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    (void)ctx;
    VERIFY_CHECK(label != NULL);
    VERIFY_CHECK(label_tweak32 != NULL);
    VERIFY_CHECK(receiver_scan_seckey != NULL);

    /* Compute label_tweak = hash(ser_256(b_scan) || ser_32(m))  [sha256 with tag "BIP0352/Label"] */
    secp256k1_silentpayments_sha256_init_label(&hash);
    secp256k1_sha256_write(&hash, receiver_scan_seckey, 32);
    secp256k1_write_be32(m_serialized, m);
    secp256k1_sha256_write(&hash, m_serialized, sizeof(m_serialized));
    secp256k1_sha256_finalize(&hash, label_tweak32);

    /* Compute label = label_tweak * G */
    if (!secp256k1_ec_pubkey_create(ctx, label, label_tweak32)) {
        return 0;
    }

    return 1;
}

int secp256k1_silentpayments_create_address_spend_pubkey(const secp256k1_context *ctx, unsigned char *l_addr_spend_pubkey33, const secp256k1_pubkey *receiver_spend_pubkey, const secp256k1_pubkey *label) {
    secp256k1_ge B_m, label_addend;
    secp256k1_gej result_gej;
    secp256k1_ge result_ge;
    size_t ser_size;
    int ser_ret;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    VERIFY_CHECK(l_addr_spend_pubkey33 != NULL);
    VERIFY_CHECK(receiver_spend_pubkey != NULL);
    VERIFY_CHECK(label != NULL);

    /* Calculate B_m = B_spend + label */
    secp256k1_pubkey_load(ctx, &B_m, receiver_spend_pubkey);
    secp256k1_pubkey_load(ctx, &label_addend, label);
    secp256k1_gej_set_ge(&result_gej, &B_m);
    secp256k1_gej_add_ge_var(&result_gej, &result_gej, &label_addend, NULL);

    /* Serialize B_m */
    secp256k1_ge_set_gej(&result_ge, &result_gej);
    ser_ret = secp256k1_eckey_pubkey_serialize(&result_ge, l_addr_spend_pubkey33, &ser_size, 1);
    VERIFY_CHECK(ser_ret && ser_size == 33);
    (void)ser_ret;

    return 1;
}

/** Set hash state to the BIP340 tagged hash midstate for "BIP0352/SharedSecret". */
static void secp256k1_silentpayments_sha256_init_sharedsecret(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0x88831537ul;
    hash->s[1] = 0x5127079bul;
    hash->s[2] = 0x69c2137bul;
    hash->s[3] = 0xab0303e6ul;
    hash->s[4] = 0x98fa21faul;
    hash->s[5] = 0x4a888523ul;
    hash->s[6] = 0xbd99daabul;
    hash->s[7] = 0xf25e5e0aul;

    hash->bytes = 64;
}

static void secp256k1_silentpayments_create_t_k(secp256k1_scalar *t_k_scalar, const unsigned char *shared_secret33, unsigned int k) {
    secp256k1_sha256 hash;
    unsigned char hash_ser[32];
    unsigned char k_serialized[4];

    /* Compute t_k = hash(shared_secret || ser_32(k))  [sha256 with tag "BIP0352/SharedSecret"] */
    secp256k1_silentpayments_sha256_init_sharedsecret(&hash);
    secp256k1_sha256_write(&hash, shared_secret33, 33);
    secp256k1_write_be32(k_serialized, k);
    secp256k1_sha256_write(&hash, k_serialized, sizeof(k_serialized));
    secp256k1_sha256_finalize(&hash, hash_ser);
    secp256k1_scalar_set_b32(t_k_scalar, hash_ser, NULL);
}

int secp256k1_silentpayments_sender_create_output_pubkey(const secp256k1_context *ctx, secp256k1_xonly_pubkey *P_output_xonly, const unsigned char *shared_secret33, const secp256k1_pubkey *receiver_spend_pubkey, unsigned int k) {
    secp256k1_ge P_output_ge;
    secp256k1_scalar t_k_scalar;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(P_output_xonly != NULL);
    ARG_CHECK(shared_secret33 != NULL);
    ARG_CHECK(receiver_spend_pubkey != NULL);

    /* Calculate and return P_output_xonly = B_spend + t_k * G */
    secp256k1_silentpayments_create_t_k(&t_k_scalar, shared_secret33, k);
    secp256k1_pubkey_load(ctx, &P_output_ge, receiver_spend_pubkey);
    if (!secp256k1_eckey_pubkey_tweak_add(&P_output_ge, &t_k_scalar)) {
        return 0;
    }
    secp256k1_xonly_pubkey_save(P_output_xonly, &P_output_ge);

    return 1;
}

int secp256k1_silentpayments_receiver_scan_output(const secp256k1_context *ctx, int *direct_match, unsigned char *t_k, secp256k1_silentpayments_label_data *label_data, const unsigned char *shared_secret33, const secp256k1_pubkey *receiver_spend_pubkey, unsigned int k, const secp256k1_xonly_pubkey *tx_output) {
    secp256k1_scalar t_k_scalar;
    secp256k1_ge P_output_ge;
    secp256k1_xonly_pubkey P_output_xonly;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(direct_match != NULL);
    ARG_CHECK(t_k != NULL);
    ARG_CHECK(shared_secret33 != NULL);
    ARG_CHECK(receiver_spend_pubkey != NULL);
    ARG_CHECK(tx_output != NULL);

    /* Calculate t_k = hash(shared_secret || ser_32(k)) */
    secp256k1_silentpayments_create_t_k(&t_k_scalar, shared_secret33, k);
    secp256k1_scalar_get_b32(t_k, &t_k_scalar);

    /* Calculate P_output = B_spend + t_k * G */
    secp256k1_pubkey_load(ctx, &P_output_ge, receiver_spend_pubkey);
    if (!secp256k1_eckey_pubkey_tweak_add(&P_output_ge, &t_k_scalar)) {
        return 0;
    }

    /* If the calculated output matches the one from the tx, we have a direct match and can
     * return without labels calculation (one of the two would result in point of infinity) */
    secp256k1_xonly_pubkey_save(&P_output_xonly, &P_output_ge);
    if (secp256k1_xonly_pubkey_cmp(ctx, &P_output_xonly, tx_output) == 0) {
        *direct_match = 1;
        return 1;
    }
    *direct_match = 0;

    /* If desired, also calculate label candidates */
    if (label_data != NULL) {
        secp256k1_ge P_output_negated_ge, tx_output_ge;
        secp256k1_ge label_ge;
        secp256k1_gej label_gej;

        /* Calculate negated P_output (common addend) first */
        secp256k1_ge_neg(&P_output_negated_ge, &P_output_ge);

        /* Calculate first scan label candidate: label1 = tx_output - P_output */
        secp256k1_xonly_pubkey_load(ctx, &tx_output_ge, tx_output);
        secp256k1_gej_set_ge(&label_gej, &tx_output_ge);
        secp256k1_gej_add_ge_var(&label_gej, &label_gej, &P_output_negated_ge, NULL);
        secp256k1_ge_set_gej(&label_ge, &label_gej);
        secp256k1_pubkey_save(&label_data->label, &label_ge);

        /* Calculate second scan label candidate: label2 = -tx_output - P_output */
        secp256k1_gej_set_ge(&label_gej, &tx_output_ge);
        secp256k1_gej_neg(&label_gej, &label_gej);
        secp256k1_gej_add_ge_var(&label_gej, &label_gej, &P_output_negated_ge, NULL);
        secp256k1_ge_set_gej(&label_ge, &label_gej);
        secp256k1_pubkey_save(&label_data->label_negated, &label_ge);
    }

    return 1;
}

int secp256k1_silentpayments_create_output_seckey(const secp256k1_context *ctx, unsigned char *output_seckey, const unsigned char *receiver_spend_seckey, const unsigned char *t_k, const unsigned char *label_tweak) {
    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output_seckey != NULL);
    memset(output_seckey, 0, 32);
    ARG_CHECK(receiver_spend_seckey != NULL);
    ARG_CHECK(t_k != NULL);

    /* Compute d = (b_spend + t_k) mod n */
    memcpy(output_seckey, receiver_spend_seckey, 32);
    if (!secp256k1_ec_seckey_tweak_add(ctx, output_seckey, t_k)) {
        return 0;
    }

    if (label_tweak != NULL) {
        if (!secp256k1_ec_seckey_tweak_add(ctx, output_seckey, label_tweak)) {
            return 0;
        }
    }

    return 1;
}

#endif
