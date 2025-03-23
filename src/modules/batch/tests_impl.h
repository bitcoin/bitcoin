#ifndef SECP256K1_MODULE_BATCH_TESTS_H
#define SECP256K1_MODULE_BATCH_TESTS_H

#include "../../../include/secp256k1_batch.h"
#ifdef ENABLE_MODULE_SCHNORRSIG
#include "../../../include/secp256k1_schnorrsig.h"
#include "../../../include/secp256k1_schnorrsig_batch.h"
#endif
#ifdef ENABLE_MODULE_EXTRAKEYS
#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_tweak_check_batch.h"
#endif

/* Tests for the equality of two sha256 structs. This function only produces a
 * correct result if an integer multiple of 64 many bytes have been written
 * into the hash functions. */
void test_batch_sha256_eq(const secp256k1_sha256 *sha1, const secp256k1_sha256 *sha2) {
    /* Is buffer fully consumed? */
    CHECK((sha1->bytes & 0x3F) == 0);

    CHECK(sha1->bytes == sha2->bytes);
    CHECK(secp256k1_memcmp_var(sha1->s, sha2->s, sizeof(sha1->s)) == 0);
}

/* Checks that hash initialized by secp256k1_batch_sha256_tagged has the
 * expected state. */
void test_batch_sha256_tagged(void) {
    unsigned char tag[13] = "BIP0340/batch";
    secp256k1_sha256 sha;
    secp256k1_sha256 sha_optimized;

    secp256k1_sha256_initialize_tagged(&sha, (unsigned char *) tag, sizeof(tag));
    secp256k1_batch_sha256_tagged(&sha_optimized);
    test_batch_sha256_eq(&sha, &sha_optimized);
}

#define N_SIGS 10
#define N_TWK_CHECKS 10
#define N_TERMS (N_TWK_CHECKS + 2*N_SIGS)
void test_batch_api(void) {
    secp256k1_batch *batch_none;
    secp256k1_batch *batch_sign;
    secp256k1_batch *batch_vrfy;
    secp256k1_batch *batch_both;
    secp256k1_batch *batch_sttc;
    unsigned char aux_rand16[32];
    int ecount;

#ifdef ENABLE_MODULE_EXTRAKEYS
    unsigned char sk[32];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pk;
    /* xonly pubkey tweak checks data */
    unsigned char tweaked_pk[N_TWK_CHECKS][32];
    int tweaked_pk_parity[N_TWK_CHECKS];
    unsigned char tweak[N_TWK_CHECKS][32];
    secp256k1_pubkey tmp_pk;
    secp256k1_xonly_pubkey tmp_xonly_pk;
    size_t i;
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    /* schnorr verification data */
    unsigned char msg[N_SIGS][32];
    unsigned char sig[N_SIGS][64];
#endif
    /* context and batch setup */
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_context *vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_context *both = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_context *sttc = malloc(sizeof(*secp256k1_context_no_precomp));
    memcpy(sttc, secp256k1_context_no_precomp, sizeof(secp256k1_context));

    secp256k1_context_set_error_callback(none, counting_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sign, counting_callback_fn, &ecount);
    secp256k1_context_set_error_callback(vrfy, counting_callback_fn, &ecount);
    secp256k1_context_set_error_callback(both, counting_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sttc, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(none, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(vrfy, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(both, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sttc, counting_callback_fn, &ecount);

    /* 16 byte auxiliary randomness */
    testrand256(aux_rand16);
    memset(&aux_rand16[16], 0, 16);

#ifdef ENABLE_MODULE_EXTRAKEYS
    /* generate keypair data */
    testrand256(sk);
    CHECK(secp256k1_keypair_create(sign, &keypair, sk) == 1);
    CHECK(secp256k1_keypair_xonly_pub(sign, &pk, NULL, &keypair) == 1);

    /* generate N_TWK_CHECKS tweak check data (tweaked_pk, tweaked_pk_parity, tweak) */
    for (i = 0; i < N_TWK_CHECKS; i++) {
        testrand256(tweak[i]);
        CHECK(secp256k1_xonly_pubkey_tweak_add(vrfy, &tmp_pk, &pk, tweak[i]));
        CHECK(secp256k1_xonly_pubkey_from_pubkey(vrfy, &tmp_xonly_pk, &tweaked_pk_parity[i], &tmp_pk));
        CHECK(secp256k1_xonly_pubkey_serialize(vrfy, tweaked_pk[i], &tmp_xonly_pk));
        CHECK(secp256k1_xonly_pubkey_tweak_add_check(vrfy, tweaked_pk[i], tweaked_pk_parity[i], &pk, tweak[i]));
    }
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    /* generate N_SIGS schnorr verify data (msg, sig) */
    for (i = 0; i < N_SIGS; i++) {
        testrand256(msg[i]);
        CHECK(secp256k1_schnorrsig_sign32(sign, sig[i], msg[i], &keypair, NULL) == 1);
        CHECK(secp256k1_schnorrsig_verify(vrfy, sig[i], msg[i], sizeof(msg[i]), &pk));
    }
#endif

    /** main test body **/
    /* batch_create tests */
    ecount = 0;
    batch_none = secp256k1_batch_create(none, 1, NULL);
    CHECK(batch_none != NULL);
    CHECK(ecount == 0);
    /* 2*N_SIGS since one schnorrsig creates two scalar-point pair in batch */
    batch_sign = secp256k1_batch_create(sign, 2*N_SIGS, NULL);
    CHECK(batch_sign != NULL);
    CHECK(ecount == 0);
    batch_vrfy = secp256k1_batch_create(vrfy, N_TWK_CHECKS - 1, aux_rand16);
    CHECK(batch_vrfy != NULL);
    CHECK(ecount == 0);
    batch_both = secp256k1_batch_create(both, N_TERMS/4, aux_rand16);
    CHECK(batch_both != NULL);
    CHECK(ecount == 0);
    /* ARG_CHECK(max_terms != 0) in `batch_create` should fail*/
    batch_sttc = secp256k1_batch_create(sttc, 0, NULL);
    CHECK(batch_sttc == NULL);
    CHECK(ecount == 1);

#ifdef ENABLE_MODULE_SCHNORRSIG
    ecount = 0;
    for (i = 0; i < N_SIGS; i++) {
        CHECK(secp256k1_batch_usable(sign, batch_sign) == 1);
        CHECK(ecount == 0);
        CHECK(secp256k1_batch_add_schnorrsig(sign, batch_sign, sig[i], msg[i], sizeof(msg[i]), &pk) == 1);
        CHECK(ecount == 0);
    }
#endif

#ifdef ENABLE_MODULE_EXTRAKEYS
    ecount = 0;
    for (i = 0; i < N_TWK_CHECKS; i++) {
        CHECK(secp256k1_batch_usable(vrfy, batch_vrfy));
        CHECK(ecount == 0);
        CHECK(secp256k1_batch_add_xonlypub_tweak_check(vrfy, batch_vrfy, tweaked_pk[i], tweaked_pk_parity[i], &pk, tweak[i]));
        CHECK(ecount == 0);
    }
#endif

#if defined(ENABLE_MODULE_SCHNORRSIG) && defined(ENABLE_MODULE_EXTRAKEYS)
    /* secp256k1_batch_add_tests for batch_both */
    ecount = 0;
    for (i = 0; i < N_SIGS; i++) {
        CHECK(secp256k1_batch_usable(both, batch_both) == 1);
        CHECK(ecount == 0);
        CHECK(secp256k1_batch_add_schnorrsig(both, batch_both, sig[i], msg[i], sizeof(msg[i]), &pk) == 1);
        CHECK(ecount == 0);
    }
    for (i = 0; i < N_TWK_CHECKS; i++) {
        CHECK(secp256k1_batch_usable(both, batch_both));
        CHECK(ecount == 0);
        CHECK(secp256k1_batch_add_xonlypub_tweak_check(both, batch_both, tweaked_pk[i], tweaked_pk_parity[i], &pk, tweak[i]));
        CHECK(ecount == 0);
    }
#endif

    /* batch_verify tests */
    ecount = 0;
    CHECK(secp256k1_batch_verify(none, batch_none) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_verify(sign, batch_sign) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_verify(vrfy, batch_vrfy) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_verify(both, batch_both) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_verify(sttc, NULL) == 0);
    CHECK(ecount == 1);

    ecount = 0;
    secp256k1_batch_destroy(none, batch_none);
    CHECK(ecount == 0);
    secp256k1_batch_destroy(sign, batch_sign);
    CHECK(ecount == 0);
    secp256k1_batch_destroy(vrfy, batch_vrfy);
    CHECK(ecount == 0);
    secp256k1_batch_destroy(both, batch_both);
    CHECK(ecount == 0);
    secp256k1_batch_destroy(sttc, NULL);
    CHECK(ecount == 0);

    secp256k1_context_destroy(none);
    secp256k1_context_destroy(sign);
    secp256k1_context_destroy(vrfy);
    secp256k1_context_destroy(both);
    secp256k1_context_destroy(sttc);
}
#undef N_SIGS
#undef N_TWK_CHECKS
#undef N_TERMS


void run_batch_tests(void) {
    test_batch_api();
    test_batch_sha256_tagged();
}

#endif /* SECP256K1_MODULE_BATCH_TESTS_H */
