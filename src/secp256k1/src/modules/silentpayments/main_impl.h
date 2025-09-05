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

/** Sort an array of silent payment recipients. This is used to group recipients by scan pubkey to
 *  ensure the correct values of k are used when creating multiple outputs for a recipient.
 */
static int secp256k1_silentpayments_recipient_sort_cmp(const void* pk1, const void* pk2, void *ctx) {
    return secp256k1_ec_pubkey_cmp((secp256k1_context *)ctx,
        &(*(const secp256k1_silentpayments_recipient **)pk1)->scan_pubkey,
        &(*(const secp256k1_silentpayments_recipient **)pk2)->scan_pubkey
    );
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

static void secp256k1_silentpayments_calculate_input_hash(unsigned char *input_hash, const unsigned char *outpoint_smallest36, secp256k1_ge *pubkey_sum) {
    secp256k1_sha256 hash;
    unsigned char pubkey_sum_ser[33];
    size_t len;
    int ret;

    secp256k1_silentpayments_sha256_init_inputs(&hash);
    secp256k1_sha256_write(&hash, outpoint_smallest36, 36);
    ret = secp256k1_eckey_pubkey_serialize(pubkey_sum, pubkey_sum_ser, &len, 1);
#ifdef VERIFIY
    VERIFY_CHECK(ret && len == sizeof(pubkey_sum_ser));
#else
    (void)ret;
#endif
    secp256k1_sha256_write(&hash, pubkey_sum_ser, sizeof(pubkey_sum_ser));
    secp256k1_sha256_finalize(&hash, input_hash);
}

static void secp256k1_silentpayments_create_shared_secret(const secp256k1_context *ctx, unsigned char *shared_secret33, const secp256k1_ge *public_component, const secp256k1_scalar *secret_component) {
    secp256k1_gej ss_j;
    secp256k1_ge ss;
    size_t len;
    int ret;

    secp256k1_ecmult_const(&ss_j, public_component, secret_component);
    secp256k1_ge_set_gej(&ss, &ss_j);
    secp256k1_declassify(ctx, &ss, sizeof(ss));
    /* This can only fail if the shared secret is the point at infinity, which should be
     * impossible at this point considering we have already validated the public key and
     * the secret key.
     */
    ret = secp256k1_eckey_pubkey_serialize(&ss, shared_secret33, &len, 1);
#ifdef VERIFY
    VERIFY_CHECK(ret && len == 33);
#else
    (void)ret;
#endif

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

static void secp256k1_silentpayments_create_t_k(secp256k1_scalar *t_k_scalar, const unsigned char *shared_secret33, uint32_t k) {
    secp256k1_sha256 hash;
    unsigned char hash_ser[32];
    unsigned char k_serialized[4];
    int overflow = 0;

    /* Compute hash(shared_secret || ser_32(k))  [sha256 with tag "BIP0352/SharedSecret"] */
    secp256k1_silentpayments_sha256_init_sharedsecret(&hash);
    secp256k1_sha256_write(&hash, shared_secret33, 33);
    secp256k1_write_be32(k_serialized, k);
    secp256k1_sha256_write(&hash, k_serialized, sizeof(k_serialized));
    secp256k1_sha256_finalize(&hash, hash_ser);
    secp256k1_scalar_set_b32(t_k_scalar, hash_ser, &overflow);
    VERIFY_CHECK(!overflow);
    VERIFY_CHECK(!secp256k1_scalar_is_zero(t_k_scalar));
    /* Leaking this value would break indistinguishability of the transaction, so clear it. */
    secp256k1_memclear(hash_ser, sizeof(hash_ser));
    secp256k1_sha256_clear(&hash);
}

static int secp256k1_silentpayments_create_output_pubkey(const secp256k1_context *ctx, secp256k1_xonly_pubkey *output_xonly, const unsigned char *shared_secret33, const secp256k1_pubkey *recipient_spend_pubkey, uint32_t k) {
    secp256k1_ge output_ge;
    secp256k1_scalar t_k_scalar;
    secp256k1_silentpayments_create_t_k(&t_k_scalar, shared_secret33, k);
    if (!secp256k1_pubkey_load(ctx, &output_ge, recipient_spend_pubkey)) {
        secp256k1_scalar_clear(&t_k_scalar);
        return 0;
    }
    /* `tweak_add` only fails if t_k_scalar*G = -recipient_spend_pubkey. Considering t_k is the output of a hash function,
     * this will happen only with negligible probability for honestly created recipent_spend_pubkey, but we handle this
     * error anyway to protect against this function being called with a malicious inputs, i.e., recipent_spend_pubkey = -(_create_t_k(shared_secret33, k))*G
     */
    if (!secp256k1_eckey_pubkey_tweak_add(&output_ge, &t_k_scalar)) {
        secp256k1_scalar_clear(&t_k_scalar);
        return 0;
    };
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
    const secp256k1_keypair * const *taproot_seckeys,
    size_t n_taproot_seckeys,
    const unsigned char * const *plain_seckeys,
    size_t n_plain_seckeys
) {
    size_t i, k;
    secp256k1_scalar seckey_sum_scalar, addend, input_hash_scalar;
    secp256k1_ge input_pubkey_sum_ge;
    secp256k1_gej input_pubkey_sum_gej;
    unsigned char input_hash[32];
    unsigned char shared_secret[33];
    secp256k1_pubkey current_scan_pubkey;
    int overflow = 0;
    int ret, sum_is_zero;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(generated_outputs != NULL);
    ARG_CHECK(recipients != NULL);
    ARG_CHECK(n_recipients > 0);
    ARG_CHECK((plain_seckeys != NULL) || (taproot_seckeys != NULL));
    if (taproot_seckeys != NULL) {
        ARG_CHECK(n_taproot_seckeys > 0);
    } else {
        ARG_CHECK(n_taproot_seckeys == 0);
    }
    if (plain_seckeys != NULL) {
        ARG_CHECK(n_plain_seckeys > 0);
    } else {
        ARG_CHECK(n_plain_seckeys == 0);
    }
    ARG_CHECK(outpoint_smallest36 != NULL);
    for (i = 0; i < n_recipients; i++) {
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
    /* Private keys used for taproot outputs have to be negated if they result in an odd point. This is to ensure
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
        secp256k1_declassify(ctx, &ret, sizeof(ret));
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
    secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &input_pubkey_sum_gej, &seckey_sum_scalar);
    secp256k1_ge_set_gej(&input_pubkey_sum_ge, &input_pubkey_sum_gej);
    /* We declassify the pubkey sum because serializing a group element (done in the
     * `_calculate_input_hash` call following) is not a constant-time operation.
     */
    secp256k1_declassify(ctx, &input_pubkey_sum_ge, sizeof(input_pubkey_sum_ge));

    /* Calculate the input_hash and convert it to a scalar so that it can be multiplied with the summed up private keys, i.e., a_sum = a_sum * input_hash.
     * By multiplying the scalars together first, we can save an elliptic curve multiplication.
     *
     * This should fail if input hash is greater than the curve order, but this happens only
     * with negligible probability, so we only do a VERIFY_CHECK here.
     */
    secp256k1_silentpayments_calculate_input_hash(input_hash, outpoint_smallest36, &input_pubkey_sum_ge);
    secp256k1_scalar_set_b32(&input_hash_scalar, input_hash, &overflow);
    VERIFY_CHECK(!overflow);
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
                secp256k1_memclear(&shared_secret, sizeof(shared_secret));
                return 0;
            }
            secp256k1_silentpayments_create_shared_secret(ctx, shared_secret, &pk, &seckey_sum_scalar);
            k = 0;
        }
        if (!secp256k1_silentpayments_create_output_pubkey(ctx, generated_outputs[recipients[i]->index], shared_secret, &recipients[i]->spend_pubkey, k)) {
            secp256k1_scalar_clear(&seckey_sum_scalar);
            secp256k1_memclear(&shared_secret, sizeof(shared_secret));
            return 0;
        }
        VERIFY_CHECK(k < SIZE_MAX);
        k++;
        current_scan_pubkey = recipients[i]->scan_pubkey;
    }
    secp256k1_scalar_clear(&seckey_sum_scalar);
    secp256k1_memclear(&shared_secret, sizeof(shared_secret));
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

int secp256k1_silentpayments_recipient_create_label(const secp256k1_context *ctx, secp256k1_pubkey *label, unsigned char *label_tweak32, const unsigned char *recipient_scan_key32, const uint32_t m) {
    secp256k1_sha256 hash;
    unsigned char m_serialized[4];

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(label != NULL);
    ARG_CHECK(label_tweak32 != NULL);
    ARG_CHECK(recipient_scan_key32 != NULL);

    /* Compute hash(ser_256(b_scan) || ser_32(m))  [sha256 with tag "BIP0352/Label"] */
    secp256k1_silentpayments_sha256_init_label(&hash);
    secp256k1_sha256_write(&hash, recipient_scan_key32, 32);
    secp256k1_write_be32(m_serialized, m);
    secp256k1_sha256_write(&hash, m_serialized, sizeof(m_serialized));
    secp256k1_sha256_finalize(&hash, label_tweak32);

    secp256k1_memclear(m_serialized, sizeof(m_serialized));
    secp256k1_sha256_clear(&hash);
    return secp256k1_ec_pubkey_create(ctx, label, label_tweak32);
}

int secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(const secp256k1_context *ctx, secp256k1_pubkey *labeled_spend_pubkey, const secp256k1_pubkey *recipient_spend_pubkey, const secp256k1_pubkey *label) {
    secp256k1_ge labeled_spend_pubkey_ge, label_addend;
    secp256k1_gej result_gej;
    secp256k1_ge result_ge;
    int ret;

    /* Sanity check inputs. */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(labeled_spend_pubkey != NULL);
    ARG_CHECK(recipient_spend_pubkey != NULL);
    ARG_CHECK(label != NULL);

    /* Calculate labeled_spend_pubkey = recipient_spend_pubkey + label.
     * If either the label or spend public key is an invalid public key,
     * return early
     */
    ret = secp256k1_pubkey_load(ctx, &labeled_spend_pubkey_ge, recipient_spend_pubkey);
    ret &= secp256k1_pubkey_load(ctx, &label_addend, label);
    if (!ret) {
        return ret;
    }
    secp256k1_gej_set_ge(&result_gej, &labeled_spend_pubkey_ge);
    secp256k1_gej_add_ge_var(&result_gej, &result_gej, &label_addend, NULL);
    if (secp256k1_gej_is_infinity(&result_gej)) {
        return 0;
    }

    secp256k1_ge_set_gej(&result_ge, &result_gej);
    secp256k1_pubkey_save(labeled_spend_pubkey, &result_ge);

    return 1;
}

/** A explanation of the prevouts_summary object and its usage:
 *
 *  The prevouts_summary object contains:
 *
 *  [magic: 4 bytes][boolean: 1 byte][input_pubkey_sum: 64 bytes][input_hash: 32 bytes]
 *
 *  The magic bytes are checked by functions using the prevouts_summary object to
 *  check that the public data object was initialized correctly.
 *
 *  The boolean (combined) indicates whether or not the summed prevout public keys and the
 *  input_hash scalar have already been combined or are both included. The reason
 *  for keeping input_hash and the summed prevout public keys separate is so that an elliptic
 *  curve multiplication can be avoided when creating the shared secret, i.e.,
 *  (recipient_scan_key * input_hash) * prevout_pubkey_sum.
 *
 *  But when storing the public data object, either to send to light clients or for
 *  wallet rescans, we can save 32-bytes by combining the input_hash and prevout_pubkey_sum and saving
 *  the resulting point serialized as a compressed public key, i.e., input_hash * prevout_pubkey_sum.
 *
 *  For the each function:
 *
 *  - `_recipient_prevouts_summary_create` always creates a prevouts_summary object with combined = false
 *  - `_recipient_prevouts_summary_serialize` only accepts a prevouts_summary object with combined = false
 *    and then performs an EC mult before serializing the resulting public key as a compressed
 *    public key
 *  - `_recipient_prevouts_summary_parse` assumes the input represents a previously serialized
 *    prevouts_summary object and always deserializes into a prevouts_summary object with combined = true
 *    (and the input_hash portion zeroed out).
 */

int secp256k1_silentpayments_recipient_prevouts_summary_create(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary,
    const unsigned char *outpoint_smallest36,
    const secp256k1_xonly_pubkey * const *xonly_pubkeys,
    size_t n_xonly_pubkeys,
    const secp256k1_pubkey * const *plain_pubkeys,
    size_t n_plain_pubkeys
) {
    size_t i;
    size_t pubkeylen = 64;
    secp256k1_ge input_pubkey_sum_ge, addend;
    secp256k1_gej input_pubkey_sum_gej;
    secp256k1_scalar input_hash_scalar;
    unsigned char input_hash_local[32];
    int overflow = 0;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(outpoint_smallest36 != NULL);
    ARG_CHECK((plain_pubkeys != NULL) || (xonly_pubkeys != NULL));
    if (xonly_pubkeys != NULL) {
        ARG_CHECK(n_xonly_pubkeys > 0);
    } else {
        ARG_CHECK(n_xonly_pubkeys == 0);
    }
    if (plain_pubkeys != NULL) {
        ARG_CHECK(n_plain_pubkeys > 0);
    } else {
        ARG_CHECK(n_plain_pubkeys == 0);
    }

    /* Compute input public keys sum: input_pubkey_sum = A_1 + A_2 + ... + A_n.
     *
     * Since an attacker can maliciously craft transactions where the public keys sum to zero, fail early here
     * to avoid making the caller do extra work, e.g., when building an index or scanning a malicious transaction.
     *
     * This will also fail if any of the provided input public keys are malformed.
     */
    secp256k1_gej_set_infinity(&input_pubkey_sum_gej);
    for (i = 0; i < n_plain_pubkeys; i++) {
        if (!secp256k1_pubkey_load(ctx, &addend, plain_pubkeys[i])) {
            return 0;
        }
        secp256k1_gej_add_ge_var(&input_pubkey_sum_gej, &input_pubkey_sum_gej, &addend, NULL);
    }
    for (i = 0; i < n_xonly_pubkeys; i++) {
        if (!secp256k1_xonly_pubkey_load(ctx, &addend, xonly_pubkeys[i])) {
            return 0;
        }
        secp256k1_gej_add_ge_var(&input_pubkey_sum_gej, &input_pubkey_sum_gej, &addend, NULL);
    }
    if (secp256k1_gej_is_infinity(&input_pubkey_sum_gej)) {
        return 0;
    }
    secp256k1_ge_set_gej(&input_pubkey_sum_ge, &input_pubkey_sum_gej);
    secp256k1_silentpayments_calculate_input_hash(input_hash_local, outpoint_smallest36, &input_pubkey_sum_ge);
    /* Convert input_hash to a scalar to ensure the value is less than the curve order.
     *
     * This can only fail if the output of the hash function is greater than the curve order, which
     * happens with negligible probability. We use a VERIFY_CHECK as opposed to returning an error,
     * since returning an error here would result in an untestable branch in the code.
     */
    secp256k1_scalar_set_b32(&input_hash_scalar, input_hash_local, &overflow);
    VERIFY_CHECK(!overflow);
    memcpy(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4);
    prevouts_summary->data[4] = 0;
    secp256k1_ge_to_bytes(&prevouts_summary->data[5], &input_pubkey_sum_ge);
    memcpy(&prevouts_summary->data[5 + pubkeylen], input_hash_local, 32);
    return 1;
}

int secp256k1_silentpayments_recipient_prevouts_summary_serialize(const secp256k1_context *ctx, unsigned char *output33, const secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary) {
    secp256k1_ge ge;
    size_t pubkeylen = 33;
    int ret, combined;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output33 != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4) == 0);
    /* These functions should never fail at this point considering:
     *   - loading the pubkey and input hash can only fail if the public data object was created incorrectly
     *     and we already check for this above.
     *   - `_tweak_mul` can only fail if input_hash_scalar is zero, but assuming the prevouts_summary object
     *     was created correctly, this is impossible because input_hash_scalar is the output of a hash function.
     *   - `_eckey_pubkey_serialize` can only fail if the point we are trying to serialize is the point at infinity.
     *
     *   Note: we don't verify that the input hash is less than the curve order since this is verified when the
     *   prevouts_summary object is created.
     */
    secp256k1_ge_from_bytes(&ge, &prevouts_summary->data[5]);
    combined = (int)prevouts_summary->data[4];
    ret = 1;
    if (!combined) {
        secp256k1_scalar input_hash_scalar;
        secp256k1_scalar_set_b32(&input_hash_scalar, &prevouts_summary->data[5 + 64], NULL);
        ret &= secp256k1_eckey_pubkey_tweak_mul(&ge, &input_hash_scalar);
    }
    ret &= secp256k1_eckey_pubkey_serialize(&ge, output33, &pubkeylen, 1);
#ifdef VERIFY
    VERIFY_CHECK(ret && pubkeylen == 33);
#else
    (void)ret;
#endif
    return 1;
}

int secp256k1_silentpayments_recipient_prevouts_summary_parse(const secp256k1_context *ctx, secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary, const unsigned char *input33) {
    size_t inputlen = 33;
    secp256k1_ge pk;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(input33 != NULL);
    /* Since an attacker can send us malicious data that looks like a serialized public key but is not, fail early. */
    if (!secp256k1_eckey_pubkey_parse(&pk, input33, inputlen)) {
        return 0;
    }
    /* A serialized public data will always have have the input_hash multiplied in, so we set combined = true.
     * Additionally, we zero out the 32 bytes used to represent the input_hash.
     */
    memcpy(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4);
    prevouts_summary->data[4] = 1;
    secp256k1_ge_to_bytes(&prevouts_summary->data[5], &pk);
    memset(&prevouts_summary->data[5 + 64], 0, 32);
    return 1;
}

int secp256k1_silentpayments_recipient_scan_outputs(
    const secp256k1_context *ctx,
    secp256k1_silentpayments_found_output **found_outputs, size_t *n_found_outputs,
    const secp256k1_xonly_pubkey * const *tx_outputs, size_t n_tx_outputs,
    const unsigned char *recipient_scan_key32,
    const secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary,
    const secp256k1_pubkey *recipient_spend_pubkey,
    const secp256k1_silentpayments_label_lookup label_lookup,
    const void *label_context
) {
    secp256k1_scalar t_k_scalar, rsk_scalar;
    secp256k1_ge label_ge, recipient_spend_pubkey_ge, input_pubkey_sum_ge;
    secp256k1_xonly_pubkey output_xonly;
    unsigned char shared_secret[33];
    const unsigned char *label_tweak = NULL;
    size_t i, k, n_found, found_idx;
    int found, combined;
    int ret = 1;

    /* Sanity check inputs */
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(found_outputs != NULL);
    ARG_CHECK(n_found_outputs != NULL);
    ARG_CHECK(tx_outputs != NULL);
    ARG_CHECK(n_tx_outputs > 0);
    ARG_CHECK(recipient_scan_key32 != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4) == 0);
    ARG_CHECK(recipient_spend_pubkey != NULL);
    /* Passing a context without a lookup function is non-sensical */
    if (label_context != NULL) {
        ARG_CHECK(label_lookup != NULL);
    }
    ret = secp256k1_scalar_set_b32_seckey(&rsk_scalar, recipient_scan_key32);
    secp256k1_declassify(ctx, &ret, sizeof(ret));
    if (!ret) {
        /* Leaking this value would break indistinguishability of the transaction, so clear it. */
        secp256k1_scalar_clear(&rsk_scalar);
        return 0;
    }
    secp256k1_ge_from_bytes(&input_pubkey_sum_ge, &prevouts_summary->data[5]);
    combined = (int)prevouts_summary->data[4];
    if (!combined) {
        secp256k1_scalar input_hash_scalar;
        secp256k1_scalar_set_b32(&input_hash_scalar, &prevouts_summary->data[5 + 64], NULL);
        secp256k1_scalar_mul(&rsk_scalar, &rsk_scalar, &input_hash_scalar);
    }
    ret &= secp256k1_pubkey_load(ctx, &recipient_spend_pubkey_ge, recipient_spend_pubkey);
    if (!ret) {
        secp256k1_scalar_clear(&rsk_scalar);
        return 0;
    }
    secp256k1_silentpayments_create_shared_secret(ctx, shared_secret, &input_pubkey_sum_ge, &rsk_scalar);

    found_idx = 0;
    n_found = 0;
    k = 0;
    while (1) {
        secp256k1_ge output_ge = recipient_spend_pubkey_ge;
        secp256k1_silentpayments_create_t_k(&t_k_scalar, shared_secret, k);

        /* Calculate output = recipient_spend_pubkey + t_k * G.
         * This can fail if t_k is the negation of recipient_spend_pubkey, but this happens only
         * with negligible probability as t_k is the output of a hash function. */
        if (!secp256k1_eckey_pubkey_tweak_add(&output_ge, &t_k_scalar)) {
            secp256k1_scalar_clear(&rsk_scalar);
            return 0;
        }
        found = 0;
        secp256k1_xonly_pubkey_save(&output_xonly, &output_ge);
        for (i = 0; i < n_tx_outputs; i++) {
            if (secp256k1_xonly_pubkey_cmp(ctx, &output_xonly, tx_outputs[i]) == 0) {
                label_tweak = NULL;
                found = 1;
                found_idx = i;
                break;
            }

            /* If not found, proceed to check for labels (if a label lookup function is provided). */
            if (label_lookup != NULL) {
                secp256k1_ge output_negated_ge, tx_output_ge;
                secp256k1_gej tx_output_gej, label_gej;
                unsigned char label33[33];
                size_t len;

                secp256k1_xonly_pubkey_load(ctx, &tx_output_ge, tx_outputs[i]);
                secp256k1_gej_set_ge(&tx_output_gej, &tx_output_ge);
                secp256k1_ge_neg(&output_negated_ge, &output_ge);
                /* Negate the generated output and calculate first scan label candidate:
                 *     label1 = tx_output - generated_output
                 */
                secp256k1_gej_add_ge_var(&label_gej, &tx_output_gej, &output_negated_ge, NULL);
                secp256k1_ge_set_gej(&label_ge, &label_gej);
                ret = secp256k1_eckey_pubkey_serialize(&label_ge, label33, &len, 1);
                /* Serialize must succeed because the point was just loaded. */
                VERIFY_CHECK(ret && len == 33);
                label_tweak = label_lookup(label33, label_context);
                if (label_tweak != NULL) {
                    found = 1;
                    found_idx = i;
                    break;
                }

                secp256k1_gej_neg(&label_gej, &tx_output_gej);
                /* If not found, negate the tx_output and calculate second scan label candidate:
                 *     label2 = -tx_output - generated_output
                 */
                secp256k1_gej_add_ge_var(&label_gej, &label_gej, &output_negated_ge, NULL);
                secp256k1_ge_set_gej(&label_ge, &label_gej);
                ret = secp256k1_eckey_pubkey_serialize(&label_ge, label33, &len, 1);
                /* Serialize must succeed because the point was just loaded. */
                VERIFY_CHECK(ret && len == 33);
                label_tweak = label_lookup(label33, label_context);
                if (label_tweak != NULL) {
                    found = 1;
                    found_idx = i;
                    break;
                }
            }
        }
        if (found) {
            found_outputs[n_found]->output = *tx_outputs[found_idx];
            secp256k1_scalar_get_b32(found_outputs[n_found]->tweak, &t_k_scalar);
            if (label_tweak != NULL) {
                found_outputs[n_found]->found_with_label = 1;
                /* This is extremely unlikely to fail in that it can only really fail if label_tweak
                 * is the negation of the shared secret tweak. But since both tweak and label_tweak are
                 * created by hashing data, practically speaking this would only happen if an attacker
                 * tricked us into using a particular label_tweak (deviating from the protocol).
                 */
                ret = secp256k1_ec_seckey_tweak_add(ctx, found_outputs[n_found]->tweak, label_tweak);
                VERIFY_CHECK(ret);
                secp256k1_pubkey_save(&found_outputs[n_found]->label, &label_ge);
            } else {
                found_outputs[n_found]->found_with_label = 0;
                /* Set the label public key with an invalid public key value. */
                memset(&found_outputs[n_found]->label, 0, sizeof(secp256k1_pubkey));
            }
            /* Reset everything for the next round of scanning. */
            label_tweak = NULL;
            n_found++;
            k++;
        } else {
            break;
        }
    }
    *n_found_outputs = n_found;

    /* Leaking these values would break indistinguishability of the transaction, so clear them. */
    secp256k1_scalar_clear(&rsk_scalar);
    secp256k1_scalar_clear(&t_k_scalar);
    secp256k1_memclear(shared_secret, sizeof(shared_secret));
    return ret;
}

int secp256k1_silentpayments_recipient_create_shared_secret(const secp256k1_context *ctx, unsigned char *shared_secret33, const unsigned char *recipient_scan_key32, const secp256k1_silentpayments_recipient_prevouts_summary *prevouts_summary) {
    secp256k1_scalar rsk;
    secp256k1_ge input_pubkey_ge;
    int ret, combined;
    /* Sanity check inputs */
    ARG_CHECK(shared_secret33 != NULL);
    ARG_CHECK(recipient_scan_key32 != NULL);
    ARG_CHECK(prevouts_summary != NULL);
    ARG_CHECK(secp256k1_memcmp_var(&prevouts_summary->data[0], secp256k1_silentpayments_prevouts_summary_magic, 4) == 0);
    ret = secp256k1_scalar_set_b32_seckey(&rsk, recipient_scan_key32);
    /* If there are any issues with the recipient scan key, return early. */
    secp256k1_declassify(ctx, &ret, sizeof(ret));
    if (!ret) {
        return 0;
    }
    secp256k1_ge_from_bytes(&input_pubkey_ge, &prevouts_summary->data[5]);
    combined = (int)prevouts_summary->data[4];
    if (!combined) {
        secp256k1_scalar input_hash_scalar;
        secp256k1_scalar_set_b32(&input_hash_scalar, &prevouts_summary->data[5 + 64], NULL);
        secp256k1_scalar_mul(&rsk, &rsk, &input_hash_scalar);
    }
    secp256k1_silentpayments_create_shared_secret(ctx, shared_secret33, &input_pubkey_ge, &rsk);

    secp256k1_scalar_clear(&rsk);
    return 1;
}

int secp256k1_silentpayments_recipient_create_output_pubkey(const secp256k1_context *ctx, secp256k1_xonly_pubkey *output_xonly, const unsigned char *shared_secret33, const secp256k1_pubkey *recipient_spend_pubkey, const uint32_t k)
{
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output_xonly != NULL);
    ARG_CHECK(shared_secret33 != NULL);
    ARG_CHECK(recipient_spend_pubkey != NULL);
    return secp256k1_silentpayments_create_output_pubkey(ctx, output_xonly, shared_secret33, recipient_spend_pubkey, k);
}


#endif
