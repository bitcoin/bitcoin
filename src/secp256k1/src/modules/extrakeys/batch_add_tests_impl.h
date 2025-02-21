#ifndef SECP256K1_MODULE_EXTRAKEYS_BATCH_ADD_TESTS_IMPL_H
#define SECP256K1_MODULE_EXTRAKEYS_BATCH_ADD_TESTS_IMPL_H

#include "../../../include/secp256k1_extrakeys.h"
#include "../../../include/secp256k1_batch.h"
#include "../../../include/secp256k1_tweak_check_batch.h"

/* Checks that a bit flip in the n_flip-th argument (that has n_bytes many
 * bytes) changes the hash function */
void batch_xonlypub_tweak_randomizer_gen_bitflip(secp256k1_sha256 *sha, unsigned char **args, size_t n_flip, size_t n_bytes) {
    unsigned char randomizers[2][32];
    secp256k1_sha256 sha_cpy;
    sha_cpy = *sha;
    secp256k1_batch_xonlypub_tweak_randomizer_gen(randomizers[0], &sha_cpy, args[0], args[1], args[2], args[3]);
    testrand_flip(args[n_flip], n_bytes);
    sha_cpy = *sha;
    secp256k1_batch_xonlypub_tweak_randomizer_gen(randomizers[1], &sha_cpy, args[0], args[1], args[2], args[3]);
    CHECK(secp256k1_memcmp_var(randomizers[0], randomizers[1], 32) != 0);
}

void run_batch_xonlypub_tweak_randomizer_gen_tests(void) {
    secp256k1_sha256 sha;
    size_t n_checks = 20;
    unsigned char tweaked_pk[32];
    unsigned char tweaked_pk_parity;
    unsigned char tweak[32];
    unsigned char internal_pk[33];
    unsigned char *args[4];
    size_t i; /* loops through n_checks */
    int j; /* loops through count */

    secp256k1_batch_sha256_tagged(&sha);

    for (i = 0; i < n_checks; i++) {
        uint8_t temp_rand;

        /* generate i-th tweak check data */
        testrand256(tweaked_pk);
        tweaked_pk_parity = (unsigned char) testrand_int(2);
        testrand256(tweak);
        testrand256(&internal_pk[1]);
        temp_rand = testrand_int(2) + 2; /* randomly choose 2 or 3 */
        internal_pk[0] = (unsigned char)temp_rand;

        /* check bitflip in any argument results in generates randomizers */
        args[0] = tweaked_pk;
        args[1] = &tweaked_pk_parity;
        args[2] = internal_pk;
        args[3] = tweak;

        for (j = 0; j < COUNT; j++) {
            batch_xonlypub_tweak_randomizer_gen_bitflip(&sha, args, 0, 32);
            batch_xonlypub_tweak_randomizer_gen_bitflip(&sha, args, 1, 1);
            batch_xonlypub_tweak_randomizer_gen_bitflip(&sha, args, 2, 33);
            batch_xonlypub_tweak_randomizer_gen_bitflip(&sha, args, 3, 32);
        }

        /* write i-th tweak check data to the sha object
         * this is required for generating the next randomizer */
        secp256k1_sha256_write(&sha, tweaked_pk, 32);
        secp256k1_sha256_write(&sha, &tweaked_pk_parity, 1);
        secp256k1_sha256_write(&sha, tweak, 32);
        secp256k1_sha256_write(&sha, internal_pk, 33);
    }

}

void test_batch_add_xonlypub_tweak_api(void) {
    unsigned char sk[32];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey pk;
    /* xonly pubkey tweak checks data */
    unsigned char tweaked_pk[32];
    int tweaked_pk_parity;
    unsigned char tweak[32];
    secp256k1_pubkey tmp_pk;
    secp256k1_xonly_pubkey tmp_xonly_pk;
    unsigned char overflows[32];

    /** setup **/
    secp256k1_context *none = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_context *vrfy = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
    secp256k1_batch *batch1 = secp256k1_batch_create(none, 1, NULL);
    /* batch2 is used when batch_add_xonlypub_tweak is expected to fail */
    secp256k1_batch *batch2 = secp256k1_batch_create(none, 1, NULL);
    int ecount;

    secp256k1_context_set_error_callback(none, counting_callback_fn, &ecount);
    secp256k1_context_set_error_callback(sign, counting_callback_fn, &ecount);
    secp256k1_context_set_error_callback(vrfy, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(none, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(sign, counting_callback_fn, &ecount);
    secp256k1_context_set_illegal_callback(vrfy, counting_callback_fn, &ecount);

    /** generate keypair data **/
    testrand256(sk);
    CHECK(secp256k1_keypair_create(sign, &keypair, sk) == 1);
    CHECK(secp256k1_keypair_xonly_pub(sign, &pk, NULL, &keypair) == 1);
    memset(overflows, 0xFF, sizeof(overflows));

    /** generate tweak check data (tweaked_pk, tweaked_pk_parity, tweak) **/
    testrand256(tweak);
    CHECK(secp256k1_xonly_pubkey_tweak_add(vrfy, &tmp_pk, &pk, tweak));
    CHECK(secp256k1_xonly_pubkey_from_pubkey(vrfy, &tmp_xonly_pk, &tweaked_pk_parity, &tmp_pk));
    CHECK(secp256k1_xonly_pubkey_serialize(vrfy, tweaked_pk, &tmp_xonly_pk));
    CHECK(secp256k1_xonly_pubkey_tweak_add_check(vrfy, tweaked_pk, tweaked_pk_parity, &pk, tweak));

    CHECK(batch1 != NULL);
    CHECK(batch2 != NULL);

    /** main test body **/
    ecount = 0;
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch1, tweaked_pk, tweaked_pk_parity, &pk, tweak) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_verify(none, batch1) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, NULL, tweaked_pk_parity, &pk, tweak) == 0);
    CHECK(ecount == 1);
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, tweaked_pk, tweaked_pk_parity, NULL, tweak) == 0);
    CHECK(ecount == 2);
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, tweaked_pk, tweaked_pk_parity, &pk, NULL) == 0);
    CHECK(ecount == 3);
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, NULL, tweaked_pk, tweaked_pk_parity, &pk, tweak) == 0);
    CHECK(ecount == 4);
    /** overflowing tweak not allowed **/
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, tweaked_pk, tweaked_pk_parity, &pk, overflows) == 0);
    CHECK(ecount == 4);
    /** x-coordinate of tweaked pubkey should be less than prime order **/
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, overflows, tweaked_pk_parity, &pk, tweak) == 0);
    CHECK(ecount == 4);

    /** batch_verify should fail for incorrect tweak **/
    ecount = 0;
    CHECK(secp256k1_batch_usable(none, batch2));
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, tweaked_pk, !tweaked_pk_parity, &pk, tweak) == 1);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_verify(none, batch2) == 0);
    CHECK(ecount == 0);

    /** batch_add_ should ignore unusable batch object (i.e, batch->result = 0) **/
    ecount = 0;
    CHECK(secp256k1_batch_usable(none, batch2) == 0);
    CHECK(ecount == 0);
    CHECK(secp256k1_batch_add_xonlypub_tweak_check(none, batch2, tweaked_pk, tweaked_pk_parity, &pk, tweak) == 0);
    CHECK(ecount == 0);

    ecount = 0;
    secp256k1_batch_destroy(none, batch1);
    CHECK(ecount == 0);
    secp256k1_batch_destroy(none, batch2);
    CHECK(ecount == 0);

    secp256k1_context_destroy(none);
    secp256k1_context_destroy(sign);
    secp256k1_context_destroy(vrfy);
}

void run_batch_add_xonlypub_tweak_tests(void) {
    run_batch_xonlypub_tweak_randomizer_gen_tests();
    test_batch_add_xonlypub_tweak_api();
}


#endif /* SECP256K1_MODULE_EXTRAKEYS_BATCH_ADD_TESTS_IMPL_H */
