#ifndef SECP256K1_MODULE_SCHNORRSIG_BATCH_ADD_TESTS_IMPL_H
#define SECP256K1_MODULE_SCHNORRSIG_BATCH_ADD_TESTS_IMPL_H

#include "../../../include/secp256k1_schnorrsig.h"
#include "../../../include/secp256k1_batch.h"
#include "../../../include/secp256k1_schnorrsig_batch.h"
#include "../../unit_test.h"

/* Checks that a bit flip in the n_flip-th argument (that has n_bytes many
 * bytes) changes the hash function */
static void batch_schnorrsig_randomizer_gen_bitflip(secp256k1_sha256 *sha, unsigned char **args, size_t n_flip, size_t n_bytes, size_t msglen) {
    unsigned char randomizers[2][32];
    secp256k1_sha256 sha_cpy;
    sha_cpy = *sha;
    secp256k1_batch_schnorrsig_randomizer_gen(randomizers[0], &sha_cpy, args[0], args[1], msglen, args[2]);
    testrand_flip(args[n_flip], n_bytes);
    sha_cpy = *sha;
    secp256k1_batch_schnorrsig_randomizer_gen(randomizers[1], &sha_cpy, args[0], args[1], msglen, args[2]);
    CHECK(secp256k1_memcmp_var(randomizers[0], randomizers[1], 32) != 0);
}

static void run_batch_schnorrsig_randomizer_gen_tests(void) {
    secp256k1_sha256 sha;
    size_t n_sigs = 20;
    unsigned char msg[32];
    size_t msglen = sizeof(msg);
    unsigned char sig[64];
    unsigned char compressed_pk[33];
    unsigned char *args[3];
    size_t i; /* loops through n_sigs */
    int j; /* loops through count */

    secp256k1_batch_sha256_tagged(&sha);

    for (i = 0; i < n_sigs; i++) {
        uint8_t temp_rand;
        unsigned char randomizer[32];
        /* batch_schnorrsig_randomizer_gen func modifies the sha object passed
         * so, pass the copied obj instead of original */
        secp256k1_sha256 sha_cpy;

        /* generate i-th schnorrsig verify data */
        testrand256(msg);
        testrand256(&sig[0]);
        testrand256(&sig[32]);
        testrand256(&compressed_pk[1]);
        temp_rand = testrand_int(2) + 2; /* randomly choose 2 or 3 */
        compressed_pk[0] = (unsigned char)temp_rand;

        /* check that bitflip in an argument results in different nonces */
        args[0] = sig;
        args[1] = msg;
        args[2] = compressed_pk;

        for (j = 0; j < COUNT; j++) {
            batch_schnorrsig_randomizer_gen_bitflip(&sha, args, 0, 64, msglen);
            batch_schnorrsig_randomizer_gen_bitflip(&sha, args, 1, 32, msglen);
            batch_schnorrsig_randomizer_gen_bitflip(&sha, args, 2, 33, msglen);
        }

        /* different msglen should generate different randomizers */
        sha_cpy = sha;
        secp256k1_batch_schnorrsig_randomizer_gen(randomizer, &sha_cpy, sig, msg, msglen, compressed_pk);

        for (j = 0; j < COUNT; j++) {
            unsigned char randomizer2[32];
            uint32_t offset = testrand_int(msglen - 1);
            size_t msglen_tmp = (msglen + offset) % msglen;

            sha_cpy = sha;
            secp256k1_batch_schnorrsig_randomizer_gen(randomizer2, &sha_cpy, sig, msg, msglen_tmp, compressed_pk);
            CHECK(secp256k1_memcmp_var(randomizer, randomizer2, 32) != 0);
        }

        /* write i-th schnorrsig verify data to the sha object
         * this is required for generating the next randomizer */
        secp256k1_sha256_write(&sha, sig, 64);
        secp256k1_sha256_write(&sha, msg, msglen);
        secp256k1_sha256_write(&sha, compressed_pk, 33);
    }

}

/* Helper function that adds a schnorrsig to the batch context and
 * checks if it fails. Resets the batch at the end. */
static void check_batch_add_schnorrsig_verify_fails(secp256k1_batch *batch, unsigned char *sig64, unsigned char *msg, size_t msglen, secp256k1_xonly_pubkey *pk) {
    secp256k1_batch_add_schnorrsig(CTX, batch, sig64, msg, msglen, pk);
    CHECK(secp256k1_batch_verify(CTX, batch) == 0);
    secp256k1_batch_reset(CTX, batch);
}

#define N_SIGS 3
#define ONE_SIG 1
/* Creates N_SIGS valid signatures and verifies them with batch_verify.
 * Then flips some bits and checks that verification now fails. This is a
 * variation of `test_schnorrsig_sign_verify` (in schnorrsig/tests_impl.h) */
static void test_schnorrsig_sign_batch_verify_internal(void) {
    unsigned char sk[32];
    unsigned char msg[N_SIGS][32];
    unsigned char sig[N_SIGS][64];
    size_t i;
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pk;
    secp256k1_scalar s;
    secp256k1_batch *batch = secp256k1_batch_create(CTX, 2*N_SIGS, NULL);

    testrand256(sk);
    CHECK(secp256k1_keypair_create(CTX, &keypair, sk));
    CHECK(secp256k1_keypair_xonly_pub(CTX, &pk, NULL, &keypair));

    for (i = 0; i < N_SIGS; i++) {
        testrand256(msg[i]);
        CHECK(secp256k1_schnorrsig_sign32(CTX, sig[i], msg[i], &keypair, NULL));
        secp256k1_batch_add_schnorrsig(CTX, batch, sig[i], msg[i], sizeof(msg[i]), &pk);
    }
    CHECK(secp256k1_batch_verify(CTX, batch));
    secp256k1_batch_reset(CTX, batch);

    {
        /* Flip a few bits in the signature and in the message and check that
         * verify and verify_batch fail */
        size_t sig_idx = testrand_int(N_SIGS);
        size_t byte_idx = testrand_bits(5);
        unsigned char xorbyte = testrand_int(254)+1;

        sig[sig_idx][byte_idx] ^= xorbyte;
        check_batch_add_schnorrsig_verify_fails(batch, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk);
        sig[sig_idx][byte_idx] ^= xorbyte;

        byte_idx = testrand_bits(5);
        sig[sig_idx][32+byte_idx] ^= xorbyte;
        check_batch_add_schnorrsig_verify_fails(batch, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk);
        sig[sig_idx][32+byte_idx] ^= xorbyte;

        byte_idx = testrand_bits(5);
        msg[sig_idx][byte_idx] ^= xorbyte;
        check_batch_add_schnorrsig_verify_fails(batch, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk);
        msg[sig_idx][byte_idx] ^= xorbyte;

        /* Check that above bitflips have been reversed correctly */
        CHECK(secp256k1_schnorrsig_verify(CTX, sig[sig_idx], msg[sig_idx], sizeof(msg[sig_idx]), &pk));
    }

    /* Test overflowing s */
    CHECK(secp256k1_schnorrsig_sign32(CTX, sig[0], msg[0], &keypair, NULL));
    secp256k1_batch_add_schnorrsig(CTX, batch, sig[0], msg[0], sizeof(msg[0]), &pk);
    memset(&sig[0][32], 0xFF, 32);
    secp256k1_batch_add_schnorrsig(CTX, batch, sig[0], msg[0], sizeof(msg[0]), &pk);
    CHECK(secp256k1_batch_verify(CTX, batch) == 0);
    secp256k1_batch_reset(CTX, batch);

    /* Test negative s */
    CHECK(secp256k1_schnorrsig_sign32(CTX, sig[0], msg[0], &keypair, NULL));
    secp256k1_batch_add_schnorrsig(CTX, batch, sig[0], msg[0], sizeof(msg[0]), &pk);
    secp256k1_scalar_set_b32(&s, &sig[0][32], NULL);
    secp256k1_scalar_negate(&s, &s);
    secp256k1_scalar_get_b32(&sig[0][32], &s);
    secp256k1_batch_add_schnorrsig(CTX, batch, sig[0], msg[0], sizeof(msg[0]), &pk);
    CHECK(secp256k1_batch_verify(CTX, batch) == 0);
    secp256k1_batch_reset(CTX, batch);

    /* The empty message can be signed & verified */
    CHECK(secp256k1_schnorrsig_sign_custom(CTX, sig[0], NULL, 0, &keypair, NULL) == 1);
    secp256k1_batch_add_schnorrsig(CTX, batch, sig[0], NULL, 0, &pk);
    CHECK(secp256k1_batch_verify(CTX, batch) == 1);
    secp256k1_batch_reset(CTX, batch);

    {
        /* Test varying message lengths */
        unsigned char msg_large[32 * 8];
        uint32_t msglen  = testrand_int(sizeof(msg_large));
        for (i = 0; i < sizeof(msg_large); i += 32) {
            testrand256(&msg_large[i]);
        }
        CHECK(secp256k1_schnorrsig_sign_custom(CTX, sig[0], msg_large, msglen, &keypair, NULL) == 1);
        secp256k1_batch_add_schnorrsig(CTX, batch, sig[0], msg_large, msglen, &pk);
        CHECK(secp256k1_batch_verify(CTX, batch) == 1);
        /* batch_add fails for a random wrong message length */
        msglen = (msglen + (sizeof(msg_large) - 1)) % sizeof(msg_large);
        check_batch_add_schnorrsig_verify_fails(batch, sig[0], msg_large, msglen, &pk);
    }

    secp256k1_batch_destroy(CTX, batch);
}
#undef N_SIGS
#undef ONE_SIG

#define ONE_SIG 1
static void test_batch_add_schnorrsig_api(void) {
    unsigned char sk[32];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pk;
    secp256k1_xonly_pubkey zero_pk;
    unsigned char msg[32];
    unsigned char sig[64];
    unsigned char nullmsg_sig[64];

    secp256k1_batch *batch = secp256k1_batch_create(CTX, 2*ONE_SIG, NULL);

    /** generate keypair data **/
    testrand256(sk);
    CHECK(secp256k1_keypair_create(CTX, &keypair, sk) == 1);
    CHECK(secp256k1_keypair_xonly_pub(CTX, &pk, NULL, &keypair) == 1);
    memset(&zero_pk, 0, sizeof(zero_pk));
    /** generate a signature **/
    testrand256(msg);
    CHECK(secp256k1_schnorrsig_sign32(CTX, sig, msg, &keypair, NULL) == 1);
    CHECK(secp256k1_schnorrsig_verify(CTX, sig, msg, sizeof(msg), &pk));

    CHECK(batch != NULL);

    /** main test body **/
    secp256k1_batch_add_schnorrsig(CTX, batch, sig, msg, sizeof(msg), &pk);
    CHECK(secp256k1_batch_verify(CTX, batch) == 1);
    CHECK_ILLEGAL_VOID(CTX, secp256k1_batch_add_schnorrsig(CTX, batch, NULL, msg, sizeof(msg), &pk));
    CHECK_ILLEGAL_VOID(CTX, secp256k1_batch_add_schnorrsig(CTX, batch, sig, NULL, sizeof(msg), &pk));
    CHECK_ILLEGAL_VOID(CTX, secp256k1_batch_add_schnorrsig(CTX, batch, sig, msg, sizeof(msg), NULL));
    CHECK_ILLEGAL_VOID(CTX, secp256k1_batch_add_schnorrsig(CTX, batch, sig, msg, sizeof(msg), &zero_pk));
    CHECK_ILLEGAL_VOID(CTX, secp256k1_batch_add_schnorrsig(CTX, NULL, sig, msg, sizeof(msg), &pk));
    secp256k1_batch_reset(CTX, batch);

    /** NULL msg with valid signature **/
    CHECK(secp256k1_schnorrsig_sign_custom(CTX, nullmsg_sig, NULL, 0, &keypair, NULL) == 1);
    secp256k1_batch_add_schnorrsig(CTX, batch, nullmsg_sig, NULL, 0, &pk);
    CHECK(secp256k1_batch_verify(CTX, batch) == 1);
    secp256k1_batch_reset(CTX, batch);

    /** NULL msg with invalid signature **/
    check_batch_add_schnorrsig_verify_fails(batch, sig, NULL, 0, &pk);

    secp256k1_batch_destroy(CTX, batch);
}
#undef ONE_SIG

/* --- Test registry --- */
REPEAT_TEST(test_schnorrsig_sign_batch_verify)

static const struct tf_test_entry tests_batch_add_schnorrsig[] = {
    CASE(batch_schnorrsig_randomizer_gen_tests),
    CASE1(test_batch_add_schnorrsig_api),
};

#endif /* SECP256K1_MODULE_SCHNORRSIG_BATCH_ADD_TESTS_IMPL_H */
