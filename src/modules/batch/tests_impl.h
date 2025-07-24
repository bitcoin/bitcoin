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
    unsigned char tag[] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'b', 'a', 't', 'c', 'h'};
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
    /* batch object setup */
    secp256k1_batch *batch;
    unsigned char aux_rand16[32];

    /* 16 byte auxiliary randomness */
    testrand256(aux_rand16);
    memset(&aux_rand16[16], 0, 16);

    /** main test body **/
    /* batch_create tests */
    batch = secp256k1_batch_create(CTX, N_TERMS, aux_rand16);
    CHECK(batch != NULL);
    /* ARG_CHECK(max_terms != 0) in `batch_create` should fail*/
    CHECK_ILLEGAL(CTX, secp256k1_batch_create(CTX, 0, NULL));

#ifdef ENABLE_MODULE_EXTRAKEYS
    /* generate keypair data */
    testrand256(sk);
    CHECK(secp256k1_keypair_create(CTX, &keypair, sk) == 1);
    CHECK(secp256k1_keypair_xonly_pub(CTX, &pk, NULL, &keypair) == 1);

    /* generate N_TWK_CHECKS tweak check data (tweaked_pk, tweaked_pk_parity, tweak) */
    for (i = 0; i < N_TWK_CHECKS; i++) {
        testrand256(tweak[i]);
        CHECK(secp256k1_xonly_pubkey_tweak_add(CTX, &tmp_pk, &pk, tweak[i]));
        CHECK(secp256k1_xonly_pubkey_from_pubkey(CTX, &tmp_xonly_pk, &tweaked_pk_parity[i], &tmp_pk));
        CHECK(secp256k1_xonly_pubkey_serialize(CTX, tweaked_pk[i], &tmp_xonly_pk));
        CHECK(secp256k1_xonly_pubkey_tweak_add_check(CTX, tweaked_pk[i], tweaked_pk_parity[i], &pk, tweak[i]));
    }
    /* add tweak checks to the batch object */
    for (i = 0; i < N_TWK_CHECKS; i++) {
        CHECK(secp256k1_batch_usable(CTX, batch));
        CHECK(secp256k1_batch_add_xonlypub_tweak_check(CTX, batch, tweaked_pk[i], tweaked_pk_parity[i], &pk, tweak[i]));
    }
#endif

#ifdef ENABLE_MODULE_SCHNORRSIG
    /* generate N_SIGS schnorr signatures */
    for (i = 0; i < N_SIGS; i++) {
        testrand256(msg[i]);
        CHECK(secp256k1_schnorrsig_sign32(CTX, sig[i], msg[i], &keypair, NULL) == 1);
        CHECK(secp256k1_schnorrsig_verify(CTX, sig[i], msg[i], sizeof(msg[i]), &pk));
    }
    /* add schnorrsigs to the batch object */
    for (i = 0; i < N_SIGS; i++) {
        CHECK(secp256k1_batch_usable(CTX, batch) == 1);
        CHECK(secp256k1_batch_add_schnorrsig(CTX, batch, sig[i], msg[i], sizeof(msg[i]), &pk) == 1);
    }
#endif

    /* batch_verify tests */
    CHECK(secp256k1_batch_verify(CTX, batch) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_batch_verify(CTX, NULL));

    secp256k1_batch_destroy(CTX, batch);
}
#undef N_SIGS
#undef N_TWK_CHECKS
#undef N_TERMS


void run_batch_tests(void) {
    test_batch_api();
    test_batch_sha256_tagged();
}

#endif /* SECP256K1_MODULE_BATCH_TESTS_H */
