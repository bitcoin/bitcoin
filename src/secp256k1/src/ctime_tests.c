/***********************************************************************
 * Copyright (c) 2020 Gregory Maxwell                                  *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/secp256k1.h"
#include "assumptions.h"
#include "checkmem.h"

#if !SECP256K1_CHECKMEM_ENABLED
#  error "This tool cannot be compiled without memory-checking interface (valgrind or msan)"
#endif

#ifdef ENABLE_MODULE_ECDH
# include "../include/secp256k1_ecdh.h"
#endif

#ifdef ENABLE_MODULE_RECOVERY
# include "../include/secp256k1_recovery.h"
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
# include "../include/secp256k1_extrakeys.h"
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
#include "../include/secp256k1_schnorrsig.h"
#endif

#ifdef ENABLE_MODULE_MUSIG
#include "../include/secp256k1_musig.h"
#endif

#ifdef ENABLE_MODULE_ELLSWIFT
#include "../include/secp256k1_ellswift.h"
#endif

#ifdef ENABLE_MODULE_SILENTPAYMENTS
#include "../include/secp256k1_silentpayments.h"
#endif

static void run_tests(secp256k1_context *ctx, unsigned char *key);

int main(void) {
    secp256k1_context* ctx;
    unsigned char key[32];
    int ret, i;

    if (!SECP256K1_CHECKMEM_RUNNING()) {
        fprintf(stderr, "This test can only usefully be run inside valgrind because it was not compiled under msan.\n");
        fprintf(stderr, "Usage: libtool --mode=execute valgrind ./ctime_tests\n");
        return EXIT_FAILURE;
    }
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_DECLASSIFY);
    /** In theory, testing with a single secret input should be sufficient:
     *  If control flow depended on secrets the tool would generate an error.
     */
    for (i = 0; i < 32; i++) {
        key[i] = i + 65;
    }

    run_tests(ctx, key);

    /* Test context randomisation. Do this last because it leaves the context
     * tainted. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_context_randomize(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);

    secp256k1_context_destroy(ctx);
    return EXIT_SUCCESS;
}

static void run_tests(secp256k1_context *ctx, unsigned char *key) {
    secp256k1_ecdsa_signature signature;
    secp256k1_pubkey pubkey;
    size_t siglen = 74;
    size_t outputlen = 33;
    int i;
    int ret;
    unsigned char msg[32];
    unsigned char sig[74];
    unsigned char spubkey[33];
#ifdef ENABLE_MODULE_RECOVERY
    secp256k1_ecdsa_recoverable_signature recoverable_signature;
    int recid;
#endif
#ifdef ENABLE_MODULE_EXTRAKEYS
    secp256k1_keypair keypair;
#endif
#ifdef ENABLE_MODULE_ELLSWIFT
    unsigned char ellswift[64];
    static const unsigned char prefix[64] = {'t', 'e', 's', 't'};
#endif
#ifdef ENABLE_MODULE_SILENTPAYMENTS
    secp256k1_xonly_pubkey generated_output;
    secp256k1_xonly_pubkey *generated_outputs[1];
    secp256k1_silentpayments_recipient recipient;
    const secp256k1_silentpayments_recipient *recipients[1];
    unsigned char outpoint_smallest[36] = { 0 };
    secp256k1_keypair taproot_seckey;
    const secp256k1_keypair *taproot_seckeys[1];
    const unsigned char *plain_seckeys[1];
    secp256k1_silentpayments_found_output *found_outputs[1];
    size_t n_found_outputs;
    const secp256k1_xonly_pubkey *tx_outputs[1];
    secp256k1_silentpayments_recipient_prevouts_summary prevouts_summary;
    unsigned char label_tweak[32] = { 0 };
    secp256k1_xonly_pubkey xonly_pubkey;
    const secp256k1_xonly_pubkey *xonly_pubkeys[1];
    secp256k1_pubkey plain_pubkey;
    const secp256k1_pubkey *plain_pubkeys[1];
    unsigned char prevouts_summary_ser[33] = { 0 };
    unsigned char shared_secret[33] = { 0 };
#endif

    for (i = 0; i < 32; i++) {
        msg[i] = i + 1;
    }

    /* Test keygen. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ec_pubkey_create(ctx, &pubkey, key);
    SECP256K1_CHECKMEM_DEFINE(&pubkey, sizeof(secp256k1_pubkey));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, spubkey, &outputlen, &pubkey, SECP256K1_EC_COMPRESSED) == 1);

    /* Test signing. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ecdsa_sign(ctx, &signature, msg, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&signature, sizeof(secp256k1_ecdsa_signature));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, sig, &siglen, &signature));

#ifdef ENABLE_MODULE_ECDH
    /* Test ECDH. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ecdh(ctx, msg, &pubkey, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_RECOVERY
    /* Test signing a recoverable signature. */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ecdsa_sign_recoverable(ctx, &recoverable_signature, msg, key, NULL, NULL);
    SECP256K1_CHECKMEM_DEFINE(&recoverable_signature, sizeof(recoverable_signature));
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, sig, &recid, &recoverable_signature));
    CHECK(recid >= 0 && recid <= 3);
#endif

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ec_seckey_verify(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ec_seckey_negate(ctx, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(msg, 32);
    ret = secp256k1_ec_seckey_tweak_add(ctx, key, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(msg, 32);
    ret = secp256k1_ec_seckey_tweak_mul(ctx, key, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* Test keypair_create and keypair_xonly_tweak_add. */
#ifdef ENABLE_MODULE_EXTRAKEYS
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_keypair_create(ctx, &keypair, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    /* The tweak is not treated as a secret in keypair_tweak_add */
    SECP256K1_CHECKMEM_DEFINE(msg, 32);
    ret = secp256k1_keypair_xonly_tweak_add(ctx, &keypair, msg);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    SECP256K1_CHECKMEM_UNDEFINE(&keypair, sizeof(keypair));
    ret = secp256k1_keypair_sec(ctx, key, &keypair);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_keypair_create(ctx, &keypair, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
    ret = secp256k1_schnorrsig_sign32(ctx, sig, msg, &keypair, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
#endif

#ifdef ENABLE_MODULE_MUSIG
    {
        secp256k1_pubkey pk;
        const secp256k1_pubkey *pk_ptr[1];
        secp256k1_xonly_pubkey agg_pk;
        unsigned char session_secrand[32];
        uint64_t nonrepeating_cnt = 0;
        secp256k1_musig_secnonce secnonce;
        secp256k1_musig_pubnonce pubnonce;
        const secp256k1_musig_pubnonce *pubnonce_ptr[1];
        secp256k1_musig_aggnonce aggnonce;
        secp256k1_musig_keyagg_cache cache;
        secp256k1_musig_session session;
        secp256k1_musig_partial_sig partial_sig;
        unsigned char extra_input[32];

        pk_ptr[0] = &pk;
        pubnonce_ptr[0] = &pubnonce;
        SECP256K1_CHECKMEM_DEFINE(key, 32);
        memcpy(session_secrand, key, sizeof(session_secrand));
        session_secrand[0] = session_secrand[0] + 1;
        memcpy(extra_input, key, sizeof(extra_input));
        extra_input[0] = extra_input[0] + 2;

        CHECK(secp256k1_keypair_create(ctx, &keypair, key));
        CHECK(secp256k1_keypair_pub(ctx, &pk, &keypair));
        CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_pk, &cache, pk_ptr, 1));

        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_UNDEFINE(session_secrand, sizeof(session_secrand));
        SECP256K1_CHECKMEM_UNDEFINE(extra_input, sizeof(extra_input));
        ret = secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_secrand, key, &pk, msg, &cache, extra_input);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        ret = secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, nonrepeating_cnt, &keypair, msg, &cache, extra_input);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptr, 1));
        /* Make sure that previous tests don't undefine msg. It's not used as a secret here. */
        SECP256K1_CHECKMEM_DEFINE(msg, sizeof(msg));
        CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg, &cache) == 1);

        ret = secp256k1_keypair_create(ctx, &keypair, key);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
        ret = secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, &keypair, &cache, &session);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
    }
#endif

#ifdef ENABLE_MODULE_ELLSWIFT
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ellswift_create(ctx, ellswift, key, NULL);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_ellswift_create(ctx, ellswift, key, ellswift);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    for (i = 0; i < 2; i++) {
        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_DEFINE(&ellswift, sizeof(ellswift));
        ret = secp256k1_ellswift_xdh(ctx, msg, ellswift, ellswift, key, i, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);

        SECP256K1_CHECKMEM_UNDEFINE(key, 32);
        SECP256K1_CHECKMEM_DEFINE(&ellswift, sizeof(ellswift));
        ret = secp256k1_ellswift_xdh(ctx, msg, ellswift, ellswift, key, i, secp256k1_ellswift_xdh_hash_function_prefix, (void *)prefix);
        SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
        CHECK(ret == 1);
    }

#endif

#ifdef ENABLE_MODULE_SILENTPAYMENTS
    SECP256K1_CHECKMEM_DEFINE(key, 32);

    generated_outputs[0] = &generated_output;

    /* Initialize recipient */
    CHECK(secp256k1_ec_pubkey_create(ctx, &recipient.scan_pubkey, key));
    key[31] ^= 1;
    CHECK(secp256k1_ec_pubkey_create(ctx, &recipient.spend_pubkey, key));
    key[31] ^= (1 << 1);
    recipient.index = 0;
    recipients[0] = &recipient;

    /* Set up secret keys */
    SECP256K1_CHECKMEM_UNDEFINE(key, 32);
    ret = secp256k1_keypair_create(ctx, &taproot_seckey, key);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret);
    key[31] ^= (1 << 2);
    taproot_seckeys[0] = &taproot_seckey;
    plain_seckeys[0] = key;

    ret = secp256k1_silentpayments_sender_create_outputs(ctx, generated_outputs, recipients, 1, outpoint_smallest, taproot_seckeys, 1, plain_seckeys, 1);
    CHECK(ret == 1);

    ret = secp256k1_silentpayments_recipient_create_label(ctx, &recipient.spend_pubkey, label_tweak, key, 0);
    key[31] ^= (1 << 3);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);

    CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly_pubkey, NULL, &taproot_seckey));
    SECP256K1_CHECKMEM_DEFINE(&xonly_pubkey, sizeof(xonly_pubkey));
    xonly_pubkeys[0] = &xonly_pubkey;
    ret = secp256k1_ec_pubkey_create(ctx, &plain_pubkey, plain_seckeys[0]);
    SECP256K1_CHECKMEM_DEFINE(&ret, sizeof(ret));
    CHECK(ret == 1);
    SECP256K1_CHECKMEM_DEFINE(&plain_pubkey, sizeof(plain_pubkey));
    plain_pubkeys[0] = &plain_pubkey;

    ret = secp256k1_silentpayments_recipient_prevouts_summary_create(ctx, &prevouts_summary, outpoint_smallest, xonly_pubkeys, 1, plain_pubkeys, 1);
    CHECK(ret == 1);

    tx_outputs[0] = generated_outputs[0];
    n_found_outputs = 1;
    SECP256K1_CHECKMEM_DEFINE(&recipient.spend_pubkey, sizeof(recipient.spend_pubkey));
    /* It is sufficient to check _recipient_scan_outputs without a label lookup function, since the shared secret is created once (which is where the constant timeness matters)
     * and then reused for the rest of the scanning logic.
     */
    CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx, found_outputs, &n_found_outputs, tx_outputs, 1, key, &prevouts_summary, &recipient.spend_pubkey, NULL, NULL));
    /* Serialise and then deserialise the public data object before using in _recipient_create_shared_secret.
     * This is necessary since _recipent_created_shared_secret expects a public data object that was deserialised from
     * its serialised format, i.e., a compressed public key.
     */
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_serialize(ctx, prevouts_summary_ser, &prevouts_summary));
    CHECK(secp256k1_silentpayments_recipient_prevouts_summary_parse(ctx, &prevouts_summary, prevouts_summary_ser));
    CHECK(secp256k1_silentpayments_recipient_create_shared_secret(ctx, shared_secret, key, &prevouts_summary));
    CHECK(secp256k1_silentpayments_recipient_create_output_pubkey(ctx, &xonly_pubkey, shared_secret, &recipient.spend_pubkey, 0));

#endif
}
