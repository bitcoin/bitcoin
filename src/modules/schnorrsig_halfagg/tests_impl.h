#ifndef SECP256K1_MODULE_SCHNORRSIG_HALFAGG_TESTS_H
#define SECP256K1_MODULE_SCHNORRSIG_HALFAGG_TESTS_H

#include "../../../include/secp256k1_schnorrsig_halfagg.h"

#include "../../unit_test.h"

#include "vectors.h"

#define N_MAX 50

/* We test that the hash initialized by secp256k1_schnorrsig_sha256_tagged_aggregate
 * has the expected state. */
void test_schnorrsig_sha256_tagged_aggregate(void) {
    unsigned char tag[] = {'H', 'a', 'l', 'f', 'A', 'g', 'g', '/', 'r', 'a', 'n', 'd', 'o', 'm', 'i', 'z', 'e', 'r'};
    secp256k1_sha256 sha;
    secp256k1_sha256 sha_optimized;
    const secp256k1_hash_ctx *hash_ctx = &CTX->hash_ctx;

    secp256k1_sha256_initialize_tagged(hash_ctx, &sha, (unsigned char *) tag, sizeof(tag));
    secp256k1_schnorrsig_sha256_tagged_aggregation(&sha_optimized);
    test_sha256_eq(&sha, &sha_optimized);
}

/* Create n many x-only pubkeys and sigs for random messages */
void test_schnorrsig_aggregate_input_helper(secp256k1_xonly_pubkey *pubkeys, unsigned char *msgs32, unsigned char *sigs64, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) {
        unsigned char sk[32];
        secp256k1_keypair keypair;
        testrand256(sk);
        testrand256(&msgs32[i*32]);

        CHECK(secp256k1_keypair_create(CTX, &keypair, sk));
        CHECK(secp256k1_keypair_xonly_pub(CTX, &pubkeys[i], NULL, &keypair));
        CHECK(secp256k1_schnorrsig_sign32(CTX, &sigs64[i*64], &msgs32[i*32], &keypair, NULL));
    }
}

/* In this test we create a bunch of Schnorr signatures,
 * aggregate some of them in one shot, and then
 * aggregate the others incrementally to the already aggregated ones.
 * The aggregate signature should verify after both steps. */
void test_schnorrsig_aggregate_internal(void) {
    secp256k1_xonly_pubkey pubkeys[N_MAX];
    unsigned char msgs32[N_MAX*32];
    unsigned char sigs64[N_MAX*64];
    unsigned char aggsig[32*(N_MAX + 1) + 17];
    size_t aggsig_len = sizeof(aggsig);

    size_t n = testrand_int(N_MAX + 1);
    size_t n_initial = testrand_int(n + 1);
    size_t n_new = n - n_initial;
    test_schnorrsig_aggregate_input_helper(pubkeys, msgs32, sigs64, n);

    /* Aggregate the first n_initial of them */
    CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, sigs64, n_initial));
    /* Make sure that the aggregate signature verifies */
    CHECK(aggsig_len == 32*(n_initial + 1));
    CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n_initial, aggsig, aggsig_len));
    /* Aggregate the remaining n_new many signatures to the already existing ones */
    aggsig_len = sizeof(aggsig);
    CHECK(secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, n_new));
    /* Make sure that the aggregate signature verifies */
    CHECK(aggsig_len == 32*(n + 1));
    CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len));

    /* Check that a direct aggregation of the n sigs yields an identical aggsig */
    {
        unsigned char aggsig2[sizeof(aggsig)];
        size_t aggsig_len2 = sizeof(aggsig2);
        CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig2, &aggsig_len2, pubkeys, msgs32, sigs64, n));
        CHECK(aggsig_len == aggsig_len2);
        CHECK(secp256k1_memcmp_var(aggsig, aggsig2, aggsig_len) == 0);
    }
}

/* Tests secp256k1_schnorrsig_aggregate against the half-aggregation test vectors. */
void test_schnorrsig_aggregate_vectors(void) {
    size_t i;
    for (i = 0; i < sizeof(halfagg_aggregate_vectors)/sizeof(halfagg_aggregate_vectors[0]); ++i) {
        const struct halfagg_aggregate_vector *v = &halfagg_aggregate_vectors[i];
        secp256k1_xonly_pubkey pubkeys[HALFAGG_VECTOR_MAX_N];
        unsigned char aggsig[HALFAGG_VECTOR_MAX_AGGSIG_LEN];
        size_t aggsig_len = sizeof(aggsig);
        size_t j;

        for (j = 0; j < v->n; ++j) {
            CHECK(secp256k1_xonly_pubkey_parse(CTX, &pubkeys[j], v->pubkeys[j]));
        }

        if (v->expected_result) {
            CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, &v->msgs[0][0], &v->sigs[0][0], v->n));
            CHECK(aggsig_len == v->expected_aggsig_len);
            CHECK(secp256k1_memcmp_var(aggsig, v->expected_aggsig, aggsig_len) == 0);
        } else {
            CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, &v->msgs[0][0], &v->sigs[0][0], v->n) == 0);
        }
    }
}

/* Tests secp256k1_schnorrsig_inc_aggregate against the half-aggregation test vectors. */
void test_schnorrsig_incaggregate_vectors(void) {
    size_t i;
    for (i = 0; i < sizeof(halfagg_incaggregate_vectors)/sizeof(halfagg_incaggregate_vectors[0]); ++i) {
        const struct halfagg_incaggregate_vector *v = &halfagg_incaggregate_vectors[i];
        secp256k1_xonly_pubkey pubkeys[HALFAGG_VECTOR_MAX_N];
        unsigned char aggsig[HALFAGG_VECTOR_MAX_AGGSIG_LEN];
        size_t aggsig_len = sizeof(aggsig);
        size_t n_total = v->n_before + v->n_new;
        size_t j;

        for (j = 0; j < n_total; ++j) {
            CHECK(secp256k1_xonly_pubkey_parse(CTX, &pubkeys[j], v->pubkeys[j]));
        }
        /* The input aggregate signature is provided in the same buffer that
         * receives the (larger) output aggregate signature. */
        memcpy(aggsig, v->aggsig_in, v->aggsig_in_len);

        if (v->expected_result) {
            CHECK(secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, &v->msgs[0][0], &v->new_sigs[0][0], v->n_before, v->n_new));
            CHECK(aggsig_len == v->expected_aggsig_len);
            CHECK(secp256k1_memcmp_var(aggsig, v->expected_aggsig, aggsig_len) == 0);
        } else {
            CHECK(secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, &v->msgs[0][0], &v->new_sigs[0][0], v->n_before, v->n_new) == 0);
        }
    }
}

/* Tests secp256k1_schnorrsig_aggverify against the half-aggregation test vectors. */
void test_schnorrsig_aggverify_vectors(void) {
    size_t i;
    for (i = 0; i < sizeof(halfagg_verify_vectors)/sizeof(halfagg_verify_vectors[0]); ++i) {
        const struct halfagg_verify_vector *v = &halfagg_verify_vectors[i];
        secp256k1_xonly_pubkey pubkeys[HALFAGG_VECTOR_MAX_N];
        size_t j;
        int pubkeys_ok = 1;

        for (j = 0; j < v->n; ++j) {
            if (!secp256k1_xonly_pubkey_parse(CTX, &pubkeys[j], v->pubkeys[j])) {
                pubkeys_ok = 0;
                break;
            }
        }
        if (!pubkeys_ok) {
            /* A public key that fails to parse cannot be verified, so the
             * vector must expect failure. */
            CHECK(!v->expected_result);
            continue;
        }
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, &v->msgs[0][0], v->n, v->aggsig, v->aggsig_len) == v->expected_result);
    }
}

static void test_schnorrsig_aggregate_api_internal(void) {
    size_t n = testrand_int(N_MAX + 1);
    size_t n_initial = testrand_int(n + 1);
    size_t n_new = n - n_initial;

    /* Test preparation. */
    secp256k1_xonly_pubkey pubkeys[N_MAX];
    unsigned char msgs32[N_MAX*32];
    unsigned char sigs64[N_MAX*64];
    unsigned char aggsig[32*(N_MAX + 1)];
    test_schnorrsig_aggregate_input_helper(pubkeys, msgs32, sigs64, n);

    /* Test body 1: Check API of function aggregate. */
    {
        /* Should not accept NULL for aggsig or aggsig length */
        size_t aggsig_len = sizeof(aggsig);
        CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggregate(CTX, NULL, &aggsig_len, pubkeys, msgs32, sigs64, n_initial));
        CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggregate(CTX, aggsig, NULL, pubkeys, msgs32, sigs64, n_initial));
        /* Should not accept NULL for keys, messages, or signatures if n_initial is not 0 */
        if (n_initial != 0) {
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, NULL, msgs32, sigs64, n_initial));
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, NULL, sigs64, n_initial));
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, NULL, n_initial));
        }
    }

    /* Test body 2: Check API of function inc_aggregate. */
    {
        size_t aggsig_len = sizeof(aggsig);
        CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, sigs64, n_initial));
        aggsig_len = 32*(n+1);
        /* Should not accept NULL for aggsig or aggsig length */
        CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, NULL, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, n_new));
        CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, NULL, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, n_new));
        /* Should not accept NULL for keys or messages if n is not 0 */
        if (n != 0) {
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, NULL, msgs32, &sigs64[n_initial*64], n_initial, n_new));
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, NULL, &sigs64[n_initial*64], n_initial, n_new));
        }
        /* Should not accept NULL for new_sigs64 if n_new is not 0 */
        if (n_new != 0) {
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, NULL, n_initial, n_new));
        }
        /* Should not accept overflowing number of sigs. */
        CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], SIZE_MAX, SIZE_MAX));
        if (n_initial > 0) {
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, SIZE_MAX));
        }
        /* Should reject if aggsig_len is too small. */
        aggsig_len = 32*n;
        CHECK(secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, n_new) == 0);
        aggsig_len = 32*(n+1) - 1;
        CHECK(secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, n_new) == 0);
    }

    /* Test body 3: Check API of function aggverify. */
    {
        size_t aggsig_len = sizeof(aggsig);
        CHECK(secp256k1_schnorrsig_inc_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[n_initial*64], n_initial, n_new));
        /* aggverify operates only on public data, so it works with the static
         * context (unlike signing/keygen it needs no ecmult_gen context). */
        CHECK(secp256k1_schnorrsig_aggverify(STATIC_CTX, pubkeys, msgs32, n, aggsig, aggsig_len) == 1);
        /* Should not accept NULL for keys or messages if n is not 0 */
        if (n != 0) {
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggverify(CTX, NULL, msgs32, n, aggsig, aggsig_len));
            CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggverify(CTX, pubkeys, NULL, n, aggsig, aggsig_len));
        }
        /* Should never accept NULL the aggsig */
        CHECK_ILLEGAL(CTX, secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, NULL, aggsig_len));
        /* Should reject for invalid aggsig_len. */
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len + 1) == 0);
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len - 1) == 0);
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len + 32) == 0);
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len - 32) == 0);
    }
}

/* In this test, we make sure that trivial attempts to break
 * the security of verification do not work. */
static void test_schnorrsig_aggregate_unforge_internal(void) {
    secp256k1_xonly_pubkey pubkeys[N_MAX];
    unsigned char msgs32[N_MAX*32];
    unsigned char sigs64[N_MAX*64];
    unsigned char aggsig[32*(N_MAX + 1)];

    size_t n = testrand_int(N_MAX + 1);

    /* Test 1: We fix a set of n messages and compute
     * a random aggsig for them. This should not verify. */
    test_schnorrsig_aggregate_input_helper(pubkeys, msgs32, sigs64, n);
    {
        size_t aggsig_len = sizeof(aggsig);
        size_t i;
        /* Sample aggsig randomly */
        for (i = 0; i < n + 1; ++i) {
            testrand256(&aggsig[i*32]);
        }
        /* Make sure that it does not verify */
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len) == 0);
    }

    /* Test 2: We fix a set of n messages and compute valid
     * signatures for all but one. The resulting aggregate signature
     * should not verify. */
    test_schnorrsig_aggregate_input_helper(pubkeys, msgs32, sigs64, n);
    if (n > 0) {
        size_t aggsig_len = sizeof(aggsig);
        /* Replace a randomly chosen real sig with a random one. */
        size_t k = testrand_int(n);
        testrand256(&sigs64[k*64]);
        testrand256(&sigs64[k*64+32]);
        /* Aggregate the n signatures */
        CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, sigs64, n));
        /* Make sure the result does not verify */
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len) == 0);
    }

    /* Test 3: We generate a valid aggregate signature and then
     * change one of the messages. This should not verify. */
    test_schnorrsig_aggregate_input_helper(pubkeys, msgs32, sigs64, n);
    if (n > 0) {
        size_t aggsig_len = sizeof(aggsig);
        size_t k;
        /* Aggregate the n signatures */
        CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, sigs64, n));
        /* Change one of the messages */
        k = testrand_int(32*n);
        msgs32[k] = msgs32[k]^0xff;
        /* Make sure the result does not verify */
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len) == 0);
    }
}

/* In this test, we make sure that the algorithms properly reject
 * for overflowing and non parseable values. */
static void test_schnorrsig_aggregate_overflow_internal(void) {
    secp256k1_xonly_pubkey pubkeys[N_MAX];
    unsigned char msgs32[N_MAX*32];
    unsigned char sigs64[N_MAX*64];
    unsigned char aggsig[32*(N_MAX + 1)];
    size_t n = testrand_int(N_MAX + 1);

    /* We check that verification returns 0 if the s in aggsig overflows. */
    test_schnorrsig_aggregate_input_helper(pubkeys, msgs32, sigs64, n);
    {
        size_t aggsig_len = sizeof(aggsig);
        /* Aggregate */
        CHECK(secp256k1_schnorrsig_aggregate(CTX, aggsig, &aggsig_len, pubkeys, msgs32, sigs64, n));
        /* Make s in the aggsig overflow */
        memset(&aggsig[n*32], 0xFF, 32);
        /* Should not verify */
        CHECK(secp256k1_schnorrsig_aggverify(CTX, pubkeys, msgs32, n, aggsig, aggsig_len) == 0);
    }
}

/* --- Test registry --- */
REPEAT_TEST(test_schnorrsig_aggregate)
REPEAT_TEST(test_schnorrsig_aggregate_api)
REPEAT_TEST(test_schnorrsig_aggregate_unforge)
REPEAT_TEST(test_schnorrsig_aggregate_overflow)

static const struct tf_test_entry tests_schnorrsig_halfagg[] = {
    CASE1(test_schnorrsig_sha256_tagged_aggregate),
    CASE1(test_schnorrsig_aggregate_vectors),
    CASE1(test_schnorrsig_incaggregate_vectors),
    CASE1(test_schnorrsig_aggverify_vectors),
    CASE1(test_schnorrsig_aggregate),
    CASE1(test_schnorrsig_aggregate_api),
    CASE1(test_schnorrsig_aggregate_unforge),
    CASE1(test_schnorrsig_aggregate_overflow),
};

#undef N_MAX

#endif
