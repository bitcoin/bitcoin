/***********************************************************************
 * Copyright (c) 2025 Fabian Jahr                                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_FULLAGG_TESTS_IMPL_H
#define SECP256K1_MODULE_FULLAGG_TESTS_IMPL_H

#include <stdlib.h>
#include <string.h>

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_fullagg.h"

#include "../../scalar.h"
#include "../../field.h"
#include "../../group.h"
#include "../../hash.h"
#include "../../util.h"

#include "vectors.h"

static int create_keypair_and_pk_fullagg(secp256k1_keypair *keypair, secp256k1_xonly_pubkey *pk, const unsigned char *sk) {
    int ret;
    secp256k1_keypair keypair_tmp;
    ret = secp256k1_keypair_create(CTX, &keypair_tmp, sk);
    ret &= secp256k1_keypair_xonly_pub(CTX, pk, NULL, &keypair_tmp);
    if (keypair != NULL) {
        *keypair = keypair_tmp;
    }
    return ret;
}

/* Simple test with three signers */
static void fullagg_simple_test_internal(void) {
    unsigned char sk[3][32];
    secp256k1_keypair keypair[3];
    secp256k1_fullagg_pubnonce pubnonce[3];
    const secp256k1_fullagg_pubnonce *pubnonce_ptr[3];
    secp256k1_fullagg_aggnonce aggnonce;
    unsigned char msg[3][32];
    unsigned char session_secrand[3][32];
    secp256k1_fullagg_secnonce secnonce[3];
    secp256k1_xonly_pubkey pk[3];
    const secp256k1_xonly_pubkey *pk_ptr[3];
    const unsigned char *msg_ptr[3];
    secp256k1_fullagg_partial_sig partial_sig[3];
    const secp256k1_fullagg_partial_sig *partial_sig_ptr[3];
    unsigned char final_sig[64];
    secp256k1_fullagg_session session;
    int i;

    testrand256(msg[0]);
    testrand256(msg[1]);
    testrand256(msg[2]);

    for (i = 0; i < 3; i++) {
        testrand256(sk[i]);
        pk_ptr[i] = &pk[i];
        msg_ptr[i] = msg[i];
        pubnonce_ptr[i] = &pubnonce[i];
        partial_sig_ptr[i] = &partial_sig[i];

        CHECK(create_keypair_and_pk_fullagg(&keypair[i], &pk[i], sk[i]));
    }

    /* Generate nonces */
    for (i = 0; i < 3; i++) {
        testrand256(session_secrand[i]);
        CHECK(secp256k1_fullagg_nonce_gen(CTX, &secnonce[i], &pubnonce[i], session_secrand[i],
                                          sk[i], &pk[i], NULL) == 1);
    }

    /* Aggregate nonces */
    CHECK(secp256k1_fullagg_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 3) == 1);

    /* Initialize session */
    CHECK(secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                         pubnonce_ptr, 3) == 1);

    /* The signers create and verify their partial signatures */
    for (i = 0; i < 3; i++) {
        CHECK(secp256k1_fullagg_partial_sign(CTX, &partial_sig[i], &secnonce[i],
                                             &keypair[i], msg[i], &session, pk_ptr, msg_ptr,
                                             pubnonce_ptr, 3, i) == 1);
        CHECK(secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[i], pubnonce_ptr,
                                                   pk_ptr, msg_ptr, 3, i) == 1);
    }

    /* Aggregate partial signatures */
    CHECK(secp256k1_fullagg_partial_sig_agg(CTX, final_sig, &session, partial_sig_ptr, 3) == 1);

    /* Verify the aggregate signature */
    CHECK(secp256k1_fullagg_verify(CTX, final_sig, pk_ptr, msg_ptr, 3) == 1);
}

/* Test API parameter validation */
static void fullagg_api_tests(void) {
    secp256k1_fullagg_partial_sig partial_sig[2];
    const secp256k1_fullagg_partial_sig *partial_sig_ptr[2];
    unsigned char sig[64];
    unsigned char sk[2][32];
    secp256k1_keypair keypair[2];
    unsigned char zeros132[132] = { 0 };
    unsigned char session_secrand[2][32];
    secp256k1_fullagg_secnonce secnonce[2];
    secp256k1_fullagg_pubnonce pubnonce[2];
    const secp256k1_fullagg_pubnonce *pubnonce_ptr[2];
    unsigned char pubnonce_ser[66];
    secp256k1_fullagg_aggnonce aggnonce;
    unsigned char aggnonce_ser[66];
    unsigned char msg[2][32];
    const unsigned char *msg_ptr[2];
    secp256k1_fullagg_session session;
    secp256k1_xonly_pubkey pk[2];
    const secp256k1_xonly_pubkey *pk_ptr[2];
    int i;

    for (i = 0; i < 2; i++) {
        pk_ptr[i] = &pk[i];
        msg_ptr[i] = msg[i];
        pubnonce_ptr[i] = &pubnonce[i];
        partial_sig_ptr[i] = &partial_sig[i];
        testrand256(session_secrand[i]);
        testrand256(sk[i]);
        testrand256(msg[i]);
        CHECK(create_keypair_and_pk_fullagg(&keypair[i], &pk[i], sk[i]));
    }

    /* Nonce generation */
    CHECK(secp256k1_fullagg_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0],
                                      sk[0], &pk[0], NULL) == 1);
    /* Check that session_secrand is zeroed */
    CHECK(secp256k1_memcmp_var(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);

    /* session_secrand = 0 is disallowed */
    memset(session_secrand[0], 0, sizeof(session_secrand[0]));
    CHECK(secp256k1_fullagg_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0],
                                      sk[0], &pk[0], NULL) == 0);

    /* Test NULL parameters */
    testrand256(session_secrand[0]);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_gen(CTX, NULL, &pubnonce[0], session_secrand[0],
                                                   sk[0], &pk[0], NULL));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_gen(CTX, &secnonce[0], NULL, session_secrand[0],
                                                   sk[0], &pk[0], NULL));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_gen(CTX, &secnonce[0], &pubnonce[0], NULL,
                                                   sk[0], &pk[0], NULL));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0],
                                                   sk[0], NULL, NULL));

    /* Generate valid nonces for both signers */
    for (i = 0; i < 2; i++) {
        testrand256(session_secrand[i]);
        CHECK(secp256k1_fullagg_nonce_gen(CTX, &secnonce[i], &pubnonce[i], session_secrand[i],
                                          sk[i], &pk[i], NULL) == 1);
    }

    /* Serialize and parse public nonces */
    CHECK(secp256k1_fullagg_pubnonce_serialize(CTX, pubnonce_ser, &pubnonce[0]) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_pubnonce_serialize(CTX, NULL, &pubnonce[0]));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_pubnonce_serialize(CTX, pubnonce_ser, NULL));

    CHECK(secp256k1_fullagg_pubnonce_parse(CTX, &pubnonce[0], pubnonce_ser) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_pubnonce_parse(CTX, NULL, pubnonce_ser));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_pubnonce_parse(CTX, &pubnonce[0], NULL));

    /* Nonce aggregation */
    CHECK(secp256k1_fullagg_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_agg(CTX, NULL, pubnonce_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_agg(CTX, &aggnonce, NULL, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 0));

    /* Serialize and parse aggregate nonces */
    CHECK(secp256k1_fullagg_aggnonce_serialize(CTX, aggnonce_ser, &aggnonce) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_aggnonce_serialize(CTX, NULL, &aggnonce));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_aggnonce_serialize(CTX, aggnonce_ser, NULL));

    CHECK(secp256k1_fullagg_aggnonce_parse(CTX, &aggnonce, aggnonce_ser) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_aggnonce_parse(CTX, NULL, aggnonce_ser));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_aggnonce_parse(CTX, &aggnonce, NULL));

    /* Session initialization */
    CHECK(secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                         pubnonce_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_session_init(CTX, NULL, &aggnonce, pk_ptr, msg_ptr,
                                                      pubnonce_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_session_init(CTX, &session, NULL, pk_ptr, msg_ptr,
                                                      pubnonce_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_session_init(CTX, &session, &aggnonce, NULL, msg_ptr,
                                                      pubnonce_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, NULL,
                                                      pubnonce_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                                      NULL, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                                      pubnonce_ptr, 0));

    /* Partial signing requires a context with the generator context built */
    CHECK_ILLEGAL(STATIC_CTX, secp256k1_fullagg_partial_sign(STATIC_CTX, &partial_sig[0], &secnonce[0],
                                                             &keypair[0], msg[0], &session, pk_ptr, msg_ptr,
                                                             pubnonce_ptr, 2, 0));

    /* n_signers must match the number of signers in the session */
    {
        secp256k1_fullagg_secnonce secnonce_tmp;
        memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
        CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0],
                                                          msg[0], &session, pk_ptr, msg_ptr,
                                                          pubnonce_ptr, 1, 0));
    }

    /* Signing fails if the lists differ from those the session was
     * initialized with */
    {
        secp256k1_fullagg_secnonce secnonce_tmp;
        unsigned char other_msg[32];
        const unsigned char *wrong_msg_ptr[2];
        memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
        memcpy(other_msg, msg[1], 32);
        other_msg[0] ^= 1;
        wrong_msg_ptr[0] = msg[0];
        wrong_msg_ptr[1] = other_msg;
        CHECK(secp256k1_fullagg_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0],
                                             msg[0], &session, pk_ptr, wrong_msg_ptr,
                                             pubnonce_ptr, 2, 0) == 0);
    }

    /* Partial signing */
    for (i = 0; i < 2; i++) {
        secp256k1_fullagg_secnonce secnonce_tmp;
        memcpy(&secnonce_tmp, &secnonce[i], sizeof(secnonce_tmp));
        CHECK(secp256k1_fullagg_partial_sign(CTX, &partial_sig[i], &secnonce_tmp, &keypair[i],
                                             msg[i], &session, pk_ptr, msg_ptr, pubnonce_ptr, 2, i) == 1);
        /* The secnonce is set to 0 and following signing attempts fail */
        CHECK(secp256k1_memcmp_var(&secnonce_tmp, zeros132, sizeof(secnonce_tmp)) == 0);
        CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sign(CTX, &partial_sig[i], &secnonce_tmp,
                                                          &keypair[i], msg[i], &session, pk_ptr, msg_ptr,
                                                          pubnonce_ptr, 2, i));
    }

    /** Partial signature verification **/
    CHECK(secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[0], pubnonce_ptr, pk_ptr,
                                               msg_ptr, 2, 0) == 1);
    CHECK(secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[1], pubnonce_ptr, pk_ptr,
                                               msg_ptr, 2, 1) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_verify(CTX, NULL, pubnonce_ptr, pk_ptr,
                                                            msg_ptr, 2, 0));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[0], NULL, pk_ptr,
                                                            msg_ptr, 2, 0));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[0], pubnonce_ptr, NULL,
                                                            msg_ptr, 2, 0));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[0], pubnonce_ptr, pk_ptr,
                                                            NULL, 2, 0));

    /** Signature aggregation **/
    CHECK(secp256k1_fullagg_partial_sig_agg(CTX, sig, &session, partial_sig_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_agg(CTX, NULL, &session, partial_sig_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_agg(CTX, sig, NULL, partial_sig_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_agg(CTX, sig, &session, NULL, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_partial_sig_agg(CTX, sig, &session, partial_sig_ptr, 0));

    /** Verification **/
    CHECK(secp256k1_fullagg_verify(CTX, sig, pk_ptr, msg_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_verify(CTX, NULL, pk_ptr, msg_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_verify(CTX, sig, NULL, msg_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_verify(CTX, sig, pk_ptr, NULL, 2));
    CHECK_ILLEGAL(CTX, secp256k1_fullagg_verify(CTX, sig, pk_ptr, msg_ptr, 0));
}

/* Test with counter-based nonce generation */
static void fullagg_nonce_gen_counter_test(void) {
    unsigned char sk[2][32];
    secp256k1_keypair keypair[2];
    secp256k1_xonly_pubkey pk[2];
    const secp256k1_xonly_pubkey *pk_ptr[2];
    secp256k1_fullagg_pubnonce pubnonce[2];
    const secp256k1_fullagg_pubnonce *pubnonce_ptr[2];
    secp256k1_fullagg_aggnonce aggnonce;
    secp256k1_fullagg_secnonce secnonce[2];
    secp256k1_fullagg_session session;
    secp256k1_fullagg_partial_sig partial_sig[2];
    const secp256k1_fullagg_partial_sig *partial_sig_ptr[2];
    unsigned char msg[2][32];
    const unsigned char *msg_ptr[2];
    unsigned char final_sig[64];
    uint64_t nonrepeating_cnt = 0;
    int i;

    for (i = 0; i < 2; i++) {
        testrand256(sk[i]);
        testrand256(msg[i]);
        CHECK(create_keypair_and_pk_fullagg(&keypair[i], &pk[i], sk[i]));
        pk_ptr[i] = &pk[i];
        msg_ptr[i] = msg[i];
        pubnonce_ptr[i] = &pubnonce[i];
        partial_sig_ptr[i] = &partial_sig[i];
    }

    /* Generate nonces using counter */
    for (i = 0; i < 2; i++) {
        CHECK(secp256k1_fullagg_nonce_gen_counter(CTX, &secnonce[i], &pubnonce[i],
                                                  nonrepeating_cnt + i, &keypair[i],
                                                  NULL) == 1);
    }

    /* Complete the protocol */
    CHECK(secp256k1_fullagg_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2) == 1);
    CHECK(secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                         pubnonce_ptr, 2) == 1);

    for (i = 0; i < 2; i++) {
        CHECK(secp256k1_fullagg_partial_sign(CTX, &partial_sig[i], &secnonce[i], &keypair[i],
                                             msg[i], &session, pk_ptr, msg_ptr, pubnonce_ptr, 2, i) == 1);
    }

    CHECK(secp256k1_fullagg_partial_sig_agg(CTX, final_sig, &session, partial_sig_ptr, 2) == 1);
    CHECK(secp256k1_fullagg_verify(CTX, final_sig, pk_ptr, msg_ptr, 2) == 1);
}

/* Test serialization round-trip */
static void fullagg_serialization_test(void) {
    secp256k1_fullagg_pubnonce pubnonce, pubnonce2;
    secp256k1_fullagg_aggnonce aggnonce, aggnonce2;
    secp256k1_fullagg_partial_sig partial_sig, partial_sig2;
    unsigned char pubnonce_ser[66];
    unsigned char aggnonce_ser[66];
    unsigned char partial_sig_ser[32];
    unsigned char session_secrand[32];
    unsigned char sk[32];
    secp256k1_xonly_pubkey pk;
    secp256k1_fullagg_secnonce secnonce;
    secp256k1_scalar s;
    const secp256k1_fullagg_pubnonce *pubnonce_ptr;

    testrand256(sk);
    testrand256(session_secrand);
    CHECK(create_keypair_and_pk_fullagg(NULL, &pk, sk));

    /* Test pubnonce serialization */
    CHECK(secp256k1_fullagg_nonce_gen(CTX, &secnonce, &pubnonce, session_secrand,
                                      sk, &pk, NULL) == 1);
    CHECK(secp256k1_fullagg_pubnonce_serialize(CTX, pubnonce_ser, &pubnonce) == 1);
    CHECK(secp256k1_fullagg_pubnonce_parse(CTX, &pubnonce2, pubnonce_ser) == 1);
    CHECK(secp256k1_memcmp_var(&pubnonce, &pubnonce2, sizeof(pubnonce)) == 0);

    /* Test aggnonce serialization */
    pubnonce_ptr = &pubnonce;
    CHECK(secp256k1_fullagg_nonce_agg(CTX, &aggnonce, &pubnonce_ptr, 1) == 1);
    CHECK(secp256k1_fullagg_aggnonce_serialize(CTX, aggnonce_ser, &aggnonce) == 1);
    CHECK(secp256k1_fullagg_aggnonce_parse(CTX, &aggnonce2, aggnonce_ser) == 1);
    CHECK(secp256k1_memcmp_var(&aggnonce, &aggnonce2, sizeof(aggnonce)) == 0);

    /* Test partial_sig serialization */
    testutil_random_scalar_order_test(&s);
    secp256k1_fullagg_partial_sig_save(&partial_sig, &s);
    CHECK(secp256k1_fullagg_partial_sig_serialize(CTX, partial_sig_ser, &partial_sig) == 1);
    CHECK(secp256k1_fullagg_partial_sig_parse(CTX, &partial_sig2, partial_sig_ser) == 1);
    CHECK(secp256k1_memcmp_var(&partial_sig, &partial_sig2, sizeof(partial_sig)) == 0);
}

/* Writes a test vector secnonce (two secret nonce scalars) into a secnonce
 * object */
static void fullagg_test_set_secnonce(secp256k1_fullagg_secnonce *secnonce, const unsigned char *secnonce64, const secp256k1_xonly_pubkey *pubkey) {
    secp256k1_ge pk;
    secp256k1_scalar k[2];

    secp256k1_scalar_set_b32(&k[0], &secnonce64[0], NULL);
    secp256k1_scalar_set_b32(&k[1], &secnonce64[32], NULL);
    CHECK(secp256k1_xonly_pubkey_load(CTX, &pk, pubkey));
    secp256k1_fullagg_secnonce_save(secnonce, k, &pk);
}

/* Computes the pubnonce (R1_i, R2_i) = (k1_i*G, k2_i*G) corresponding to a
 * test vector secnonce */
static void fullagg_test_pubnonce_from_secnonce(secp256k1_fullagg_pubnonce *pubnonce, const unsigned char *secnonce64) {
    unsigned char pubnonce_ser[66];
    secp256k1_pubkey tmp;
    int i;

    for (i = 0; i < 2; i++) {
        size_t len = 33;
        CHECK(secp256k1_ec_pubkey_create(CTX, &tmp, &secnonce64[32*i]));
        CHECK(secp256k1_ec_pubkey_serialize(CTX, &pubnonce_ser[33*i], &len, &tmp, SECP256K1_EC_COMPRESSED));
    }
    CHECK(secp256k1_fullagg_pubnonce_parse(CTX, pubnonce, pubnonce_ser));
}

static void fullagg_test_vectors_sign(void) {
    size_t i, j;

    for (i = 0; i < ARRAY_SIZE(fullagg_sign_cases); i++) {
        const struct fullagg_sign_case *c = &fullagg_sign_cases[i];
        secp256k1_keypair keypair[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_xonly_pubkey pk[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_xonly_pubkey *pk_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        const unsigned char *msg_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_pubnonce pubnonce[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_fullagg_pubnonce *pubnonce_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_partial_sig partial_sig[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_fullagg_partial_sig *partial_sig_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_aggnonce aggnonce;
        unsigned char aggnonce_ser[66];
        secp256k1_fullagg_session session;
        unsigned char sig[64];

        for (j = 0; j < c->n_signers; j++) {
            CHECK(create_keypair_and_pk_fullagg(&keypair[j], &pk[j], c->sks[j]));
            fullagg_test_pubnonce_from_secnonce(&pubnonce[j], c->secnonces[j]);
            pk_ptr[j] = &pk[j];
            msg_ptr[j] = c->msgs[j];
            pubnonce_ptr[j] = &pubnonce[j];
            partial_sig_ptr[j] = &partial_sig[j];
        }

        CHECK(secp256k1_fullagg_nonce_agg(CTX, &aggnonce, pubnonce_ptr, c->n_signers));
        CHECK(secp256k1_fullagg_aggnonce_serialize(CTX, aggnonce_ser, &aggnonce));
        CHECK(secp256k1_memcmp_var(aggnonce_ser, c->expected_aggnonce, sizeof(aggnonce_ser)) == 0);

        CHECK(secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                             pubnonce_ptr, c->n_signers));

        for (j = 0; j < c->n_signers; j++) {
            unsigned char psig_ser[32];
            secp256k1_fullagg_secnonce secnonce;

            fullagg_test_set_secnonce(&secnonce, c->secnonces[j], &pk[j]);
            CHECK(secp256k1_fullagg_partial_sign(CTX, &partial_sig[j], &secnonce, &keypair[j],
                                                 c->msgs[j], &session, pk_ptr, msg_ptr, pubnonce_ptr,
                                                 c->n_signers, j));
            CHECK(secp256k1_fullagg_partial_sig_serialize(CTX, psig_ser, &partial_sig[j]));
            CHECK(secp256k1_memcmp_var(psig_ser, c->expected_psigs[j], sizeof(psig_ser)) == 0);
            CHECK(secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig[j], pubnonce_ptr, pk_ptr,
                                                       msg_ptr, c->n_signers, j));
        }

        CHECK(secp256k1_fullagg_partial_sig_agg(CTX, sig, &session, partial_sig_ptr, c->n_signers));
        CHECK(secp256k1_memcmp_var(sig, c->expected_sig, sizeof(sig)) == 0);
        CHECK(secp256k1_fullagg_verify(CTX, sig, pk_ptr, msg_ptr, c->n_signers));
    }
}

static void fullagg_test_vectors_sign_error(void) {
    size_t i, j;

    for (i = 0; i < ARRAY_SIZE(fullagg_sign_error_cases); i++) {
        const struct fullagg_sign_error_case *c = &fullagg_sign_error_cases[i];
        secp256k1_keypair keypair;
        secp256k1_xonly_pubkey signer_pk;
        secp256k1_xonly_pubkey pk[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_xonly_pubkey *pk_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        const unsigned char *msg_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_pubnonce pubnonce[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_fullagg_pubnonce *pubnonce_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_aggnonce aggnonce;
        secp256k1_fullagg_session session;
        secp256k1_fullagg_secnonce secnonce;
        secp256k1_fullagg_partial_sig partial_sig;

        CHECK(create_keypair_and_pk_fullagg(&keypair, &signer_pk, c->sk));
        for (j = 0; j < c->n_signers; j++) {
            CHECK(secp256k1_xonly_pubkey_parse(CTX, &pk[j], c->pubkeys[j]));
            CHECK(secp256k1_fullagg_pubnonce_parse(CTX, &pubnonce[j], c->pubnonces[j]));
            pk_ptr[j] = &pk[j];
            msg_ptr[j] = c->msgs[j];
            pubnonce_ptr[j] = &pubnonce[j];
        }

        CHECK(secp256k1_fullagg_nonce_agg(CTX, &aggnonce, pubnonce_ptr, c->n_signers));
        CHECK(secp256k1_fullagg_session_init(CTX, &session, &aggnonce, pk_ptr, msg_ptr,
                                             pubnonce_ptr, c->n_signers));
        /* The signer's slot is index 0 in all sign error vectors. Signing
         * must fail. */
        fullagg_test_set_secnonce(&secnonce, c->secnonce, &signer_pk);
        CHECK(secp256k1_fullagg_partial_sign(CTX, &partial_sig, &secnonce, &keypair,
                                             c->msg, &session, pk_ptr, msg_ptr, pubnonce_ptr,
                                             c->n_signers, 0) == 0);
    }
}

static void fullagg_test_vectors_verify(void) {
    size_t i, j;

    for (i = 0; i < ARRAY_SIZE(fullagg_verify_cases); i++) {
        const struct fullagg_verify_case *c = &fullagg_verify_cases[i];
        secp256k1_xonly_pubkey pk[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_xonly_pubkey *pk_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        const unsigned char *msg_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        int parse_ok = 1;

        for (j = 0; j < c->n_signers; j++) {
            parse_ok &= secp256k1_xonly_pubkey_parse(CTX, &pk[j], c->pubkeys[j]);
            pk_ptr[j] = &pk[j];
            msg_ptr[j] = c->msgs[j];
        }
        /* A public key that is not on the curve fails to parse */
        if (!parse_ok) {
            CHECK(!c->expected);
            continue;
        }
        CHECK(secp256k1_fullagg_verify(CTX, c->sig, pk_ptr, msg_ptr, c->n_signers) == c->expected);
    }
}

static void fullagg_test_vectors_partial_sig_verify(void) {
    size_t i, j;

    for (i = 0; i < ARRAY_SIZE(fullagg_partial_sig_verify_cases); i++) {
        const struct fullagg_partial_sig_verify_case *c = &fullagg_partial_sig_verify_cases[i];
        secp256k1_xonly_pubkey pk[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_xonly_pubkey *pk_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        const unsigned char *msg_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_pubnonce pubnonce[FULLAGG_VECTORS_MAX_SIGNERS];
        const secp256k1_fullagg_pubnonce *pubnonce_ptr[FULLAGG_VECTORS_MAX_SIGNERS];
        secp256k1_fullagg_partial_sig partial_sig;

        for (j = 0; j < c->n_signers; j++) {
            CHECK(secp256k1_xonly_pubkey_parse(CTX, &pk[j], c->pubkeys[j]));
            CHECK(secp256k1_fullagg_pubnonce_parse(CTX, &pubnonce[j], c->pubnonces[j]));
            pk_ptr[j] = &pk[j];
            msg_ptr[j] = c->msgs[j];
            pubnonce_ptr[j] = &pubnonce[j];
        }

        /* A partial signature that is not less than the group order fails to
         * parse */
        if (!secp256k1_fullagg_partial_sig_parse(CTX, &partial_sig, c->psig)) {
            CHECK(!c->expected);
            continue;
        }
        CHECK(secp256k1_fullagg_partial_sig_verify(CTX, &partial_sig, pubnonce_ptr, pk_ptr,
                                                   msg_ptr, c->n_signers, c->signer_index) == c->expected);
    }
}

static void fullagg_test_vectors_tweak(void) {
    size_t i;

    for (i = 0; i < ARRAY_SIZE(fullagg_tweak_cases); i++) {
        const struct fullagg_tweak_case *c = &fullagg_tweak_cases[i];
        unsigned char sk[32];
        unsigned char pk_ser[33];
        size_t len = 33;
        secp256k1_pubkey pk;

        if (c->is_xonly) {
            secp256k1_keypair keypair;
            CHECK(secp256k1_keypair_create(CTX, &keypair, c->sk));
            CHECK(secp256k1_keypair_xonly_tweak_add(CTX, &keypair, c->tweak) == c->expected);
            if (!c->expected) {
                continue;
            }
            CHECK(secp256k1_keypair_sec(CTX, sk, &keypair));
            CHECK(secp256k1_keypair_pub(CTX, &pk, &keypair));
        } else {
            memcpy(sk, c->sk, sizeof(sk));
            CHECK(secp256k1_ec_seckey_tweak_add(CTX, sk, c->tweak) == c->expected);
            if (!c->expected) {
                continue;
            }
            CHECK(secp256k1_ec_pubkey_create(CTX, &pk, sk));
        }
        CHECK(secp256k1_memcmp_var(sk, c->expected_sk, sizeof(sk)) == 0);
        CHECK(secp256k1_ec_pubkey_serialize(CTX, pk_ser, &len, &pk, SECP256K1_EC_COMPRESSED));
        CHECK(secp256k1_memcmp_var(pk_ser, c->expected_pk, sizeof(pk_ser)) == 0);
    }
}

/* --- Test registry --- */
REPEAT_TEST(fullagg_simple_test)

static const struct tf_test_entry tests_fullagg[] = {
    CASE1(fullagg_simple_test),
    CASE1(fullagg_api_tests),
    CASE1(fullagg_nonce_gen_counter_test),
    CASE1(fullagg_serialization_test),
    CASE1(fullagg_test_vectors_sign),
    CASE1(fullagg_test_vectors_sign_error),
    CASE1(fullagg_test_vectors_verify),
    CASE1(fullagg_test_vectors_partial_sig_verify),
    CASE1(fullagg_test_vectors_tweak),
};

#endif /* SECP256K1_MODULE_FULLAGG_TESTS_IMPL_H */
