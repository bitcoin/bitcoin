/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_MUSIG_TESTS_IMPL_H
#define SECP256K1_MODULE_MUSIG_TESTS_IMPL_H

#include <stdlib.h>
#include <string.h>

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_musig.h"

#include "session.h"
#include "keyagg.h"
#include "../../scalar.h"
#include "../../field.h"
#include "../../group.h"
#include "../../hash.h"
#include "../../util.h"

#include "vectors.h"

static int create_keypair_and_pk(secp256k1_keypair *keypair, secp256k1_pubkey *pk, const unsigned char *sk) {
    int ret;
    secp256k1_keypair keypair_tmp;
    ret = secp256k1_keypair_create(CTX, &keypair_tmp, sk);
    ret &= secp256k1_keypair_pub(CTX, pk, &keypair_tmp);
    if (keypair != NULL) {
        *keypair = keypair_tmp;
    }
    return ret;
}

/* Just a simple (non-tweaked) 2-of-2 MuSig aggregate, sign, verify
 * test. */
static void musig_simple_test(void) {
    unsigned char sk[2][32];
    secp256k1_keypair keypair[2];
    secp256k1_musig_pubnonce pubnonce[2];
    const secp256k1_musig_pubnonce *pubnonce_ptr[2];
    secp256k1_musig_aggnonce aggnonce;
    unsigned char msg[32];
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_musig_keyagg_cache keyagg_cache;
    unsigned char session_secrand[2][32];
    secp256k1_musig_secnonce secnonce[2];
    secp256k1_pubkey pk[2];
    const secp256k1_pubkey *pk_ptr[2];
    secp256k1_musig_partial_sig partial_sig[2];
    const secp256k1_musig_partial_sig *partial_sig_ptr[2];
    unsigned char final_sig[64];
    secp256k1_musig_session session;
    int i;

    testrand256(msg);
    for (i = 0; i < 2; i++) {
        testrand256(sk[i]);
        pk_ptr[i] = &pk[i];
        pubnonce_ptr[i] = &pubnonce[i];
        partial_sig_ptr[i] = &partial_sig[i];

        CHECK(create_keypair_and_pk(&keypair[i], &pk[i], sk[i]));
        if (i == 0) {
            testrand256(session_secrand[i]);
            CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[i], &pubnonce[i], session_secrand[i], sk[i], &pk[i], NULL, NULL, NULL) == 1);
        } else {
            uint64_t nonrepeating_cnt = 0;
            CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[i], &pubnonce[i], nonrepeating_cnt, &keypair[i], NULL, NULL, NULL) == 1);
        }
    }

    CHECK(secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, pk_ptr, 2) == 1);
    CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2) == 1);
    CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, msg, &keyagg_cache) == 1);

    for (i = 0; i < 2; i++) {
        CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig[i], &secnonce[i], &keypair[i], &keyagg_cache, &session) == 1);
        CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[i], &pubnonce[i], &pk[i], &keyagg_cache, &session) == 1);
    }

    CHECK(secp256k1_musig_partial_sig_agg(CTX, final_sig, &session, partial_sig_ptr, 2) == 1);
    CHECK(secp256k1_schnorrsig_verify(CTX, final_sig, msg, sizeof(msg), &agg_pk) == 1);
}

/* Generate two pubnonces such that both group elements of their sum (calculated
 * with secp256k1_musig_sum_pubnonces) are infinity. */
static void pubnonce_summing_to_inf(secp256k1_musig_pubnonce *pubnonce) {
    secp256k1_ge ge[2];
    int i;
    secp256k1_gej summed_pubnonces[2];
    const secp256k1_musig_pubnonce *pubnonce_ptr[2];

    testutil_random_ge_test(&ge[0]);
    testutil_random_ge_test(&ge[1]);

    for (i = 0; i < 2; i++) {
        secp256k1_musig_pubnonce_save(&pubnonce[i], ge);
        pubnonce_ptr[i] = &pubnonce[i];
        secp256k1_ge_neg(&ge[0], &ge[0]);
        secp256k1_ge_neg(&ge[1], &ge[1]);
    }

    secp256k1_musig_sum_pubnonces(CTX, summed_pubnonces, pubnonce_ptr, 2);
    CHECK(secp256k1_gej_is_infinity(&summed_pubnonces[0]));
    CHECK(secp256k1_gej_is_infinity(&summed_pubnonces[1]));
}

int memcmp_and_randomize(unsigned char *value, const unsigned char *expected, size_t len) {
    int ret;
    size_t i;
    ret = secp256k1_memcmp_var(value, expected, len);
    for (i = 0; i < len; i++) {
        value[i] = testrand_bits(8);
    }
    return ret;
}

static void musig_api_tests(void) {
    secp256k1_musig_partial_sig partial_sig[2];
    const secp256k1_musig_partial_sig *partial_sig_ptr[2];
    secp256k1_musig_partial_sig invalid_partial_sig;
    const secp256k1_musig_partial_sig *invalid_partial_sig_ptr[2];
    unsigned char pre_sig[64];
    unsigned char buf[32];
    unsigned char sk[2][32];
    secp256k1_keypair keypair[2];
    secp256k1_keypair invalid_keypair;
    unsigned char max64[64];
    unsigned char zeros132[132] = { 0 };
    unsigned char session_secrand[2][32];
    unsigned char nonrepeating_cnt = 0;
    secp256k1_musig_secnonce secnonce[2];
    secp256k1_musig_secnonce secnonce_tmp;
    secp256k1_musig_secnonce invalid_secnonce;
    secp256k1_musig_pubnonce pubnonce[2];
    const secp256k1_musig_pubnonce *pubnonce_ptr[2];
    unsigned char pubnonce_ser[66];
    secp256k1_musig_pubnonce inf_pubnonce[2];
    const secp256k1_musig_pubnonce *inf_pubnonce_ptr[2];
    secp256k1_musig_pubnonce invalid_pubnonce;
    const secp256k1_musig_pubnonce *invalid_pubnonce_ptr[1];
    secp256k1_musig_aggnonce aggnonce;
    unsigned char aggnonce_ser[66];
    unsigned char msg[32];
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_pubkey full_agg_pk;
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_musig_keyagg_cache invalid_keyagg_cache;
    secp256k1_musig_session session;
    secp256k1_musig_session invalid_session;
    secp256k1_pubkey pk[2];
    const secp256k1_pubkey *pk_ptr[2];
    secp256k1_pubkey invalid_pk;
    const secp256k1_pubkey *invalid_pk_ptr2[2];
    const secp256k1_pubkey *invalid_pk_ptr3[3];
    unsigned char tweak[32];
    int i;

    /** setup **/
    memset(max64, 0xff, sizeof(max64));
    memset(&invalid_keypair, 0, sizeof(invalid_keypair));
    memset(&invalid_pk, 0, sizeof(invalid_pk));
    memset(&invalid_secnonce, 0, sizeof(invalid_secnonce));
    memset(&invalid_partial_sig, 0, sizeof(invalid_partial_sig));
    pubnonce_summing_to_inf(inf_pubnonce);
    /* Simulate structs being uninitialized by setting it to 0s. We don't want
     * to produce undefined behavior by actually providing uninitialized
     * structs. */
    memset(&invalid_keyagg_cache, 0, sizeof(invalid_keyagg_cache));
    memset(&invalid_pk, 0, sizeof(invalid_pk));
    memset(&invalid_pubnonce, 0, sizeof(invalid_pubnonce));
    memset(&invalid_session, 0, sizeof(invalid_session));

    testrand256(msg);
    testrand256(tweak);
    for (i = 0; i < 2; i++) {
        pk_ptr[i] = &pk[i];
        invalid_pk_ptr2[i] = &invalid_pk;
        invalid_pk_ptr3[i] = &pk[i];
        pubnonce_ptr[i] = &pubnonce[i];
        inf_pubnonce_ptr[i] = &inf_pubnonce[i];
        partial_sig_ptr[i] = &partial_sig[i];
        invalid_partial_sig_ptr[i] = &partial_sig[i];
        testrand256(session_secrand[i]);
        testrand256(sk[i]);
        CHECK(create_keypair_and_pk(&keypair[i], &pk[i], sk[i]));
    }
    invalid_pubnonce_ptr[0] = &invalid_pubnonce;
    invalid_partial_sig_ptr[0] = &invalid_partial_sig;
    /* invalid_pk_ptr3 has two valid, one invalid pk, which is important to test
     * musig_pubkey_agg */
    invalid_pk_ptr3[2] = &invalid_pk;

    /** main test body **/

    /** Key aggregation **/
    CHECK(secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, pk_ptr, 2) == 1);
    CHECK(secp256k1_musig_pubkey_agg(CTX, NULL, &keyagg_cache, pk_ptr, 2) == 1);
    CHECK(secp256k1_musig_pubkey_agg(CTX, &agg_pk, NULL, pk_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, NULL, 2));
    CHECK(memcmp_and_randomize(agg_pk.data, zeros132, sizeof(agg_pk.data)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, invalid_pk_ptr2, 2));
    CHECK(memcmp_and_randomize(agg_pk.data, zeros132, sizeof(agg_pk.data)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, invalid_pk_ptr3, 3));
    CHECK(memcmp_and_randomize(agg_pk.data, zeros132, sizeof(agg_pk.data)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, pk_ptr, 0));
    CHECK(memcmp_and_randomize(agg_pk.data, zeros132, sizeof(agg_pk.data)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, NULL, 0));
    CHECK(memcmp_and_randomize(agg_pk.data, zeros132, sizeof(agg_pk.data)) == 0);

    CHECK(secp256k1_musig_pubkey_agg(CTX, &agg_pk, &keyagg_cache, pk_ptr, 2) == 1);

    /* pubkey_get */
    CHECK(secp256k1_musig_pubkey_get(CTX, &full_agg_pk, &keyagg_cache) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_get(CTX, NULL, &keyagg_cache));
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubkey_get(CTX, &full_agg_pk, NULL));
    CHECK(secp256k1_memcmp_var(&full_agg_pk, zeros132, sizeof(full_agg_pk)) == 0);

    /** Tweaking **/
    {
        int (*tweak_func[2]) (const secp256k1_context* ctx, secp256k1_pubkey *output_pubkey, secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *tweak32);
        tweak_func[0] = secp256k1_musig_pubkey_ec_tweak_add;
        tweak_func[1] = secp256k1_musig_pubkey_xonly_tweak_add;
        for (i = 0; i < 2; i++) {
            secp256k1_pubkey tmp_output_pk;
            secp256k1_musig_keyagg_cache tmp_keyagg_cache = keyagg_cache;
            CHECK((*tweak_func[i])(CTX, &tmp_output_pk, &tmp_keyagg_cache, tweak) == 1);
            /* Reset keyagg_cache */
            tmp_keyagg_cache = keyagg_cache;
            CHECK((*tweak_func[i])(CTX, NULL, &tmp_keyagg_cache, tweak) == 1);
            tmp_keyagg_cache = keyagg_cache;
            CHECK_ILLEGAL(CTX, (*tweak_func[i])(CTX, &tmp_output_pk, NULL, tweak));
            CHECK(memcmp_and_randomize(tmp_output_pk.data, zeros132, sizeof(tmp_output_pk.data)) == 0);
            tmp_keyagg_cache = keyagg_cache;
            CHECK_ILLEGAL(CTX, (*tweak_func[i])(CTX, &tmp_output_pk, &tmp_keyagg_cache, NULL));
            CHECK(memcmp_and_randomize(tmp_output_pk.data, zeros132, sizeof(tmp_output_pk.data)) == 0);
            tmp_keyagg_cache = keyagg_cache;
            CHECK((*tweak_func[i])(CTX, &tmp_output_pk, &tmp_keyagg_cache, max64) == 0);
            CHECK(memcmp_and_randomize(tmp_output_pk.data, zeros132, sizeof(tmp_output_pk.data)) == 0);
            tmp_keyagg_cache = keyagg_cache;
            /* Uninitialized keyagg_cache */
            CHECK_ILLEGAL(CTX, (*tweak_func[i])(CTX, &tmp_output_pk, &invalid_keyagg_cache, tweak));
            CHECK(memcmp_and_randomize(tmp_output_pk.data, zeros132, sizeof(tmp_output_pk.data)) == 0);
        }
    }

    /** Session creation with nonce_gen **/
    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &pk[0], msg, &keyagg_cache, max64) == 1);
    /* nonce_gen, if successful, sets session_secrand to the zero array, which
     * makes subsequent nonce_gen calls with the same session_secrand fail. So
     * check that session_secrand is indeed the zero array and fill it with
     * random values again. */
    CHECK(memcmp_and_randomize(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);

    CHECK_ILLEGAL(STATIC_CTX, secp256k1_musig_nonce_gen(STATIC_CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &pk[0], msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen(CTX, NULL, &pubnonce[0], session_secrand[0], sk[0], &pk[0], msg, &keyagg_cache, max64));

    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen(CTX, &secnonce[0], NULL, session_secrand[0], sk[0], &pk[0], msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], NULL, sk[0], &pk[0], msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    /* session_secrand = 0 is disallowed because it indicates a faulty RNG */
    memcpy(&session_secrand[0], zeros132, sizeof(session_secrand[0]));
    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], zeros132, sk[0], &pk[0], msg, &keyagg_cache, max64) == 0);
    CHECK(memcmp_and_randomize(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], NULL, &pk[0], msg, &keyagg_cache, max64) == 1);
    CHECK(memcmp_and_randomize(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);

    /* invalid seckey */
    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], max64, &pk[0], msg, &keyagg_cache, max64) == 0);
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], NULL, msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &invalid_pk, msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &pk[0], NULL, &keyagg_cache, max64) == 1);
    CHECK(memcmp_and_randomize(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);

    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &pk[0], msg, NULL, max64) == 1);
    CHECK(memcmp_and_randomize(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);

    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &pk[0], msg, &invalid_keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);

    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk[0], &pk[0], msg, &keyagg_cache, NULL) == 1);
    CHECK(memcmp_and_randomize(session_secrand[0], zeros132, sizeof(session_secrand[0])) == 0);

    /* Every in-argument except session_secrand and pubkey can be NULL */
    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], NULL, &pk[0], NULL, NULL, NULL) == 1);
    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[1], &pubnonce[1], session_secrand[1], sk[1], &pk[1], NULL, NULL, NULL) == 1);

    /** Session creation with nonce_gen_counter **/
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &keypair[0], msg, &keyagg_cache, max64) == 1);
    CHECK_ILLEGAL(STATIC_CTX, secp256k1_musig_nonce_gen_counter(STATIC_CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &keypair[0], msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen_counter(CTX, NULL, &pubnonce[0], nonrepeating_cnt, &keypair[0], msg, &keyagg_cache, max64));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], NULL, nonrepeating_cnt, &keypair[0], msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);
    /* using nonce_gen_counter requires keypair */
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, NULL, msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);
    /* invalid keypair */
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &invalid_keypair, msg, &keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &keypair[0], NULL, &keyagg_cache, max64) == 1);
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &keypair[0], msg, NULL, max64) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &keypair[0], msg, &invalid_keyagg_cache, max64));
    CHECK(memcmp_and_randomize(secnonce[0].data, zeros132, sizeof(secnonce[0].data)) == 0);
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt,&keypair[0], msg, &keyagg_cache, NULL) == 1);

    /* Every in-argument except nonrepeating_cnt and keypair can be NULL */
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[0], &pubnonce[0], nonrepeating_cnt, &keypair[0], NULL, NULL, NULL) == 1);
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce[1], &pubnonce[1], nonrepeating_cnt, &keypair[1], NULL, NULL, NULL) == 1);


    /** Serialize and parse public nonces **/
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubnonce_serialize(CTX, NULL, &pubnonce[0]));
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubnonce_serialize(CTX, pubnonce_ser, NULL));
    CHECK(memcmp_and_randomize(pubnonce_ser, zeros132, sizeof(pubnonce_ser)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubnonce_serialize(CTX, pubnonce_ser, &invalid_pubnonce));
    CHECK(memcmp_and_randomize(pubnonce_ser, zeros132, sizeof(pubnonce_ser)) == 0);
    CHECK(secp256k1_musig_pubnonce_serialize(CTX, pubnonce_ser, &pubnonce[0]) == 1);

    CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce[0], pubnonce_ser) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubnonce_parse(CTX, NULL, pubnonce_ser));
    CHECK_ILLEGAL(CTX, secp256k1_musig_pubnonce_parse(CTX, &pubnonce[0], NULL));
    CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce[0], zeros132) == 0);
    CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce[0], pubnonce_ser) == 1);

    {
        /* Check that serialize and parse results in the same value */
        secp256k1_musig_pubnonce tmp;
        CHECK(secp256k1_musig_pubnonce_serialize(CTX, pubnonce_ser, &pubnonce[0]) == 1);
        CHECK(secp256k1_musig_pubnonce_parse(CTX, &tmp, pubnonce_ser) == 1);
        CHECK(secp256k1_memcmp_var(&tmp, &pubnonce[0], sizeof(tmp)) == 0);
    }

    /** Receive nonces and aggregate **/
    CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_agg(CTX, NULL, pubnonce_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_agg(CTX, &aggnonce, NULL, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 0));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_agg(CTX, &aggnonce, invalid_pubnonce_ptr, 1));
    CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, inf_pubnonce_ptr, 2) == 1);
    {
        /* Check that the aggnonce encodes two points at infinity */
        secp256k1_ge aggnonce_pt[2];
        secp256k1_musig_aggnonce_load(CTX, aggnonce_pt, &aggnonce);
        for (i = 0; i < 2; i++) {
            secp256k1_ge_is_infinity(&aggnonce_pt[i]);
        }
    }
    CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2) == 1);

    /** Serialize and parse aggregate nonces **/
    CHECK(secp256k1_musig_aggnonce_serialize(CTX, aggnonce_ser, &aggnonce) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_aggnonce_serialize(CTX, NULL, &aggnonce));
    CHECK_ILLEGAL(CTX, secp256k1_musig_aggnonce_serialize(CTX, aggnonce_ser, NULL));
    CHECK(memcmp_and_randomize(aggnonce_ser, zeros132, sizeof(aggnonce_ser)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_aggnonce_serialize(CTX, aggnonce_ser, (secp256k1_musig_aggnonce*) &invalid_pubnonce));
    CHECK(memcmp_and_randomize(aggnonce_ser, zeros132, sizeof(aggnonce_ser)) == 0);
    CHECK(secp256k1_musig_aggnonce_serialize(CTX, aggnonce_ser, &aggnonce) == 1);

    CHECK(secp256k1_musig_aggnonce_parse(CTX, &aggnonce, aggnonce_ser) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_aggnonce_parse(CTX, NULL, aggnonce_ser));
    CHECK_ILLEGAL(CTX, secp256k1_musig_aggnonce_parse(CTX, &aggnonce, NULL));
    CHECK(secp256k1_musig_aggnonce_parse(CTX, &aggnonce, zeros132) == 1);
    CHECK(secp256k1_musig_aggnonce_parse(CTX, &aggnonce, aggnonce_ser) == 1);

    {
        /* Check that serialize and parse results in the same value */
        secp256k1_musig_aggnonce tmp;
        CHECK(secp256k1_musig_aggnonce_serialize(CTX, aggnonce_ser, &aggnonce) == 1);
        CHECK(secp256k1_musig_aggnonce_parse(CTX, &tmp, aggnonce_ser) == 1);
        CHECK(secp256k1_memcmp_var(&tmp, &aggnonce, sizeof(tmp)) == 0);
    }

    /** Process nonces **/
    CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, msg, &keyagg_cache) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_process(CTX, NULL, &aggnonce, msg, &keyagg_cache));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_process(CTX, &session, NULL, msg, &keyagg_cache));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_process(CTX, &session, (secp256k1_musig_aggnonce*) &invalid_pubnonce, msg, &keyagg_cache));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_process(CTX, &session, &aggnonce, NULL, &keyagg_cache));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_process(CTX, &session, &aggnonce, msg, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_musig_nonce_process(CTX, &session, &aggnonce, msg, &invalid_keyagg_cache));

    CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, msg, &keyagg_cache) == 1);

    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0], &keyagg_cache, &session) == 1);
    /* The secnonce is set to 0 and subsequent signing attempts fail */
    CHECK(secp256k1_memcmp_var(&secnonce_tmp, zeros132, sizeof(secnonce_tmp)) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0], &keyagg_cache, &session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, NULL, &secnonce_tmp, &keypair[0], &keyagg_cache, &session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], NULL, &keypair[0], &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &invalid_secnonce, &keypair[0], &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, NULL, &keyagg_cache, &session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &invalid_keypair, &keyagg_cache, &session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    {
        unsigned char sk_tmp[32];
        secp256k1_keypair keypair_tmp;
        testrand256(sk_tmp);
        CHECK(secp256k1_keypair_create(CTX, &keypair_tmp, sk_tmp));
        CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair_tmp, &keyagg_cache, &session));
        memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    }
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0], NULL, &session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0], &invalid_keyagg_cache, &session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0], &keyagg_cache, NULL));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce_tmp, &keypair[0], &keyagg_cache, &invalid_session));
    memcpy(&secnonce_tmp, &secnonce[0], sizeof(secnonce_tmp));

    CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce[0], &keypair[0], &keyagg_cache, &session) == 1);
    CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig[1], &secnonce[1], &keypair[1], &keyagg_cache, &session) == 1);

    CHECK(secp256k1_musig_partial_sig_serialize(CTX, buf, &partial_sig[0]) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_serialize(CTX, NULL, &partial_sig[0]));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_serialize(CTX, buf, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_serialize(CTX, buf, &invalid_partial_sig));
    CHECK(secp256k1_musig_partial_sig_parse(CTX, &partial_sig[0], buf) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_parse(CTX, NULL, buf));
    {
        /* Check that parsing failure results in an invalid sig */
        secp256k1_musig_partial_sig tmp;
        CHECK(secp256k1_musig_partial_sig_parse(CTX, &tmp, max64) == 0);
        CHECK(secp256k1_memcmp_var(&tmp, zeros132, sizeof(partial_sig[0])) == 0);
    }
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_parse(CTX, &partial_sig[0], NULL));

    {
        /* Check that serialize and parse results in the same value */
        secp256k1_musig_partial_sig tmp;
        CHECK(secp256k1_musig_partial_sig_serialize(CTX, buf, &partial_sig[0]) == 1);
        CHECK(secp256k1_musig_partial_sig_parse(CTX, &tmp, buf) == 1);
        CHECK(secp256k1_memcmp_var(&tmp, &partial_sig[0], sizeof(tmp)) == 0);
    }

    /** Partial signature verification */
    CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], &keyagg_cache, &session) == 1);
    CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[1], &pubnonce[0], &pk[0], &keyagg_cache, &session) == 0);
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, NULL, &pubnonce[0], &pk[0], &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &invalid_partial_sig, &pubnonce[0], &pk[0], &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], NULL, &pk[0], &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &invalid_pubnonce, &pk[0], &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], NULL, &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &invalid_pk, &keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], NULL, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], &invalid_keyagg_cache, &session));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], &keyagg_cache, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], &keyagg_cache, &invalid_session));

    CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], &keyagg_cache, &session) == 1);
    CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[1], &pubnonce[1], &pk[1], &keyagg_cache, &session) == 1);

    /** Signature aggregation and verification */
    CHECK(secp256k1_musig_partial_sig_agg(CTX, pre_sig, &session, partial_sig_ptr, 2) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_agg(CTX, NULL, &session, partial_sig_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_agg(CTX, pre_sig, NULL, partial_sig_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_agg(CTX, pre_sig, &invalid_session, partial_sig_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_agg(CTX, pre_sig, &session, NULL, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_agg(CTX, pre_sig, &session, invalid_partial_sig_ptr, 2));
    CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sig_agg(CTX, pre_sig, &session, partial_sig_ptr, 0));
    CHECK(secp256k1_musig_partial_sig_agg(CTX, pre_sig, &session, partial_sig_ptr, 1) == 1);
    CHECK(secp256k1_musig_partial_sig_agg(CTX, pre_sig, &session, partial_sig_ptr, 2) == 1);
}

static void musig_nonce_bitflip(unsigned char **args, size_t n_flip, size_t n_bytes) {
    secp256k1_scalar k1[2], k2[2];

    secp256k1_nonce_function_musig(k1, args[0], args[1], args[2], args[3], args[4], args[5]);
    testrand_flip(args[n_flip], n_bytes);
    secp256k1_nonce_function_musig(k2, args[0], args[1], args[2], args[3], args[4], args[5]);
    CHECK(secp256k1_scalar_eq(&k1[0], &k2[0]) == 0);
    CHECK(secp256k1_scalar_eq(&k1[1], &k2[1]) == 0);
}

static void musig_nonce_test(void) {
    unsigned char *args[6];
    unsigned char session_secrand[32];
    unsigned char sk[32];
    unsigned char pk[33];
    unsigned char msg[32];
    unsigned char agg_pk[32];
    unsigned char extra_input[32];
    int i, j;
    secp256k1_scalar k[6][2];

    testrand_bytes_test(session_secrand, sizeof(session_secrand));
    testrand_bytes_test(sk, sizeof(sk));
    testrand_bytes_test(pk, sizeof(pk));
    testrand_bytes_test(msg, sizeof(msg));
    testrand_bytes_test(agg_pk, sizeof(agg_pk));
    testrand_bytes_test(extra_input, sizeof(extra_input));

    /* Check that a bitflip in an argument results in different nonces. */
    args[0] = session_secrand;
    args[1] = msg;
    args[2] = sk;
    args[3] = pk;
    args[4] = agg_pk;
    args[5] = extra_input;
    for (i = 0; i < COUNT; i++) {
        musig_nonce_bitflip(args, 0, sizeof(session_secrand));
        musig_nonce_bitflip(args, 1, sizeof(msg));
        musig_nonce_bitflip(args, 2, sizeof(sk));
        musig_nonce_bitflip(args, 3, sizeof(pk));
        musig_nonce_bitflip(args, 4, sizeof(agg_pk));
        musig_nonce_bitflip(args, 5, sizeof(extra_input));
    }
    /* Check that if any argument is NULL, a different nonce is produced than if
     * any other argument is NULL. */
    memcpy(msg, session_secrand, sizeof(msg));
    memcpy(sk, session_secrand, sizeof(sk));
    memcpy(pk, session_secrand, sizeof(session_secrand));
    memcpy(agg_pk, session_secrand, sizeof(agg_pk));
    memcpy(extra_input, session_secrand, sizeof(extra_input));
    secp256k1_nonce_function_musig(k[0], args[0], args[1], args[2], args[3], args[4], args[5]);
    secp256k1_nonce_function_musig(k[1], args[0], NULL, args[2], args[3], args[4], args[5]);
    secp256k1_nonce_function_musig(k[2], args[0], args[1], NULL, args[3], args[4], args[5]);
    secp256k1_nonce_function_musig(k[3], args[0], args[1], args[2], NULL, args[4], args[5]);
    secp256k1_nonce_function_musig(k[4], args[0], args[1], args[2], args[3], NULL, args[5]);
    secp256k1_nonce_function_musig(k[5], args[0], args[1], args[2], args[3], args[4], NULL);
    for (i = 0; i < 6; i++) {
        CHECK(!secp256k1_scalar_eq(&k[i][0], &k[i][1]));
        for (j = i+1; j < 6; j++) {
            CHECK(!secp256k1_scalar_eq(&k[i][0], &k[j][0]));
            CHECK(!secp256k1_scalar_eq(&k[i][1], &k[j][1]));
        }
    }
}

static void sha256_tag_test_internal(secp256k1_sha256 *sha_tagged, unsigned char *tag, size_t taglen) {
    secp256k1_sha256 sha;
    secp256k1_sha256_initialize_tagged(&sha, tag, taglen);
    test_sha256_eq(&sha, sha_tagged);
}

/* Checks that the initialized tagged hashes have the expected
 * state. */
static void sha256_tag_test(void) {
    secp256k1_sha256 sha;
    {
        char tag[] = "KeyAgg list";
        secp256k1_musig_keyagglist_sha256(&sha);
        sha256_tag_test_internal(&sha, (unsigned char*)tag, sizeof(tag) - 1);
    }
    {
        char tag[] = "KeyAgg coefficient";
        secp256k1_musig_keyaggcoef_sha256(&sha);
        sha256_tag_test_internal(&sha, (unsigned char*)tag, sizeof(tag) - 1);
    }
    {
        unsigned char tag[] = "MuSig/aux";
        secp256k1_nonce_function_musig_sha256_tagged_aux(&sha);
        sha256_tag_test_internal(&sha, (unsigned char*)tag, sizeof(tag) - 1);
    }
    {
        unsigned char tag[] = "MuSig/nonce";
        secp256k1_nonce_function_musig_sha256_tagged(&sha);
        sha256_tag_test_internal(&sha, (unsigned char*)tag, sizeof(tag) - 1);
    }
    {
        unsigned char tag[] = "MuSig/noncecoef";
        secp256k1_musig_compute_noncehash_sha256_tagged(&sha);
        sha256_tag_test_internal(&sha, (unsigned char*)tag, sizeof(tag) - 1);
    }
}

/* Attempts to create a signature for the aggregate public key using given secret
 * keys and keyagg_cache. */
static void musig_tweak_test_helper(const secp256k1_xonly_pubkey* agg_pk, const unsigned char *sk0, const unsigned char *sk1, secp256k1_musig_keyagg_cache *keyagg_cache) {
    secp256k1_pubkey pk[2];
    unsigned char session_secrand[2][32];
    unsigned char msg[32];
    secp256k1_musig_secnonce secnonce[2];
    secp256k1_musig_pubnonce pubnonce[2];
    const secp256k1_musig_pubnonce *pubnonce_ptr[2];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_keypair keypair[2];
    secp256k1_musig_session session;
    secp256k1_musig_partial_sig partial_sig[2];
    const secp256k1_musig_partial_sig *partial_sig_ptr[2];
    unsigned char final_sig[64];
    int i;

    for (i = 0; i < 2; i++) {
        pubnonce_ptr[i] = &pubnonce[i];
        partial_sig_ptr[i] = &partial_sig[i];

        testrand256(session_secrand[i]);
    }
    CHECK(create_keypair_and_pk(&keypair[0], &pk[0], sk0) == 1);
    CHECK(create_keypair_and_pk(&keypair[1], &pk[1], sk1) == 1);
    testrand256(msg);

    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[0], &pubnonce[0], session_secrand[0], sk0, &pk[0], NULL, NULL, NULL) == 1);
    CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce[1], &pubnonce[1], session_secrand[1], sk1, &pk[1], NULL, NULL, NULL) == 1);

    CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2) == 1);
    CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, msg, keyagg_cache) == 1);

    CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig[0], &secnonce[0], &keypair[0], keyagg_cache, &session) == 1);
    CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig[1], &secnonce[1], &keypair[1], keyagg_cache, &session) == 1);

    CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[0], &pubnonce[0], &pk[0], keyagg_cache, &session) == 1);
    CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig[1], &pubnonce[1], &pk[1], keyagg_cache, &session) == 1);

    CHECK(secp256k1_musig_partial_sig_agg(CTX, final_sig, &session, partial_sig_ptr, 2) == 1);
    CHECK(secp256k1_schnorrsig_verify(CTX, final_sig, msg, sizeof(msg), agg_pk) == 1);
}

/* Create aggregate public key P[0], tweak multiple times (using xonly and
 * plain tweaking) and test signing. */
static void musig_tweak_test(void) {
    unsigned char sk[2][32];
    secp256k1_pubkey pk[2];
    const secp256k1_pubkey *pk_ptr[2];
    secp256k1_musig_keyagg_cache keyagg_cache;
    enum { N_TWEAKS = 8 };
    secp256k1_pubkey P[N_TWEAKS + 1];
    secp256k1_xonly_pubkey P_xonly[N_TWEAKS + 1];
    int i;

    /* Key Setup */
    for (i = 0; i < 2; i++) {
        pk_ptr[i] = &pk[i];
        testrand256(sk[i]);
        CHECK(create_keypair_and_pk(NULL, &pk[i], sk[i]) == 1);
    }
    /* Compute P0 = keyagg(pk0, pk1) and test signing for it */
    CHECK(secp256k1_musig_pubkey_agg(CTX, &P_xonly[0], &keyagg_cache, pk_ptr, 2) == 1);
    musig_tweak_test_helper(&P_xonly[0], sk[0], sk[1], &keyagg_cache);
    CHECK(secp256k1_musig_pubkey_get(CTX, &P[0], &keyagg_cache));

    /* Compute Pi = f(Pj) + tweaki*G where where j = i-1 and try signing for
     * that key. If xonly is set to true, the function f normalizes the input
     * point to have an even X-coordinate ("xonly-tweaking").
     * Otherwise, the function f is the identity function. */
    for (i = 1; i <= N_TWEAKS; i++) {
        unsigned char tweak[32];
        int P_parity;
        int xonly = testrand_bits(1);

        testrand256(tweak);
        if (xonly) {
            CHECK(secp256k1_musig_pubkey_xonly_tweak_add(CTX, &P[i], &keyagg_cache, tweak) == 1);
        } else {
            CHECK(secp256k1_musig_pubkey_ec_tweak_add(CTX, &P[i], &keyagg_cache, tweak) == 1);
        }
        CHECK(secp256k1_xonly_pubkey_from_pubkey(CTX, &P_xonly[i], &P_parity, &P[i]));
        /* Check that musig_pubkey_tweak_add produces same result as
         * xonly_pubkey_tweak_add or ec_pubkey_tweak_add. */
        if (xonly) {
            unsigned char P_serialized[32];
            CHECK(secp256k1_xonly_pubkey_serialize(CTX, P_serialized, &P_xonly[i]));
            CHECK(secp256k1_xonly_pubkey_tweak_add_check(CTX, P_serialized, P_parity, &P_xonly[i-1], tweak) == 1);
        } else {
            secp256k1_pubkey tmp_key = P[i-1];
            CHECK(secp256k1_ec_pubkey_tweak_add(CTX, &tmp_key, tweak));
            CHECK(secp256k1_memcmp_var(&tmp_key, &P[i], sizeof(tmp_key)) == 0);
        }
        /* Test signing for P[i] */
        musig_tweak_test_helper(&P_xonly[i], sk[0], sk[1], &keyagg_cache);
    }
}

int musig_vectors_keyagg_and_tweak(enum MUSIG_ERROR *error,
                                   secp256k1_musig_keyagg_cache *keyagg_cache,
                                   unsigned char *agg_pk_ser,
                                   const unsigned char pubkeys33[][33],
                                   const unsigned char tweaks32[][32],
                                   size_t key_indices_len,
                                   const size_t *key_indices,
                                   size_t tweak_indices_len,
                                   const size_t *tweak_indices,
                                   const int *is_xonly) {
    secp256k1_pubkey pubkeys[MUSIG_VECTORS_MAX_PUBKEYS];
    const secp256k1_pubkey *pk_ptr[MUSIG_VECTORS_MAX_PUBKEYS];
    int i;
    secp256k1_pubkey agg_pk;
    secp256k1_xonly_pubkey agg_pk_xonly;

    for (i = 0; i < (int)key_indices_len; i++) {
        if (!secp256k1_ec_pubkey_parse(CTX, &pubkeys[i], pubkeys33[key_indices[i]], 33)) {
            *error = MUSIG_PUBKEY;
            return 0;
        }
        pk_ptr[i] = &pubkeys[i];
    }
    if (!secp256k1_musig_pubkey_agg(CTX, NULL, keyagg_cache, pk_ptr, key_indices_len)) {
        *error = MUSIG_OTHER;
        return 0;
    }

    for (i = 0; i < (int)tweak_indices_len; i++) {
        if (is_xonly[i]) {
            if (!secp256k1_musig_pubkey_xonly_tweak_add(CTX, NULL, keyagg_cache, tweaks32[tweak_indices[i]])) {
                *error = MUSIG_TWEAK;
                return 0;
            }
        } else {
            if (!secp256k1_musig_pubkey_ec_tweak_add(CTX, NULL, keyagg_cache, tweaks32[tweak_indices[i]])) {
                *error = MUSIG_TWEAK;
                return 0;
            }
        }
    }
    if (!secp256k1_musig_pubkey_get(CTX, &agg_pk, keyagg_cache)) {
        *error = MUSIG_OTHER;
        return 0;
    }

    if (!secp256k1_xonly_pubkey_from_pubkey(CTX, &agg_pk_xonly, NULL, &agg_pk)) {
        *error = MUSIG_OTHER;
        return 0;
    }

    if (agg_pk_ser != NULL) {
        if (!secp256k1_xonly_pubkey_serialize(CTX, agg_pk_ser, &agg_pk_xonly)) {
            *error = MUSIG_OTHER;
            return 0;
        }
    }

    return 1;
}

static void musig_test_vectors_keyagg(void) {
    size_t i;
    const struct musig_key_agg_vector *vector = &musig_key_agg_vector;

    for (i = 0; i < sizeof(vector->valid_case)/sizeof(vector->valid_case[0]); i++) {
        const struct musig_key_agg_valid_test_case *c = &vector->valid_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        unsigned char agg_pk[32];

        CHECK(musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, agg_pk, vector->pubkeys, vector->tweaks, c->key_indices_len, c->key_indices, 0, NULL, NULL));
        CHECK(secp256k1_memcmp_var(agg_pk, c->expected, sizeof(agg_pk)) == 0);
    }

    for (i = 0; i < sizeof(vector->error_case)/sizeof(vector->error_case[0]); i++) {
        const struct musig_key_agg_error_test_case *c = &vector->error_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;

        CHECK(!musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, vector->tweaks, c->key_indices_len, c->key_indices, c->tweak_indices_len, c->tweak_indices, c->is_xonly));
        CHECK(c->error == error);
    }
}

static void musig_test_vectors_noncegen(void) {
    size_t i;
    const struct musig_nonce_gen_vector *vector = &musig_nonce_gen_vector;

    for (i = 0; i < sizeof(vector->test_case)/sizeof(vector->test_case[0]); i++) {
        const struct musig_nonce_gen_test_case *c = &vector->test_case[i];
        secp256k1_musig_keyagg_cache keyagg_cache;
        secp256k1_musig_keyagg_cache *keyagg_cache_ptr = NULL;
        unsigned char session_secrand32[32];
        secp256k1_musig_secnonce secnonce;
        secp256k1_musig_pubnonce pubnonce;
        const unsigned char *sk = NULL;
        const unsigned char *msg = NULL;
        const unsigned char *extra_in = NULL;
        secp256k1_pubkey pk;
        unsigned char pubnonce66[66];

        memcpy(session_secrand32, c->rand_, 32);
        if (c->has_sk) {
            sk = c->sk;
        }
        if (c->has_aggpk) {
            /* Create keyagg_cache from aggpk */
            secp256k1_keyagg_cache_internal cache_i;
            secp256k1_xonly_pubkey aggpk;
            memset(&cache_i, 0, sizeof(cache_i));
            CHECK(secp256k1_xonly_pubkey_parse(CTX, &aggpk, c->aggpk));
            CHECK(secp256k1_xonly_pubkey_load(CTX, &cache_i.pk, &aggpk));
            secp256k1_keyagg_cache_save(&keyagg_cache, &cache_i);
            keyagg_cache_ptr = &keyagg_cache;
        }
        if (c->has_msg) {
            msg = c->msg;
        }
        if (c->has_extra_in) {
            extra_in = c->extra_in;
        }

        CHECK(secp256k1_ec_pubkey_parse(CTX, &pk, c->pk, sizeof(c->pk)));
        CHECK(secp256k1_musig_nonce_gen(CTX, &secnonce, &pubnonce, session_secrand32, sk, &pk, msg, keyagg_cache_ptr, extra_in) == 1);
        CHECK(secp256k1_memcmp_var(&secnonce.data[4], c->expected_secnonce, 2*32) == 0);
        /* The last element of the secnonce is the public key (uncompressed in
         * secp256k1_musig_secnonce, compressed in the test vector secnonce). */
        CHECK(secp256k1_memcmp_var(&secnonce.data[4+2*32], &pk, sizeof(pk)) == 0);
        CHECK(secp256k1_memcmp_var(&c->expected_secnonce[2*32], c->pk, sizeof(c->pk)) == 0);

        CHECK(secp256k1_musig_pubnonce_serialize(CTX, pubnonce66, &pubnonce) == 1);
        CHECK(sizeof(c->expected_pubnonce) == sizeof(pubnonce66));
        CHECK(secp256k1_memcmp_var(pubnonce66, c->expected_pubnonce, sizeof(pubnonce66)) == 0);
    }
}


static void musig_test_vectors_nonceagg(void) {
    size_t i;
    int j;
    const struct musig_nonce_agg_vector *vector = &musig_nonce_agg_vector;

    for (i = 0; i < sizeof(vector->valid_case)/sizeof(vector->valid_case[0]); i++) {
        const struct musig_nonce_agg_test_case *c = &vector->valid_case[i];
        secp256k1_musig_pubnonce pubnonce[2];
        const secp256k1_musig_pubnonce *pubnonce_ptr[2];
        secp256k1_musig_aggnonce aggnonce;
        unsigned char aggnonce66[66];

        for (j = 0; j < 2; j++) {
            CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce[j], vector->pnonces[c->pnonce_indices[j]]) == 1);
            pubnonce_ptr[j] = &pubnonce[j];
        }
        CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, 2));
        CHECK(secp256k1_musig_aggnonce_serialize(CTX, aggnonce66, &aggnonce));
        CHECK(secp256k1_memcmp_var(aggnonce66, c->expected, 33) == 0);
    }
    for (i = 0; i < sizeof(vector->error_case)/sizeof(vector->error_case[0]); i++) {
        const struct musig_nonce_agg_test_case *c = &vector->error_case[i];
        secp256k1_musig_pubnonce pubnonce[2];
        for (j = 0; j < 2; j++) {
            int expected = c->invalid_nonce_idx != j;
            CHECK(expected == secp256k1_musig_pubnonce_parse(CTX, &pubnonce[j], vector->pnonces[c->pnonce_indices[j]]));
        }
    }
}

static void musig_test_set_secnonce(secp256k1_musig_secnonce *secnonce, const unsigned char *secnonce64, const secp256k1_pubkey *pubkey) {
    secp256k1_ge pk;
    secp256k1_scalar k[2];

    secp256k1_scalar_set_b32(&k[0], &secnonce64[0], NULL);
    secp256k1_scalar_set_b32(&k[1], &secnonce64[32], NULL);
    CHECK(secp256k1_pubkey_load(CTX, &pk, pubkey));
    secp256k1_musig_secnonce_save(secnonce, k, &pk);
}

static void musig_test_vectors_signverify(void) {
    size_t i;
    const struct musig_sign_verify_vector *vector = &musig_sign_verify_vector;

    for (i = 0; i < sizeof(vector->valid_case)/sizeof(vector->valid_case[0]); i++) {
        const struct musig_valid_case *c = &vector->valid_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        secp256k1_pubkey pubkey;
        secp256k1_musig_pubnonce pubnonce;
        secp256k1_musig_aggnonce aggnonce;
        secp256k1_musig_session session;
        secp256k1_musig_partial_sig partial_sig;
        secp256k1_musig_secnonce secnonce;
        secp256k1_keypair keypair;
        unsigned char partial_sig32[32];

        CHECK(secp256k1_keypair_create(CTX, &keypair, vector->sk));
        CHECK(musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, NULL, c->key_indices_len, c->key_indices, 0, NULL, NULL));

        CHECK(secp256k1_musig_aggnonce_parse(CTX, &aggnonce, vector->aggnonces[c->aggnonce_index]));
        CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, vector->msgs[c->msg_index], &keyagg_cache));

        CHECK(secp256k1_ec_pubkey_parse(CTX, &pubkey, vector->pubkeys[0], sizeof(vector->pubkeys[0])));
        musig_test_set_secnonce(&secnonce, vector->secnonces[0], &pubkey);
        CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig, &secnonce, &keypair, &keyagg_cache, &session));
        CHECK(secp256k1_musig_partial_sig_serialize(CTX, partial_sig32, &partial_sig));
        CHECK(secp256k1_memcmp_var(partial_sig32, c->expected, sizeof(partial_sig32)) == 0);

        CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce, vector->pubnonces[0]));
        CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig, &pubnonce, &pubkey, &keyagg_cache, &session));
    }
    for (i = 0; i < sizeof(vector->sign_error_case)/sizeof(vector->sign_error_case[0]); i++) {
        const struct musig_sign_error_case *c = &vector->sign_error_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        secp256k1_pubkey pubkey;
        secp256k1_musig_aggnonce aggnonce;
        secp256k1_musig_session session;
        secp256k1_musig_partial_sig partial_sig;
        secp256k1_musig_secnonce secnonce;
        secp256k1_keypair keypair;
        int expected;

        if (i == 0) {
            /* Skip this vector since the implementation does not error out when
             * the signing key does not belong to any pubkey. */
            continue;
        }
        expected = c->error != MUSIG_PUBKEY;
        CHECK(expected == musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, NULL, c->key_indices_len, c->key_indices, 0, NULL, NULL));
        CHECK(expected || c->error == error);
        if (!expected) {
            continue;
        }

        expected = c->error != MUSIG_AGGNONCE;
        CHECK(expected == secp256k1_musig_aggnonce_parse(CTX, &aggnonce, vector->aggnonces[c->aggnonce_index]));
        if (!expected) {
            continue;
        }
        CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, vector->msgs[c->msg_index], &keyagg_cache));

        CHECK(secp256k1_ec_pubkey_parse(CTX, &pubkey, vector->pubkeys[0], sizeof(vector->pubkeys[0])));
        musig_test_set_secnonce(&secnonce, vector->secnonces[c->secnonce_index], &pubkey);
        expected = c->error != MUSIG_SECNONCE;
        if (expected) {
            CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig, &secnonce, &keypair, &keyagg_cache, &session));
        } else {
            CHECK_ILLEGAL(CTX, secp256k1_musig_partial_sign(CTX, &partial_sig, &secnonce, &keypair, &keyagg_cache, &session));
        }
    }
    for (i = 0; i < sizeof(vector->verify_fail_case)/sizeof(vector->verify_fail_case[0]); i++) {
        const struct musig_verify_fail_error_case *c = &vector->verify_fail_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        secp256k1_musig_aggnonce aggnonce;
        secp256k1_musig_session session;
        secp256k1_musig_partial_sig partial_sig;
        enum { NUM_PUBNONCES = 3 };
        secp256k1_musig_pubnonce pubnonce[NUM_PUBNONCES];
        const secp256k1_musig_pubnonce *pubnonce_ptr[NUM_PUBNONCES];
        secp256k1_pubkey pubkey;
        int expected;
        size_t j;

        CHECK(NUM_PUBNONCES <= c->nonce_indices_len);
        for (j = 0; j < c->nonce_indices_len; j++) {
            CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce[j], vector->pubnonces[c->nonce_indices[j]]));
            pubnonce_ptr[j] = &pubnonce[j];
        }

        CHECK(musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, NULL, c->key_indices_len, c->key_indices, 0, NULL, NULL));
        CHECK(secp256k1_musig_nonce_agg(CTX, &aggnonce, pubnonce_ptr, c->nonce_indices_len) == 1);
        CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, vector->msgs[c->msg_index], &keyagg_cache));

        CHECK(secp256k1_ec_pubkey_parse(CTX, &pubkey, vector->pubkeys[c->signer_index], sizeof(vector->pubkeys[0])));

        expected = c->error != MUSIG_SIG;
        CHECK(expected == secp256k1_musig_partial_sig_parse(CTX, &partial_sig, c->sig));
        if (!expected) {
            continue;
        }
        expected = c->error != MUSIG_SIG_VERIFY;
        CHECK(expected == secp256k1_musig_partial_sig_verify(CTX, &partial_sig, pubnonce, &pubkey, &keyagg_cache, &session));
    }
    for (i = 0; i < sizeof(vector->verify_error_case)/sizeof(vector->verify_error_case[0]); i++) {
        const struct musig_verify_fail_error_case *c = &vector->verify_error_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        secp256k1_musig_pubnonce pubnonce;
        int expected;

        expected = c->error != MUSIG_PUBKEY;
        CHECK(expected == musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, NULL, c->key_indices_len, c->key_indices, 0, NULL, NULL));
        CHECK(expected || c->error == error);
        if (!expected) {
            continue;
        }
        expected = c->error != MUSIG_PUBNONCE;
        CHECK(expected == secp256k1_musig_pubnonce_parse(CTX, &pubnonce, vector->pubnonces[c->nonce_indices[c->signer_index]]));
    }
}

static void musig_test_vectors_tweak(void) {
    size_t i;
    const struct musig_tweak_vector *vector = &musig_tweak_vector;
    secp256k1_pubkey pubkey;
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_secnonce secnonce;

    CHECK(secp256k1_musig_aggnonce_parse(CTX, &aggnonce, vector->aggnonce));
    CHECK(secp256k1_ec_pubkey_parse(CTX, &pubkey, vector->pubkeys[0], sizeof(vector->pubkeys[0])));

    for (i = 0; i < sizeof(vector->valid_case)/sizeof(vector->valid_case[0]); i++) {
        const struct musig_tweak_case *c = &vector->valid_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        secp256k1_musig_pubnonce pubnonce;
        secp256k1_musig_session session;
        secp256k1_musig_partial_sig partial_sig;
        secp256k1_keypair keypair;
        unsigned char partial_sig32[32];

        musig_test_set_secnonce(&secnonce, vector->secnonce, &pubkey);

        CHECK(secp256k1_keypair_create(CTX, &keypair, vector->sk));
        CHECK(musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, vector->tweaks, c->key_indices_len, c->key_indices, c->tweak_indices_len, c->tweak_indices, c->is_xonly));

        CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, vector->msg, &keyagg_cache));

        CHECK(secp256k1_musig_partial_sign(CTX, &partial_sig, &secnonce, &keypair, &keyagg_cache, &session));
        CHECK(secp256k1_musig_partial_sig_serialize(CTX, partial_sig32, &partial_sig));
        CHECK(secp256k1_memcmp_var(partial_sig32, c->expected, sizeof(partial_sig32)) == 0);

        CHECK(secp256k1_musig_pubnonce_parse(CTX, &pubnonce, vector->pubnonces[c->nonce_indices[c->signer_index]]));
        CHECK(secp256k1_musig_partial_sig_verify(CTX, &partial_sig, &pubnonce, &pubkey, &keyagg_cache, &session));
    }
    for (i = 0; i < sizeof(vector->error_case)/sizeof(vector->error_case[0]); i++) {
        const struct musig_tweak_case *c = &vector->error_case[i];
        enum MUSIG_ERROR error;
        secp256k1_musig_keyagg_cache keyagg_cache;
        CHECK(!musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, NULL, vector->pubkeys, vector->tweaks, c->key_indices_len, c->key_indices, c->tweak_indices_len, c->tweak_indices, c->is_xonly));
        CHECK(error == MUSIG_TWEAK);
    }
}

static void musig_test_vectors_sigagg(void) {
    size_t i, j;
    const struct musig_sig_agg_vector *vector = &musig_sig_agg_vector;

    for (i = 0; i < sizeof(vector->valid_case)/sizeof(vector->valid_case[0]); i++) {
        const struct musig_sig_agg_case *c = &vector->valid_case[i];
        enum MUSIG_ERROR error;
        unsigned char final_sig[64];
        secp256k1_musig_keyagg_cache keyagg_cache;
        unsigned char agg_pk32[32];
        secp256k1_xonly_pubkey agg_pk;
        secp256k1_musig_aggnonce aggnonce;
        secp256k1_musig_session session;
        secp256k1_musig_partial_sig partial_sig[(sizeof(vector->psigs)/sizeof(vector->psigs[0]))];
        const secp256k1_musig_partial_sig *partial_sig_ptr[(sizeof(vector->psigs)/sizeof(vector->psigs[0]))];

        CHECK(musig_vectors_keyagg_and_tweak(&error, &keyagg_cache, agg_pk32, vector->pubkeys, vector->tweaks, c->key_indices_len, c->key_indices, c->tweak_indices_len, c->tweak_indices, c->is_xonly));
        CHECK(secp256k1_musig_aggnonce_parse(CTX, &aggnonce, c->aggnonce));
        CHECK(secp256k1_musig_nonce_process(CTX, &session, &aggnonce, vector->msg, &keyagg_cache));
        for (j = 0; j < c->psig_indices_len; j++) {
            CHECK(secp256k1_musig_partial_sig_parse(CTX, &partial_sig[j], vector->psigs[c->psig_indices[j]]));
            partial_sig_ptr[j] = &partial_sig[j];
        }

        CHECK(secp256k1_musig_partial_sig_agg(CTX, final_sig, &session, partial_sig_ptr, c->psig_indices_len) == 1);
        CHECK(secp256k1_memcmp_var(final_sig, c->expected, sizeof(final_sig)) == 0);

        CHECK(secp256k1_xonly_pubkey_parse(CTX, &agg_pk, agg_pk32));
        CHECK(secp256k1_schnorrsig_verify(CTX, final_sig, vector->msg, sizeof(vector->msg), &agg_pk) == 1);
    }
    for (i = 0; i < sizeof(vector->error_case)/sizeof(vector->error_case[0]); i++) {
        const struct musig_sig_agg_case *c = &vector->error_case[i];
        secp256k1_musig_partial_sig partial_sig[(sizeof(vector->psigs)/sizeof(vector->psigs[0]))];
        for (j = 0; j < c->psig_indices_len; j++) {
            int expected = c->invalid_sig_idx != (int)j;
            CHECK(expected == secp256k1_musig_partial_sig_parse(CTX, &partial_sig[j], vector->psigs[c->psig_indices[j]]));
        }
    }
}

/* Since the BIP doesn't provide static test vectors for nonce_gen_counter, we
 * define a static test here */
static void musig_test_static_nonce_gen_counter(void) {
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char pubnonce66[66];
    secp256k1_pubkey pk;
    secp256k1_keypair keypair;
    uint64_t nonrepeating_cnt = 0;
    unsigned char sk[32] = {
        0xEE, 0xC1, 0xCB, 0x7D, 0x1B, 0x72, 0x54, 0xC5,
        0xCA, 0xB0, 0xD9, 0xC6, 0x1A, 0xB0, 0x2E, 0x64,
        0x3D, 0x46, 0x4A, 0x59, 0xFE, 0x6C, 0x96, 0xA7,
        0xEF, 0xE8, 0x71, 0xF0, 0x7C, 0x5A, 0xEF, 0x54,
    };
    unsigned char expected_secnonce[64] = {
        0x84, 0x2F, 0x13, 0x80, 0xCD, 0x17, 0xA1, 0x98,
        0xFC, 0x3D, 0xAD, 0x3B, 0x7D, 0xA7, 0x49, 0x29,
        0x41, 0xF4, 0x69, 0x76, 0xF2, 0x70, 0x2F, 0xF7,
        0xC6, 0x6F, 0x24, 0xF4, 0x72, 0x03, 0x6A, 0xF1,
        0xDA, 0x3F, 0x95, 0x2D, 0xDE, 0x4A, 0x2D, 0xA6,
        0xB6, 0x32, 0x57, 0x07, 0xCE, 0x87, 0xA4, 0xE3,
        0x61, 0x6D, 0x06, 0xFC, 0x5F, 0x81, 0xA9, 0xC9,
        0x93, 0x86, 0xD2, 0x0A, 0x99, 0xCE, 0xCF, 0x99,
    };
    unsigned char expected_pubnonce[66] = {
        0x03, 0xA5, 0xB9, 0xB6, 0x90, 0x79, 0x42, 0xEA,
        0xCD, 0xDA, 0x49, 0xA3, 0x66, 0x01, 0x6E, 0xC2,
        0xE6, 0x24, 0x04, 0xA1, 0xBF, 0x4A, 0xB6, 0xD4,
        0xDB, 0x82, 0x06, 0x7B, 0xC3, 0xAD, 0xF0, 0x86,
        0xD7, 0x03, 0x32, 0x05, 0xDB, 0x9E, 0xB3, 0x4D,
        0x5C, 0x7C, 0xE0, 0x28, 0x48, 0xCA, 0xC6, 0x8A,
        0x83, 0xED, 0x73, 0xE3, 0x88, 0x34, 0x77, 0xF5,
        0x63, 0xF2, 0x3C, 0xE9, 0xA1, 0x1A, 0x77, 0x21,
        0xEC, 0x64,
    };

    CHECK(secp256k1_keypair_create(CTX, &keypair, sk));
    CHECK(secp256k1_keypair_pub(CTX, &pk, &keypair));
    CHECK(secp256k1_musig_nonce_gen_counter(CTX, &secnonce, &pubnonce, nonrepeating_cnt, &keypair, NULL, NULL, NULL) == 1);

    CHECK(secp256k1_memcmp_var(&secnonce.data[4], expected_secnonce, 2*32) == 0);
    CHECK(secp256k1_memcmp_var(&secnonce.data[4+2*32], &pk, sizeof(pk)) == 0);

    CHECK(secp256k1_musig_pubnonce_serialize(CTX, pubnonce66, &pubnonce) == 1);
    CHECK(secp256k1_memcmp_var(pubnonce66, expected_pubnonce, sizeof(pubnonce66)) == 0);
}

static void run_musig_tests(void) {
    int i;

    for (i = 0; i < COUNT; i++) {
        musig_simple_test();
    }
    musig_api_tests();
    musig_nonce_test();
    for (i = 0; i < COUNT; i++) {
        /* Run multiple times to ensure that pk and nonce have different y
         * parities */
        musig_tweak_test();
    }
    sha256_tag_test();
    musig_test_vectors_keyagg();
    musig_test_vectors_noncegen();
    musig_test_vectors_nonceagg();
    musig_test_vectors_signverify();
    musig_test_vectors_tweak();
    musig_test_vectors_sigagg();

    musig_test_static_nonce_gen_counter();
}

#endif
