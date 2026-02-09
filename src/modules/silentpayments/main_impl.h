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

    int ret = secp256k1_ec_pubkey_cmp((secp256k1_context *)ctx, &r1->scan_pubkey, &r2->scan_pubkey);
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

/** Callers must ensure that pubkey_sum is not the point at infinity before calling this function. */
static int secp256k1_silentpayments_calculate_input_hash_scalar(secp256k1_scalar *input_hash_scalar, const unsigned char *outpoint_smallest36, secp256k1_ge *pubkey_sum) {
    secp256k1_sha256 hash;
    unsigned char pubkey_sum_ser[33];
    unsigned char input_hash[32];
    int overflow;

    secp256k1_silentpayments_sha256_init_inputs(&hash);
    secp256k1_sha256_write(&hash, outpoint_smallest36, 36);
    secp256k1_eckey_pubkey_serialize33(pubkey_sum, pubkey_sum_ser);
    secp256k1_sha256_write(&hash, pubkey_sum_ser, sizeof(pubkey_sum_ser));
    secp256k1_sha256_finalize(&hash, input_hash);
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

static void secp256k1_silentpayments_create_shared_secret(const secp256k1_context *ctx, unsigned char *shared_secret33, const secp256k1_ge *public_component, const secp256k1_scalar *secret_component) {
    secp256k1_gej ss_j;
    secp256k1_ge ss;

    VERIFY_CHECK(!secp256k1_ge_is_infinity(public_component));
    VERIFY_CHECK(!secp256k1_scalar_is_zero(secret_component));

    secp256k1_ecmult_const(&ss_j, public_component, secret_component);
    secp256k1_ge_set_gej(&ss, &ss_j);
    /* We declassify the shared secret group element because serializing a group element is a non-constant time operation. */
    secp256k1_declassify(ctx, &ss, sizeof(ss));
    secp256k1_eckey_pubkey_serialize33(&ss, shared_secret33);

    /* Leaking these values would break indistinguishability of the transaction, so clear them. */
    secp256k1_ge_clear(&ss);
    secp256k1_gej_clear(&ss_j);
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

static int secp256k1_silentpayments_create_output_tweak(secp256k1_scalar *output_tweak_scalar, const unsigned char *shared_secret33, uint32_t k) {
    secp256k1_sha256 hash;
    unsigned char hash_ser[32];
    unsigned char k_serialized[4];
    int overflow;

    /* Compute hash(shared_secret || ser_32(k))  [sha256 with tag "BIP0352/SharedSecret"] */
    secp256k1_silentpayments_sha256_init_sharedsecret(&hash);
    secp256k1_sha256_write(&hash, shared_secret33, 33);
    secp256k1_write_be32(k_serialized, k);
    secp256k1_sha256_write(&hash, k_serialized, sizeof(k_serialized));
    secp256k1_sha256_finalize(&hash, hash_ser);
    /* Convert output_tweak to a scalar.
     *
     * This can only fail if the output of the hash function is zero or greater than or equal to the curve order, which
     * happens with negligible probability. Normally, we would use VERIFY_CHECK as opposed to returning an error
     * since returning an error here would result in an untestable branch in the code. But in this case, we return
     * an error to ensure strict compliance with BIP0352.
     */
    secp256k1_scalar_set_b32(output_tweak_scalar, hash_ser, &overflow);
    /* Leaking this value would break indistinguishability of the transaction, so clear it. */
    secp256k1_memclear_explicit(hash_ser, sizeof(hash_ser));
    secp256k1_sha256_clear(&hash);
    return (!secp256k1_scalar_is_zero(output_tweak_scalar)) & (!overflow);
}

static int secp256k1_silentpayments_create_output_pubkey(const secp256k1_context *ctx, secp256k1_xonly_pubkey *output_xonly, const unsigned char *shared_secret33, const secp256k1_pubkey *spend_pubkey, uint32_t k) {
    secp256k1_ge output_ge;
    secp256k1_scalar output_tweak_scalar;
    /* Calculate the output_tweak and convert it to a scalar.
     *
     * Note: _create_output_tweak can only fail if the output of the hash function is zero or greater than or equal to
     * the curve order, which is statistically improbable. Returning an error here results in an untestable branch in
     * the code, but we do this anyways to ensure strict compliance with BIP0352.
     */
    if (!secp256k1_silentpayments_create_output_tweak(&output_tweak_scalar, shared_secret33, k)) {
        return 0;
    }
    if (!secp256k1_pubkey_load(ctx, &output_ge, spend_pubkey)) {
        secp256k1_scalar_clear(&output_tweak_scalar);
        return 0;
    }
    /* `tweak_add` only fails if output_tweak_scalar*G = -spend_pubkey. Considering output_tweak is the output of a hash function,
     * this will happen only with negligible probability for honestly created spend_pubkey, but we handle this
     * error anyway to protect against this function being called with a malicious inputs, i.e., spend_pubkey = -(_create_output_tweak(shared_secret33, k))*G
     */
    if (!secp256k1_eckey_pubkey_tweak_add(&output_ge, &output_tweak_scalar)) {
        secp256k1_scalar_clear(&output_tweak_scalar);
        return 0;
    }
    secp256k1_xonly_pubkey_save(output_xonly, &output_ge);

    /* Leaking this value would break indistinguishability of the transaction, so clear it. */
    secp256k1_scalar_clear(&output_tweak_scalar);
    return 1;
}

int secp256k1_silentpayments_sender_create_outputs(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey **generated_outputs,
    const secp256k1_silentpayments_recipient **recipients,
    size_t n_recipients,
    const unsigned char *outpoint_smallest36,
    const secp256k1_keypair * const *taproot_seckeys,
    size_t n_taproot_seckeys,
    const unsigned char * const *plain_seckeys,
    size_t n_plain_seckeys
) {
    size_t i;
    uint32_t k;
    secp256k1_scalar seckey_sum_scalar, addend, input_hash_scalar;
    secp256k1_ge prevouts_pubkey_sum_ge;
    secp256k1_gej prevouts_pubkey_sum_gej;
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
    ARG_CHECK((plain_seckeys != NULL) || (taproot_seckeys != NULL));
    if (taproot_seckeys != NULL) {
        ARG_CHECK(n_taproot_seckeys > 0);
        for (i = 0; i < n_taproot_seckeys; i++) {
            ARG_CHECK(taproot_seckeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_taproot_seckeys == 0);
    }
    if (plain_seckeys != NULL) {
        ARG_CHECK(n_plain_seckeys > 0);
        for (i = 0; i < n_plain_seckeys; i++) {
            ARG_CHECK(plain_seckeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_plain_seckeys == 0);
    }
    for (i = 0; i < n_recipients; i++) {
        ARG_CHECK(generated_outputs[i] != NULL);
        ARG_CHECK(recipients[i] != NULL);
        ARG_CHECK(recipients[i]->index == i);
    }

    seckey_sum_scalar = secp256k1_scalar_zero;
    for (i = 0; i < n_plain_seckeys; i++) {
        ret = secp256k1_scalar_set_b32_seckey(&addend, plain_seckeys[i]);
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
    for (i = 0; i < n_taproot_seckeys; i++) {
        secp256k1_ge addend_point;
        ret = secp256k1_keypair_load(ctx, &addend, &addend_point, taproot_seckeys[i]);
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
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &prevouts_pubkey_sum_gej, &seckey_sum_scalar);
    secp256k1_ge_set_gej(&prevouts_pubkey_sum_ge, &prevouts_pubkey_sum_gej);
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
    if (!secp256k1_silentpayments_calculate_input_hash_scalar(&input_hash_scalar, outpoint_smallest36, &prevouts_pubkey_sum_ge)) {
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
            secp256k1_silentpayments_create_shared_secret(ctx, shared_secret, &pk, &seckey_sum_scalar);
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
    secp256k1_sha256_write(&hash, scan_key32, 32);
    secp256k1_write_be32(m_serialized, m);
    secp256k1_sha256_write(&hash, m_serialized, sizeof(m_serialized));
    secp256k1_sha256_finalize(&hash, label_tweak32);

    ret &= secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &label_tweak_scalar, &label_ge, label_tweak32);
    secp256k1_silentpayments_label_save(label, &label_ge);

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
    ARG_CHECK(unlabeled_spend_pubkey != NULL);
    ARG_CHECK(label != NULL);

    /* Calculate labeled_spend_pubkey = spend_pubkey + label.
     * If either the label or spend public key is an invalid public key,
     * return early
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
    const secp256k1_pubkey * const *plain_pubkeys,
    size_t n_plain_pubkeys
) {
    size_t i;
    secp256k1_ge prevouts_pubkey_sum_ge, addend;
    secp256k1_gej prevouts_pubkey_sum_gej;
    secp256k1_scalar input_hash_scalar;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(outpoint_smallest36 != NULL);
    ARG_CHECK((plain_pubkeys != NULL) || (xonly_pubkeys != NULL));
    if (xonly_pubkeys != NULL) {
        ARG_CHECK(n_xonly_pubkeys > 0);
        for (i = 0; i < n_xonly_pubkeys; i++) {
            ARG_CHECK(xonly_pubkeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_xonly_pubkeys == 0);
    }
    if (plain_pubkeys != NULL) {
        ARG_CHECK(n_plain_pubkeys > 0);
        for (i = 0; i < n_plain_pubkeys; i++) {
            ARG_CHECK(plain_pubkeys[i] != NULL);
        }
    } else {
        ARG_CHECK(n_plain_pubkeys == 0);
    }

    /* Compute prevouts_pubkey_sum = A_1 + A_2 + ... + A_n.
     *
     * Since an attacker can maliciously craft transactions where the public keys sum to zero, fail early here
     * to avoid making the caller do extra work, e.g., when building an index or scanning a malicious transaction.
     *
     * This will also fail if any of the provided prevout public keys are malformed.
     */
    secp256k1_gej_set_infinity(&prevouts_pubkey_sum_gej);
    for (i = 0; i < n_plain_pubkeys; i++) {
        if (!secp256k1_pubkey_load(ctx, &addend, plain_pubkeys[i])) {
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
    if (!secp256k1_silentpayments_calculate_input_hash_scalar(&input_hash_scalar, outpoint_smallest36, &prevouts_pubkey_sum_ge)) {
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
    const secp256k1_pubkey *spend_pubkey,
    const secp256k1_silentpayments_label_lookup label_lookup,
    const void *label_context
) {
    secp256k1_scalar scan_key_scalar;
    secp256k1_ge spend_pubkey_ge, prevouts_pubkey_sum_ge;
    unsigned char shared_secret[33];
    uint32_t k, k_max;
    size_t i, found_idx;
    int found, combined, valid_scan_key, ret;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(found_outputs != NULL);
    ARG_CHECK(n_found_outputs != NULL);
    ARG_CHECK(tx_outputs != NULL);
    ARG_CHECK(n_tx_outputs > 0);
    for (i = 0; i < n_tx_outputs; i++) {
        ARG_CHECK(found_outputs[i] != NULL);
        ARG_CHECK(tx_outputs[i] != NULL);
    }
    ARG_CHECK(scan_key32 != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4) == 0);
    ARG_CHECK(spend_pubkey != NULL);
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
    ret = secp256k1_pubkey_load(ctx, &spend_pubkey_ge, spend_pubkey);
    if (!ret) {
        secp256k1_scalar_clear(&scan_key_scalar);
        return 0;
    }
    /* Creating the shared secret requires that the public and secret components are
     * non-infinity and non-zero, respectively. Note that the involved parts (input hash,
     * scan secret key, and prevouts public key sum) have all been verified at this point,
     * assuming that the user hasn't tampered the `prevouts_summary` object manually. */
    secp256k1_silentpayments_create_shared_secret(ctx, shared_secret, &prevouts_pubkey_sum_ge, &scan_key_scalar);
    /* Clear the scan_key_scalar since we no longer need it and leaking this value would break indistinguishability of the transaction. */
    secp256k1_scalar_clear(&scan_key_scalar);

    found_idx = 0;
    /* Don't look further than the per-group recipient limit, in order to avoid quadratic scaling issues. */
    k_max = (n_tx_outputs < SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT) ?
             n_tx_outputs : SECP256K1_SILENTPAYMENTS_RECIPIENT_GROUP_LIMIT;
    /* TODO: potential optimization: the worst-case run-time can be cut in half by randomizing the outputs */
    for (k = 0; k < k_max; k++) {
        secp256k1_scalar output_tweak_scalar;
        secp256k1_xonly_pubkey output_xonly;
        secp256k1_ge output_ge = spend_pubkey_ge;
        secp256k1_ge output_negated_ge;
        const unsigned char *label_tweak = NULL;
        secp256k1_ge label_ge;
        size_t j;

        /* Calculate the output_tweak and convert it to a scalar.
         *
         * Note: _create_output_tweak can only fail if the output of the hash function is zero or greater than or equal
         * to the curve order, which is statistically improbable. Returning an error here results in an untestable
         * branch in the code, but we do this anyways to ensure strict compliance with BIP0352.
         */
        if (!secp256k1_silentpayments_create_output_tweak(&output_tweak_scalar, shared_secret, k)) {
            secp256k1_scalar_clear(&output_tweak_scalar);
            secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
            return 0;
        }

        /* Calculate output = spend_pubkey + output_tweak * G.
         * This can fail if output_tweak * G is the negation of spend_pubkey, but this happens only with
         * negligible probability for honestly created spend_pubkey as output_tweak is the output of a hash function. */
        if (!secp256k1_eckey_pubkey_tweak_add(&output_ge, &output_tweak_scalar)) {
            /* Leaking these values would break indistinguishability of the transaction, so clear them. */
            secp256k1_scalar_clear(&output_tweak_scalar);
            secp256k1_memclear_explicit(&shared_secret, sizeof(shared_secret));
            return 0;
        }
        /* Calculate output_negated = -output */
        secp256k1_ge_neg(&output_negated_ge, &output_ge);

        found = 0;
        secp256k1_xonly_pubkey_save(&output_xonly, &output_ge);
        for (j = 0; j < n_tx_outputs; j++) {
            if (secp256k1_xonly_pubkey_cmp(ctx, &output_xonly, tx_outputs[j]) == 0) {
                label_tweak = NULL;
                found = 1;
                found_idx = j;
                break;
            }

            /* If not found, proceed to check for labels (if a label lookup function is provided). */
            if (label_lookup != NULL) {
                secp256k1_ge tx_output_ge;
                secp256k1_gej tx_output_gej;
                secp256k1_gej label_candidates_gej[2];
                secp256k1_ge label_candidates_ge[2];

                secp256k1_xonly_pubkey_load(ctx, &tx_output_ge, tx_outputs[j]);
                secp256k1_gej_set_ge(&tx_output_gej, &tx_output_ge);
                /* Calculate scan label candidates:
                 *     label_candidate1 =  tx_output - generated_output
                 *     label_candidate2 = -tx_output - generated_output
                 */
                secp256k1_gej_add_ge_var(&label_candidates_gej[0], &tx_output_gej, &output_negated_ge, NULL);
                secp256k1_gej_neg(&tx_output_gej, &tx_output_gej);
                secp256k1_gej_add_ge_var(&label_candidates_gej[1], &tx_output_gej, &output_negated_ge, NULL);
                secp256k1_ge_set_all_gej_var(label_candidates_ge, label_candidates_gej, 2);

                /* Check if either of the label candidates is in the label cache */
                for (i = 0; i < 2; i++) {
                    unsigned char label33[33];
                    /* Note: serialize will only fail if label_ge is the point at infinity, but we know
                     * this cannot happen since we only hit this branch if tx_output != output_xonly.
                     * Thus, we know that label_ge = tx_output_gej + output_negated_ge cannot be the
                     * point at infinity.
                     */
                    secp256k1_eckey_pubkey_serialize33(&label_candidates_ge[i], label33);
                    label_tweak = label_lookup(label33, label_context);
                    if (label_tweak != NULL) {
                        found = 1;
                        found_idx = j;
                        label_ge = label_candidates_ge[i];
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
        }
        if (found) {
            found_outputs[k]->output = *tx_outputs[found_idx];
            secp256k1_scalar_get_b32(found_outputs[k]->tweak, &output_tweak_scalar);
            /* Clear the output_tweak_scalar since we no longer need it and leaking this value would
             * break indistinguishability of the transaction. */
            secp256k1_scalar_clear(&output_tweak_scalar);
            if (label_tweak != NULL) {
                found_outputs[k]->found_with_label = 1;
                /* This is extremely unlikely to fail in that it can only really fail if label_tweak
                 * is the negation of the shared secret tweak. But since both tweak and label_tweak are
                 * created by hashing data, practically speaking this would only happen if an attacker
                 * tricked us into using a particular label_tweak (deviating from the protocol).
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
            secp256k1_scalar_clear(&output_tweak_scalar);
            break;
        }
    }
    *n_found_outputs = k;

    /* Leaking the shared_secret would break indistinguishability of the transaction, so clear it. */
    secp256k1_memclear_explicit(shared_secret, sizeof(shared_secret));
    return 1;
}

#endif
