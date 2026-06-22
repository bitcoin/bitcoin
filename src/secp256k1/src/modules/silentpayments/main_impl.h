/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SILENTPAYMENTS_MAIN_H
#define SECP256K1_MODULE_SILENTPAYMENTS_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_silentpayments.h"

#include "../../eckey.h"
#include "../../ecmult.h"
#include "../../ecmult_const.h"
#include "../../ecmult_gen.h"
#include "../../group.h"
#include "../../hash.h"
#include "../../hsort.h"

/** magic bytes for ensuring prevouts_summary objects were initialized correctly. */
static const unsigned char secp256k1_silentpayments_prevouts_summary_magic[4] = { 0xa7, 0x1c, 0xd3, 0x5e };

/** Sort an array of Silent Payments recipients. This is used to group recipients by scan pubkey to
 *  ensure the correct values of k are used when creating multiple outputs for a recipient.
 *  Since heap sort is unstable, we use the recipient's index as tie-breaker to have a well-defined
 *  order, i.e. within scan pubkey groups, the spend pubkeys appear in the same order as they were
 *  passed in.
 */
static int secp256k1_silentpayments_recipient_sort_cmp(const void* pk1, const void* pk2, void *ctx) {
    const secp256k1_silentpayments_recipient *r1 = *(const secp256k1_silentpayments_recipient **)pk1;
    const secp256k1_silentpayments_recipient *r2 = *(const secp256k1_silentpayments_recipient **)pk2;

    const int ret = secp256k1_ec_pubkey_cmp((secp256k1_context *)ctx, &r1->scan_pubkey, &r2->scan_pubkey);
    if (ret != 0) {
        return ret;
    } else {
        return (r1->index > r2->index) - (r1->index < r2->index);
    }
}

static void secp256k1_silentpayments_recipient_sort(const secp256k1_context* ctx, const secp256k1_silentpayments_recipient **recipients, size_t n_recipients) {
    /* Suppress wrong warning (fixed in MSVC 19.33) */
    #if defined(_MSC_VER) && (_MSC_VER < 1933)
    #pragma warning(push)
    #pragma warning(disable: 4090)
    #endif

    secp256k1_hsort(recipients, n_recipients, sizeof(*recipients), secp256k1_silentpayments_recipient_sort_cmp, (void *)ctx);

    #if defined(_MSC_VER) && (_MSC_VER < 1933)
    #pragma warning(pop)
    #endif
}

/** Set hash state to the BIP340 tagged hash midstate for "BIP0352/Inputs". */
static void secp256k1_silentpayments_sha256_init_inputs(secp256k1_sha256* hash) {
    static const uint32_t midstate[8] = {
        0xd4143ffcul, 0x012ea4b5ul, 0x36e21c8ful, 0xf7ec7b54ul,
        0x4dd4e2acul, 0x9bcaa0a4ul, 0xe244899bul, 0xcd06903eul
    };
    secp256k1_sha256_initialize_midstate(hash, 64, midstate);
}

/** Callers must ensure that pubkey_sum is not the point at infinity before calling this function. */
static int secp256k1_silentpayments_calculate_input_hash_scalar(const secp256k1_hash_ctx *hash_ctx, secp256k1_scalar *input_hash_scalar, const unsigned char *outpoint_smallest36, secp256k1_ge *pubkey_sum) {
    secp256k1_sha256 hash;
    unsigned char pubkey_sum_ser[33];
    unsigned char input_hash[32];
    int overflow;

    secp256k1_silentpayments_sha256_init_inputs(&hash);
    secp256k1_sha256_write(hash_ctx, &hash, outpoint_smallest36, 36);
    secp256k1_eckey_pubkey_serialize33(pubkey_sum, pubkey_sum_ser);
    secp256k1_sha256_write(hash_ctx, &hash, pubkey_sum_ser, sizeof(pubkey_sum_ser));
    secp256k1_sha256_finalize(hash_ctx, &hash, input_hash);
    /* Convert input_hash to a scalar.
     *
     * This can only fail if the output of the hash function is zero or greater than or equal to the curve order, which
     * happens with negligible probability. Normally, we would use VERIFY_CHECK as opposed to returning an error
     * since returning an error here would result in an untestable branch in the code. But in this case, we return
     * an error to ensure strict compliance with BIP0352.
     */
    secp256k1_scalar_set_b32(input_hash_scalar, input_hash, &overflow);
    return (!secp256k1_scalar_is_zero(input_hash_scalar)) & (!overflow);
}

static void secp256k1_silentpayments_create_shared_secret(unsigned char *shared_secret33, const secp256k1_ge *public_component, const secp256k1_scalar *secret_component) {
    secp256k1_gej ss_j;
    secp256k1_ge ss;

    VERIFY_CHECK(!secp256k1_ge_is_infinity(public_component));
    VERIFY_CHECK(!secp256k1_scalar_is_zero(secret_component));

    secp256k1_ecmult_const(&ss_j, public_component, secret_component);
    secp256k1_ge_set_gej(&ss, &ss_j);

    /* serialize shared secret in constant-time */
    secp256k1_fe_normalize(&ss.x);
    secp256k1_fe_normalize(&ss.y);
    shared_secret33[0] = SECP256K1_TAG_PUBKEY_EVEN | secp256k1_fe_is_odd(&ss.y);
    secp256k1_fe_get_b32(&shared_secret33[1], &ss.x);

    /* Leaking these values would break indistinguishability of the transaction, so clear them. */
    secp256k1_ge_clear(&ss);
    secp256k1_gej_clear(&ss_j);
}

/** Set hash state to the BIP340 tagged hash midstate for "BIP0352/SharedSecret". */
static void secp256k1_silentpayments_sha256_init_sharedsecret(secp256k1_sha256* hash) {
    static const uint32_t midstate[8] = {
        0x88831537ul, 0x5127079bul, 0x69c2137bul, 0xab0303e6ul,
        0x98fa21faul, 0x4a888523ul, 0xbd99daabul, 0xf25e5e0aul
    };
    secp256k1_sha256_initialize_midstate(hash, 64, midstate);
}

static int secp256k1_silentpayments_create_output_tweak(const secp256k1_context *ctx, secp256k1_scalar *t_k_scalar, const unsigned char *shared_secret33, uint32_t k) {
    const secp256k1_hash_ctx *hash_ctx = secp256k1_get_hash_context(ctx);
    secp256k1_sha256 hash;
    unsigned char hash_ser[32];
    unsigned char k_serialized[4];
    int overflow;

    /* Compute hash(shared_secret || ser_32(k))  [sha256 with tag "BIP0352/SharedSecret"] */
    secp256k1_silentpayments_sha256_init_sharedsecret(&hash);
    secp256k1_sha256_write(hash_ctx, &hash, shared_secret33, 33);
    secp256k1_write_be32(k_serialized, k);
    secp256k1_sha256_write(hash_ctx, &hash, k_serialized, sizeof(k_serialized));
    secp256k1_sha256_finalize(hash_ctx, &hash, hash_ser);

    /* The only thing that the attacker can do with the hashed secret is derive the final pubkeys.
     * On the side of the sender, we assume that will reveal those on the blockchain anyway.
     * On the side of the scanner, we assume that the caller wants to branch when a payment is found. */
    secp256k1_declassify(ctx, hash_ser, sizeof(hash_ser));

    /* Convert output tweak t_k to a scalar.
     *
     * This can only fail if the output of the hash function is zero or greater than or equal to the curve order, which
     * happens with negligible probability. Normally, we would use VERIFY_CHECK as opposed to returning an error
     * since returning an error here would result in an untestable branch in the code. But in this case, we return
     * an error to ensure strict compliance with BIP0352.
     */
    secp256k1_scalar_set_b32(t_k_scalar, hash_ser, &overflow);
    /* Leaking this value would break indistinguishability of the transaction, so clear it. */
    secp256k1_memclear_explicit(hash_ser, sizeof(hash_ser));
    secp256k1_sha256_clear(&hash);
    return (!secp256k1_scalar_is_zero(t_k_scalar)) & (!overflow);
}

static int secp256k1_silentpayments_create_output_pubkey(const secp256k1_context *ctx, secp256k1_xonly_pubkey *output_xonly, const unsigned char *shared_secret33, const secp256k1_pubkey *spend_pubkey, uint32_t k) {
    secp256k1_ge output_ge;
    secp256k1_scalar t_k_scalar;
    /* Calculate the output tweak t_k and convert it to a scalar.
     *
     * Note: _create_output_tweak can only fail if the output of the hash function is zero or greater than or equal to
     * the curve order, which is statistically improbable. Returning an error here results in an untestable branch in
     * the code, but we do this anyways to ensure strict compliance with BIP0352.
     */
    if (!secp256k1_silentpayments_create_output_tweak(ctx, &t_k_scalar, shared_secret33, k)) {
        secp256k1_scalar_clear(&t_k_scalar);
        return 0;
    }

    if (!secp256k1_pubkey_load(ctx, &output_ge, spend_pubkey)) {
        secp256k1_scalar_clear(&t_k_scalar);
        return 0;
    }
    /* `tweak_add` only fails if t_k_scalar * G = -spend_pubkey. Considering t_k is the output of a hash function, this
     * will happen only with negligible probability for honestly created spend_pubkey, but we handle this error anyway
     * to protect against this function being called with malicious inputs, i.e.,
     *     spend_pubkey = -(_create_output_tweak(shared_secret33, k))*G
     */
    if (!secp256k1_eckey_pubkey_tweak_add(&output_ge, &t_k_scalar)) {
        secp256k1_scalar_clear(&t_k_scalar);
        return 0;
    }
    secp256k1_fe_normalize_var(&output_ge.y);
    secp256k1_extrakeys_ge_even_y(&output_ge);
    secp256k1_xonly_pubkey_save(output_xonly, &output_ge);

    /* Leaking this value would break indistinguishability of the transaction, so clear it. */
    secp256k1_scalar_clear(&t_k_scalar);
    return 1;
}

int secp256k1_silentpayments_sender_create_outputs(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey **generated_outputs,
    const secp256k1_silentpayments_recipient **recipients,
    size_t n_recipients,
    const unsigned char *outpoint_smallest36,
    const secp256k1_keypair * const *keypairs,
    size_t n_keypairs,
    const unsigned char * const *seckeys,
    size_t n_seckeys
) {
    size_t i;
    uint32_t k;
    secp256k1_scalar seckey_sum_scalar, addend, input_hash_scalar;
    secp256k1_ge prevouts_pubkey_sum_ge;
    unsigned char shared_secret[33];
    secp256k1_pubkey current_scan_pubkey;
    int ret, sum_is_zero;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(generated_outputs != NULL);
    ARG_CHECK(recipients != NULL);
    ARG_CHECK(n_recipients > 0);
    ARG_CHECK(outpoint_smallest36 != NULL);
    ARG_CHECK((seckeys != NULL) || (keypairs != NULL));
    if (keypairs != NULL) {
        ARG_CHECK(n_keypairs > 0);
        for (i = 0; i < n_keypairs; i++) {
            ARG_CHECK(keypairs[i] != NULL);
        }
    } else {
        ARG_CHECK(n_keypairs == 0);
    }
    if (seckeys != NULL) {
        ARG_CHECK(n_seckeys > 0);
        for (i = 0; i < n_seckeys; i++) {
            ARG_CHECK(seckeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_seckeys == 0);
    }
    for (i = 0; i < n_recipients; i++) {
        ARG_CHECK(generated_outputs[i] != NULL);
        ARG_CHECK(recipients[i] != NULL);
        ARG_CHECK(recipients[i]->index == i);
    }

    seckey_sum_scalar = secp256k1_scalar_zero;
    for (i = 0; i < n_seckeys; i++) {
        ret = secp256k1_scalar_set_b32_seckey(&addend, seckeys[i]);
        secp256k1_declassify(ctx, &ret, sizeof(ret));
        if (!ret) {
            secp256k1_scalar_clear(&addend);
            secp256k1_scalar_clear(&seckey_sum_scalar);
            return 0;
        }
        secp256k1_scalar_add(&seckey_sum_scalar, &seckey_sum_scalar, &addend);
    }
    /* Secret keys used for taproot outputs have to be negated if they result in an odd point. This is to ensure
     * the sender and recipient can arrive at the same shared secret when using x-only public keys. */
    for (i = 0; i < n_keypairs; i++) {
        secp256k1_ge addend_point;
        ret = secp256k1_keypair_load(ctx, &addend, &addend_point, keypairs[i]);
        secp256k1_declassify(ctx, &ret, sizeof(ret));
        if (!ret) {
            secp256k1_scalar_clear(&addend);
            secp256k1_scalar_clear(&seckey_sum_scalar);
            return 0;
        }
        if (secp256k1_fe_is_odd(&addend_point.y)) {
            secp256k1_scalar_negate(&addend, &addend);
        }
        secp256k1_scalar_add(&seckey_sum_scalar, &seckey_sum_scalar, &addend);
    }
    /* If there are any failures in loading/summing up the secret keys, fail early. */
    sum_is_zero = secp256k1_scalar_is_zero(&seckey_sum_scalar);
    secp256k1_declassify(ctx, &sum_is_zero, sizeof(sum_is_zero));
    secp256k1_scalar_clear(&addend);
    if (sum_is_zero) {
        secp256k1_scalar_clear(&seckey_sum_scalar);
        return 0;
    }
    secp256k1_ecmult_gen_ge(&ctx->ecmult_gen_ctx, &prevouts_pubkey_sum_ge, &seckey_sum_scalar);
    /* We declassify the pubkey sum because serializing a group element (done in the
     * `_calculate_input_hash_scalar` call following) is not a constant-time operation.
     */
    secp256k1_declassify(ctx, &prevouts_pubkey_sum_ge, sizeof(prevouts_pubkey_sum_ge));

    /* Calculate the input_hash and convert it to a scalar so that it can be multiplied with the summed up private keys, i.e., a_sum = a_sum * input_hash.
     * By multiplying the scalars together first, we can save an elliptic curve multiplication.
     *
     * Note: _input_hash_scalar can only fail if the output of the hash function is zero or greater than or equal to the
     * curve order, which is statistically improbable. Returning an error here results in an untestable branch in the
     * code, but we do this anyways to ensure strict compliance with BIP0352.
     */
    if (!secp256k1_silentpayments_calculate_input_hash_scalar(secp256k1_get_hash_context(ctx), &input_hash_scalar, outpoint_smallest36, &prevouts_pubkey_sum_ge)) {
        secp256k1_scalar_clear(&seckey_sum_scalar);
        return 0;
    }
    secp256k1_scalar_mul(&seckey_sum_scalar, &seckey_sum_scalar, &input_hash_scalar);
    /* _recipient_sort sorts the array of recipients in place by their scan public keys (lexicographically).
     * This ensures that all recipients with the same scan public key are grouped together, as specified in BIP0352.
     *
     * More specifically, this ensures `k` is incremented from 0 to the number of requested outputs for each recipient group,
     * where a recipient group is all addresses with the same scan public key.
     */
    secp256k1_silentpayments_recipient_sort(ctx, recipients, n_recipients);
    current_scan_pubkey = recipients[0]->scan_pubkey;
    k = 0;  /* This is a dead store but clang will emit a false positive warning if we omit it. */
    for (i = 0; i < n_recipients; i++) {
        if ((i == 0) || (secp256k1_ec_pubkey_cmp(ctx, &current_scan_pubkey, &recipients[i]->scan_pubkey) != 0)) {
            /* If we are on a different scan pubkey, its time to recreate the shared secret and reset k to 0.
             * It's very unlikely the scan public key is invalid by this point, since this means the caller would
             * have created the _silentpayments_recipient object incorrectly, but just to be sure we still check that
             * the public key is valid.
             */
            secp256k1_ge pk;
            if (!secp256k1_pubkey_load(ctx, &pk, &recipients[i]->scan_pubkey)) {
                secp256k1_scalar_clear(&seckey_sum_scalar);
                /* Leaking this value would break indistinguishability of the transaction, so clear it. */
                secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
                return 0;
            }
            /* Creating the shared secret requires that the public and secret components are
             * non-infinity and non-zero, respectively. Note that the involved parts (input hash,
             * secret key sum, and scan public key) have all been verified at this point. */
            secp256k1_silentpayments_create_shared_secret(shared_secret, &pk, &seckey_sum_scalar);
            k = 0;
        }
        /* If creating another output for the current recipient group exceeded the
         * protocol limit, fail, as the recipient wouldn't be guaranteed to find it. */
        if (k >= SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT) {
            secp256k1_scalar_clear(&seckey_sum_scalar);
            secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
            return 0;
        }
        if (!secp256k1_silentpayments_create_output_pubkey(ctx, generated_outputs[recipients[i]->index], shared_secret, &recipients[i]->spend_pubkey, k)) {
            secp256k1_scalar_clear(&seckey_sum_scalar);
            secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
            return 0;
        }
        current_scan_pubkey = recipients[i]->scan_pubkey;
        k++;
    }
    secp256k1_scalar_clear(&seckey_sum_scalar);
    secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
    return 1;
}

/** Set hash state to the BIP340 tagged hash midstate for "BIP0352/Label". */
static void secp256k1_silentpayments_sha256_init_label(secp256k1_sha256* hash) {
    static const uint32_t midstate[8] = {
        0x26b95d63ul, 0x8bf1b740ul, 0x10a5986ful, 0x06a387a5ul,
        0x2d1c1c30ul, 0xd035951aul, 0x2d7f0f96ul, 0x29e3e0dbul
    };
    secp256k1_sha256_initialize_midstate(hash, 64, midstate);
}

static const unsigned char secp256k1_silentpayments_label_magic[4] = { 0x27, 0x9d, 0x44, 0xba };

/* Saves a group element into a label. Requires that the provided group element is not infinity. */
static void secp256k1_silentpayments_label_save(secp256k1_silentpayments_label* label, const secp256k1_ge* ge) {
    memcpy(&label->data[0], secp256k1_silentpayments_label_magic, 4);
    secp256k1_ge_to_bytes(label->data + 4, ge);
}

/* Loads a group element from a label. Returns 1 unless the label wasn't properly initialized. */
static int secp256k1_silentpayments_label_load(const secp256k1_context* ctx, secp256k1_ge* ge, const secp256k1_silentpayments_label* label) {
    ARG_CHECK(secp256k1_memcmp_var(&label->data[0], secp256k1_silentpayments_label_magic, 4) == 0);
    secp256k1_ge_from_bytes(ge, label->data + 4);
    return 1;
}

int secp256k1_silentpayments_recipient_label_parse(const secp256k1_context* ctx, secp256k1_silentpayments_label* label, const unsigned char *in33) {
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(label != NULL);
    memset(label, 0, sizeof(*label));
    ARG_CHECK(in33 != NULL);

    if (!secp256k1_eckey_pubkey_parse(&ge, in33, 33)) {
        return 0;
    }

    secp256k1_silentpayments_label_save(label, &ge);
    return 1;
}

int secp256k1_silentpayments_recipient_label_serialize(const secp256k1_context* ctx, unsigned char *out33, const secp256k1_silentpayments_label* label) {
    secp256k1_ge ge;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(out33 != NULL);
    memset(out33, 0, 33);
    ARG_CHECK(label != NULL);

    if (!secp256k1_silentpayments_label_load(ctx, &ge, label)) {
        return 0;
    }
    secp256k1_eckey_pubkey_serialize33(&ge, out33);
    return 1;
}

int secp256k1_silentpayments_recipient_label_create(const secp256k1_context *ctx, secp256k1_silentpayments_label *label, unsigned char *label_tweak32, const unsigned char *scan_key32, uint32_t m) {
    secp256k1_sha256 hash;
    unsigned char m_serialized[4];
    secp256k1_ge label_ge;
    secp256k1_scalar label_tweak_scalar;
    int ret;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(label != NULL);
    memset(label, 0, sizeof(*label));
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(label_tweak32 != NULL);
    ARG_CHECK(scan_key32 != NULL);

    /* ensure that the passed scan key is valid, in order to avoid creating unspendable labels */
    ret = secp256k1_ec_seckey_verify(ctx, scan_key32);

    /* Compute hash(ser_256(b_scan) || ser_32(m))  [sha256 with tag "BIP0352/Label"] */
    secp256k1_silentpayments_sha256_init_label(&hash);
    secp256k1_sha256_write(secp256k1_get_hash_context(ctx), &hash, scan_key32, 32);
    secp256k1_write_be32(m_serialized, m);
    secp256k1_sha256_write(secp256k1_get_hash_context(ctx), &hash, m_serialized, sizeof(m_serialized));
    secp256k1_sha256_finalize(secp256k1_get_hash_context(ctx), &hash, label_tweak32);

    ret &= secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &label_tweak_scalar, &label_ge, label_tweak32);
    secp256k1_silentpayments_label_save(label, &label_ge);
    secp256k1_memczero(label, sizeof(*label), !ret);
    secp256k1_memczero(label_tweak32, 32, !ret);

    secp256k1_scalar_clear(&label_tweak_scalar);
    secp256k1_memclear_explicit(m_serialized, sizeof(m_serialized));
    secp256k1_sha256_clear(&hash);

    return ret;
}

int secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(const secp256k1_context *ctx, secp256k1_pubkey *labeled_spend_pubkey, const secp256k1_pubkey *unlabeled_spend_pubkey, const secp256k1_silentpayments_label *label) {
    secp256k1_ge labeled_spend_pubkey_ge, label_addend;
    secp256k1_gej result_gej;
    secp256k1_ge result_ge;
    int ret;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(labeled_spend_pubkey != NULL);
    memset(labeled_spend_pubkey, 0, sizeof(*labeled_spend_pubkey));
    ARG_CHECK(unlabeled_spend_pubkey != NULL);
    ARG_CHECK(label != NULL);

    /* Calculate labeled_spend_pubkey = unlabeled_spend_pubkey + label.
     * If either the label or spend public key is invalid, return early.
     */
    ret = secp256k1_pubkey_load(ctx, &labeled_spend_pubkey_ge, unlabeled_spend_pubkey);
    ret &= secp256k1_silentpayments_label_load(ctx, &label_addend, label);
    if (!ret) {
        return 0;
    }
    secp256k1_gej_set_ge(&result_gej, &labeled_spend_pubkey_ge);
    secp256k1_gej_add_ge_var(&result_gej, &result_gej, &label_addend, NULL);
    if (secp256k1_gej_is_infinity(&result_gej)) {
        return 0;
    }

    secp256k1_ge_set_gej_var(&result_ge, &result_gej);
    secp256k1_pubkey_save(labeled_spend_pubkey, &result_ge);

    return 1;
}

/** An explanation of the prevouts_summary object and its usage:
 *
 *  The prevouts_summary object contains:
 *
 *  [magic: 4 bytes][boolean: 1 byte][prevouts_pubkey_sum: 64 bytes][input_hash: 32 bytes]
 *
 *  The magic bytes are checked by functions using the prevouts_summary object to
 *  check that the prevouts_summary object was initialized correctly.
 *
 *  The boolean (combined) indicates whether or not the summed prevout public keys and the
 *  input_hash scalar have already been combined or are both included. The reason
 *  for keeping input_hash and the summed prevout public keys separate is so that an elliptic
 *  curve multiplication can be avoided when creating the shared secret, i.e.,
 *  (recipient_scan_key * input_hash) * prevouts_pubkey_sum.
 *
 *  But when storing the prevouts_summary object (not supported yet), either to send to
 *  light clients or for wallet rescans, we can save 32-bytes by combining the input_hash
 *  and prevouts_pubkey_sum and saving the resulting point serialized as a compressed
 *  public key, i.e., input_hash * prevouts_pubkey_sum.
 *
 *  For each function:
 *
 *  - `_recipient_prevouts_summary_create` always creates a prevouts_summary object with combined = false
 */

int secp256k1_silentpayments_recipient_prevouts_summary_create(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_prevouts_summary *prevouts_summary,
    const unsigned char *outpoint_smallest36,
    const secp256k1_xonly_pubkey * const *xonly_pubkeys,
    size_t n_xonly_pubkeys,
    const secp256k1_pubkey * const *pubkeys,
    size_t n_pubkeys
) {
    size_t i;
    secp256k1_ge prevouts_pubkey_sum_ge, addend;
    secp256k1_gej prevouts_pubkey_sum_gej;
    secp256k1_scalar input_hash_scalar;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    memset(prevouts_summary, 0, sizeof(*prevouts_summary));
    ARG_CHECK(outpoint_smallest36 != NULL);
    ARG_CHECK((pubkeys != NULL) || (xonly_pubkeys != NULL));
    if (xonly_pubkeys != NULL) {
        ARG_CHECK(n_xonly_pubkeys > 0);
        for (i = 0; i < n_xonly_pubkeys; i++) {
            ARG_CHECK(xonly_pubkeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_xonly_pubkeys == 0);
    }
    if (pubkeys != NULL) {
        ARG_CHECK(n_pubkeys > 0);
        for (i = 0; i < n_pubkeys; i++) {
            ARG_CHECK(pubkeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_pubkeys == 0);
    }

    /* Compute prevouts_pubkey_sum = A_1 + A_2 + ... + A_n.
     *
     * Since an attacker can maliciously craft transactions where the public keys sum to zero, fail early here
     * to avoid making the caller do extra work, e.g., when building an index or scanning a malicious transaction.
     *
     * This will also fail if any of the provided prevout public keys are malformed.
     */
    secp256k1_gej_set_infinity(&prevouts_pubkey_sum_gej);
    for (i = 0; i < n_pubkeys; i++) {
        if (!secp256k1_pubkey_load(ctx, &addend, pubkeys[i])) {
            return 0;
        }
        secp256k1_gej_add_ge_var(&prevouts_pubkey_sum_gej, &prevouts_pubkey_sum_gej, &addend, NULL);
    }
    for (i = 0; i < n_xonly_pubkeys; i++) {
        if (!secp256k1_xonly_pubkey_load(ctx, &addend, xonly_pubkeys[i])) {
            return 0;
        }
        secp256k1_gej_add_ge_var(&prevouts_pubkey_sum_gej, &prevouts_pubkey_sum_gej, &addend, NULL);
    }
    if (secp256k1_gej_is_infinity(&prevouts_pubkey_sum_gej)) {
        return 0;
    }
    secp256k1_ge_set_gej_var(&prevouts_pubkey_sum_ge, &prevouts_pubkey_sum_gej);
    /* Calculate the input_hash and convert it to a scalar.
     *
     * Note: _input_hash_scalar can only fail if the output of the hash function is zero or greater than or equal to the
     * curve order, which is statistically improbable. Returning an error here results in an untestable branch in the
     * code, but we do this anyways to ensure strict compliance with BIP0352.
     */
    if (!secp256k1_silentpayments_calculate_input_hash_scalar(secp256k1_get_hash_context(ctx), &input_hash_scalar, outpoint_smallest36, &prevouts_pubkey_sum_ge)) {
        return 0;
    }
    memcpy(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4);
    prevouts_summary->data[4] = 0;
    secp256k1_ge_to_bytes(&prevouts_summary->data[5], &prevouts_pubkey_sum_ge);
    secp256k1_scalar_get_b32(&prevouts_summary->data[5 + 64], &input_hash_scalar);
    return 1;
}

int secp256k1_silentpayments_recipient_scan_outputs(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_found_output **found_outputs, uint32_t *n_found_outputs,
    const secp256k1_xonly_pubkey * const *tx_outputs, size_t n_tx_outputs,
    const unsigned char *scan_key32,
    const secp256k1_silentpayments_prevouts_summary *prevouts_summary,
    const secp256k1_pubkey *unlabeled_spend_pubkey,
    secp256k1_silentpayments_label_lookup label_lookup,
    const void *label_context
) {
    secp256k1_scalar scan_key_scalar;
    secp256k1_ge unlabeled_spend_pubkey_ge, prevouts_pubkey_sum_ge, tx_output_ge;
    unsigned char shared_secret[33];
    uint32_t k, k_max;
    size_t i;
    int found_idx, combined, valid_scan_key, ret;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(found_outputs != NULL);
    ARG_CHECK(n_found_outputs != NULL);
    *n_found_outputs = 0;
    ARG_CHECK(tx_outputs != NULL);
    ARG_CHECK(n_tx_outputs > 0);
    for (i = 0; i < n_tx_outputs; i++) {
        ARG_CHECK(found_outputs[i] != NULL);
        ARG_CHECK(tx_outputs[i] != NULL);
        /* Validate each tx output object early so malformed pubkeys are always rejected. */
        if (!secp256k1_xonly_pubkey_load(ctx, &tx_output_ge, tx_outputs[i])) {
            return 0;
        }
    }
    ARG_CHECK(scan_key32 != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4) == 0);
    ARG_CHECK(unlabeled_spend_pubkey != NULL);
    /* Passing a context without a lookup function is non-sensical */
    if (label_context != NULL) {
        ARG_CHECK(label_lookup != NULL);
    }
    valid_scan_key = secp256k1_scalar_set_b32_seckey(&scan_key_scalar, scan_key32);
    secp256k1_declassify(ctx, &valid_scan_key, sizeof(valid_scan_key));
    if (!valid_scan_key) {
        secp256k1_scalar_clear(&scan_key_scalar);
        return 0;
    }
    secp256k1_ge_from_bytes(&prevouts_pubkey_sum_ge, &prevouts_summary->data[5]);
    combined = (int)prevouts_summary->data[4];
    if (!combined) {
        secp256k1_scalar input_hash_scalar;
        secp256k1_scalar_set_b32(&input_hash_scalar, &prevouts_summary->data[5 + 64], NULL);
        secp256k1_scalar_mul(&scan_key_scalar, &scan_key_scalar, &input_hash_scalar);
    }
    ret = secp256k1_pubkey_load(ctx, &unlabeled_spend_pubkey_ge, unlabeled_spend_pubkey);
    if (!ret) {
        secp256k1_scalar_clear(&scan_key_scalar);
        return 0;
    }
    /* Creating the shared secret requires that the public and secret components are
     * non-infinity and non-zero, respectively. Note that the involved parts (input hash,
     * scan secret key, and prevouts public key sum) have all been verified at this point,
     * assuming that the user hasn't tampered the `prevouts_summary` object manually. */
    secp256k1_silentpayments_create_shared_secret(shared_secret, &prevouts_pubkey_sum_ge, &scan_key_scalar);
    /* Clear the scan_key_scalar since we no longer need it and leaking this value would break indistinguishability of the transaction. */
    secp256k1_scalar_clear(&scan_key_scalar);

    found_idx = -1;
    /* Don't look further than the per-group recipient limit, in order to avoid quadratic scaling issues. */
    k_max = (n_tx_outputs < SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT) ?
             n_tx_outputs : SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT;
    for (k = 0; k < k_max; k++) {
        secp256k1_scalar t_k_scalar;
        secp256k1_xonly_pubkey unlabeled_output_xonly;
        secp256k1_ge unlabeled_output_ge = unlabeled_spend_pubkey_ge;
        secp256k1_ge unlabeled_output_negated_ge;
        /* Label scanning involves the transformation from Jacobian (gej) to affine (ge) coordinates
         * for serializing label candidates. As this is an expensive operation involving modular
         * inversion, we don't do this one by one for each tx output, but collect multiple label
         * candidates in Jacobian in order to apply Montgomery's trick for batch inversion (using
         * the function `secp256k1_ge_set_all_gej_var`). For transactions with a large number of
         * outputs, this speeds up scanning significantly (~2.5x). */
        enum { LABEL_BATCH_SIZE = 8 }; /* batch size expressed in number of tx outputs */
        secp256k1_gej label_candidates_gej[2 * LABEL_BATCH_SIZE]; /* two candidates per tx output (one per y-parity) */
        size_t label_batch_idx = 0; /* current index within a batch */
        const unsigned char *label_tweak = NULL;
        secp256k1_ge label_ge;
        size_t j;

        /* Calculate the output tweak t_k and convert it to a scalar.
         *
         * Note: _create_output_tweak can only fail if the output of the hash function is zero or greater than or equal
         * to the curve order, which is statistically improbable. Returning an error here results in an untestable
         * branch in the code, but we do this anyways to ensure strict compliance with BIP0352.
         */
        if (!secp256k1_silentpayments_create_output_tweak(ctx, &t_k_scalar, shared_secret, k)) {
            secp256k1_scalar_clear(&t_k_scalar);
            secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
            return 0;
        }

        /* Calculate unlabeled_output = unlabeled_spend_pubkey + t_k * G.
         * This can fail if t_k * G is the negation of unlabeled_spend_pubkey, but this happens only with negligible
         * probability for honestly created unlabeled_spend_pubkey as t_k is the output of a hash function. */
        if (!secp256k1_eckey_pubkey_tweak_add(&unlabeled_output_ge, &t_k_scalar)) {
            /* Leaking these values would break indistinguishability of the transaction, so clear them. */
            secp256k1_scalar_clear(&t_k_scalar);
            secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
            return 0;
        }
        /* Calculate unlabeled_output_negated = -unlabeled_output */
        secp256k1_ge_neg(&unlabeled_output_negated_ge, &unlabeled_output_ge);

        found_idx = -1;
        secp256k1_fe_normalize_var(&unlabeled_output_ge.y);
        secp256k1_extrakeys_ge_even_y(&unlabeled_output_ge);
        secp256k1_xonly_pubkey_save(&unlabeled_output_xonly, &unlabeled_output_ge);
        for (j = 0; j < n_tx_outputs; j++) {
            if (secp256k1_xonly_pubkey_cmp(ctx, &unlabeled_output_xonly, tx_outputs[j]) == 0) {
                label_tweak = NULL;
                found_idx = j;
                break;
            }

            /* If not found, proceed to check for labels (if a label lookup function is provided). */
            if (label_lookup != NULL) {
                secp256k1_gej tx_output_gej;
                secp256k1_gej *label_candidate1 = &label_candidates_gej[2 * label_batch_idx];
                secp256k1_gej *label_candidate2 = &label_candidates_gej[2 * label_batch_idx + 1];

                /* Calculate scan label candidates:
                 *     label_candidate1 =  tx_output - unlabeled_output
                 *     label_candidate2 = -tx_output - unlabeled_output
                 * and store them in the batch */
                secp256k1_xonly_pubkey_load(ctx, &tx_output_ge, tx_outputs[j]);
                secp256k1_gej_set_ge(&tx_output_gej, &tx_output_ge);
                secp256k1_gej_add_ge_var(label_candidate1, &tx_output_gej, &unlabeled_output_negated_ge, NULL);
                secp256k1_gej_neg(&tx_output_gej, &tx_output_gej);
                secp256k1_gej_add_ge_var(label_candidate2, &tx_output_gej, &unlabeled_output_negated_ge, NULL);
                label_batch_idx++;
                /* If the batch is filled or we have reached the last transaction output, perform batch
                 * inversion and check the label cache for each label candidate entry in the batch */
                if (label_batch_idx == LABEL_BATCH_SIZE || j == (n_tx_outputs-1)) {
                    secp256k1_ge label_candidates_ge[2 * LABEL_BATCH_SIZE];
                    unsigned char label33[33];
                    size_t j_start = j + 1 - label_batch_idx; /* tx outputs index that matches the first batch entry */

                    secp256k1_ge_set_all_gej_var(label_candidates_ge, label_candidates_gej, 2 * label_batch_idx);
                    for (i = 0; i < label_batch_idx; i++) {
                        /* Note: serialize will only fail if label_ge is the point at infinity, but we know this
                         * cannot happen since we only hit this branch if tx_output != unlabeled_output_xonly.
                         * Thus, we know that label_ge = tx_output_gej + unlabeled_output_negated_ge cannot be the
                         * point at infinity.
                         */
                        secp256k1_eckey_pubkey_serialize33(&label_candidates_ge[2 * i], label33);
                        label_tweak = label_lookup(label33, label_context);
                        if (label_tweak != NULL) {
                            found_idx = j_start + i;
                            label_ge = label_candidates_ge[2 * i];
                            break;
                        }

                        secp256k1_eckey_pubkey_serialize33(&label_candidates_ge[2 * i + 1], label33);
                        label_tweak = label_lookup(label33, label_context);
                        if (label_tweak != NULL) {
                            found_idx = j_start + i;
                            label_ge = label_candidates_ge[2 * i + 1];
                            break;
                        }
                    }
                    label_batch_idx = 0;
                }
                if (found_idx != -1) {
                    break;
                }
            }
        }
        if (found_idx != -1) {
            found_outputs[k]->output = *tx_outputs[found_idx];
            secp256k1_scalar_get_b32(found_outputs[k]->tweak, &t_k_scalar);
            /* Clear the t_k_scalar since we no longer need it and leaking this value would
             * break indistinguishability of the transaction. */
            secp256k1_scalar_clear(&t_k_scalar);
            if (label_tweak != NULL) {
                found_outputs[k]->found_with_label = 1;
                /* This is extremely unlikely to fail in that it can only really happen if label_tweak
                 * is the negation of the shared secret tweak. But since both tweak and label_tweak are
                 * created by hashing data, practically speaking this would only happen if an attacker
                 * tricked us into using a particular label_tweak (deviating from the protocol).
                 * Note that this call could also fail due to a malformed label_tweak data from the
                 * label cache, but we generally assume the passed in data is created using the API
                 * functions and thus have already been checked for correctness.
                 *
                 * Furthermore, although technically a failure for ec_seckey_tweak_add, this is not treated
                 * as a failure for Silent Payments because the output is still spendable with just the
                 * spend secret key. We set `tweak = 0` for this case.
                 */
                if (!secp256k1_ec_seckey_tweak_add(ctx, found_outputs[k]->tweak, label_tweak)) {
                    memset(found_outputs[k]->tweak, 0, 32);
                }
                secp256k1_silentpayments_label_save(&found_outputs[k]->label, &label_ge);
            } else {
                found_outputs[k]->found_with_label = 0;
                /* Set the label to an invalid value. */
                memset(&found_outputs[k]->label, 0, sizeof(found_outputs[k]->label));
            }
            /* Reset everything for the next round of scanning. */
            label_tweak = NULL;
        } else {
            secp256k1_scalar_clear(&t_k_scalar);
            break;
        }
    }
    *n_found_outputs = k;

    /* Leaking the shared_secret would break indistinguishability of the transaction, so clear it. */
    secp256k1_memclear_explicit(shared_secret, sizeof(shared_secret));
    return 1;
}

#endif
