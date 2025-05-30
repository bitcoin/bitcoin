/***********************************************************************
 * Copyright (c) 2013-2015 Pieter Wuille                               *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_RECOVERY_TESTS_H
#define SECP256K1_MODULE_RECOVERY_TESTS_H

static int recovery_test_nonce_function(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
    (void) msg32;
    (void) key32;
    (void) algo16;
    (void) data;

    /* On the first run, return 0 to force a second run */
    if (counter == 0) {
        memset(nonce32, 0, 32);
        return 1;
    }
    /* On the second run, return an overflow to force a third run */
    if (counter == 1) {
        memset(nonce32, 0xff, 32);
        return 1;
    }
    /* On the next run, return a valid nonce, but flip a coin as to whether or not to fail signing. */
    memset(nonce32, 1, 32);
    return testrand_bits(1);
}

static void test_ecdsa_recovery_api(void) {
    /* Setup contexts that just count errors */
    secp256k1_pubkey pubkey;
    secp256k1_pubkey recpubkey;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_ecdsa_recoverable_signature recsig;
    unsigned char privkey[32] = { 1 };
    unsigned char message[32] = { 2 };
    int recid = 0;
    unsigned char sig[74];
    unsigned char zero_privkey[32] = { 0 };
    unsigned char over_privkey[32] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    /* Construct and verify corresponding public key. */
    CHECK(secp256k1_ec_seckey_verify(CTX, privkey) == 1);
    CHECK(secp256k1_ec_pubkey_create(CTX, &pubkey, privkey) == 1);

    /* Check bad contexts and NULLs for signing */
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, privkey, NULL, NULL) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_sign_recoverable(CTX, NULL, message, privkey, NULL, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_sign_recoverable(CTX, &recsig, NULL, privkey, NULL, NULL));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, NULL, NULL, NULL));
    CHECK_ILLEGAL(STATIC_CTX, secp256k1_ecdsa_sign_recoverable(STATIC_CTX, &recsig, message, privkey, NULL, NULL));
    /* This will fail or succeed randomly, and in either case will not ARG_CHECK failure */
    secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, privkey, recovery_test_nonce_function, NULL);
    /* These will all fail, but not in ARG_CHECK way */
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, zero_privkey, NULL, NULL) == 0);
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, over_privkey, NULL, NULL) == 0);
    /* This one will succeed. */
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, privkey, NULL, NULL) == 1);

    /* Check signing with a goofy nonce function */

    /* Check bad contexts and NULLs for recovery */
    CHECK(secp256k1_ecdsa_recover(CTX, &recpubkey, &recsig, message) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recover(CTX, NULL, &recsig, message));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recover(CTX, &recpubkey, NULL, message));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recover(CTX, &recpubkey, &recsig, NULL));

    /* Check NULLs for conversion */
    CHECK(secp256k1_ecdsa_sign(CTX, &normal_sig, message, privkey, NULL, NULL) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_convert(CTX, NULL, &recsig));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_convert(CTX, &normal_sig, NULL));
    CHECK(secp256k1_ecdsa_recoverable_signature_convert(CTX, &normal_sig, &recsig) == 1);

    /* Check NULLs for de/serialization */
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &recsig, message, privkey, NULL, NULL) == 1);
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_serialize_compact(CTX, NULL, &recid, &recsig));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_serialize_compact(CTX, sig, NULL, &recsig));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_serialize_compact(CTX, sig, &recid, NULL));
    CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(CTX, sig, &recid, &recsig) == 1);

    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, NULL, sig, recid));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &recsig, NULL, recid));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &recsig, sig, -1));
    CHECK_ILLEGAL(CTX, secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &recsig, sig, 5));
    /* overflow in signature will not result in calling illegal_callback */
    memcpy(sig, over_privkey, 32);
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &recsig, sig, recid) == 0);
}

static void test_ecdsa_recovery_end_to_end(void) {
    unsigned char extra[32] = {0x00};
    unsigned char privkey[32];
    unsigned char message[32];
    secp256k1_ecdsa_signature signature[5];
    secp256k1_ecdsa_recoverable_signature rsignature[5];
    unsigned char sig[74];
    secp256k1_pubkey pubkey;
    secp256k1_pubkey recpubkey;
    int recid = 0;

    /* Generate a random key and message. */
    {
        secp256k1_scalar msg, key;
        testutil_random_scalar_order_test(&msg);
        testutil_random_scalar_order_test(&key);
        secp256k1_scalar_get_b32(privkey, &key);
        secp256k1_scalar_get_b32(message, &msg);
    }

    /* Construct and verify corresponding public key. */
    CHECK(secp256k1_ec_seckey_verify(CTX, privkey) == 1);
    CHECK(secp256k1_ec_pubkey_create(CTX, &pubkey, privkey) == 1);

    /* Serialize/parse compact and verify/recover. */
    extra[0] = 0;
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &rsignature[0], message, privkey, NULL, NULL) == 1);
    CHECK(secp256k1_ecdsa_sign(CTX, &signature[0], message, privkey, NULL, NULL) == 1);
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &rsignature[4], message, privkey, NULL, NULL) == 1);
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &rsignature[1], message, privkey, NULL, extra) == 1);
    extra[31] = 1;
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &rsignature[2], message, privkey, NULL, extra) == 1);
    extra[31] = 0;
    extra[0] = 1;
    CHECK(secp256k1_ecdsa_sign_recoverable(CTX, &rsignature[3], message, privkey, NULL, extra) == 1);
    CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(CTX, sig, &recid, &rsignature[4]) == 1);
    CHECK(secp256k1_ecdsa_recoverable_signature_convert(CTX, &signature[4], &rsignature[4]) == 1);
    CHECK(secp256k1_memcmp_var(&signature[4], &signature[0], 64) == 0);
    CHECK(secp256k1_ecdsa_verify(CTX, &signature[4], message, &pubkey) == 1);
    memset(&rsignature[4], 0, sizeof(rsignature[4]));
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsignature[4], sig, recid) == 1);
    CHECK(secp256k1_ecdsa_recoverable_signature_convert(CTX, &signature[4], &rsignature[4]) == 1);
    CHECK(secp256k1_ecdsa_verify(CTX, &signature[4], message, &pubkey) == 1);
    /* Parse compact (with recovery id) and recover. */
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsignature[4], sig, recid) == 1);
    CHECK(secp256k1_ecdsa_recover(CTX, &recpubkey, &rsignature[4], message) == 1);
    CHECK(secp256k1_memcmp_var(&pubkey, &recpubkey, sizeof(pubkey)) == 0);
    /* Serialize/destroy/parse signature and verify again. */
    CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(CTX, sig, &recid, &rsignature[4]) == 1);
    sig[testrand_bits(6)] += 1 + testrand_int(255);
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsignature[4], sig, recid) == 1);
    CHECK(secp256k1_ecdsa_recoverable_signature_convert(CTX, &signature[4], &rsignature[4]) == 1);
    CHECK(secp256k1_ecdsa_verify(CTX, &signature[4], message, &pubkey) == 0);
    /* Recover again */
    CHECK(secp256k1_ecdsa_recover(CTX, &recpubkey, &rsignature[4], message) == 0 ||
          secp256k1_memcmp_var(&pubkey, &recpubkey, sizeof(pubkey)) != 0);
}

/* Tests several edge cases. */
static void test_ecdsa_recovery_edge_cases(void) {
    const unsigned char msg32[32] = {
        'T', 'h', 'i', 's', ' ', 'i', 's', ' ',
        'a', ' ', 'v', 'e', 'r', 'y', ' ', 's',
        'e', 'c', 'r', 'e', 't', ' ', 'm', 'e',
        's', 's', 'a', 'g', 'e', '.', '.', '.'
    };
    const unsigned char sig64[64] = {
        /* Generated by signing the above message with nonce 'This is the nonce we will use...'
         * and secret key 0 (which is not valid), resulting in recid 1. */
        0x67, 0xCB, 0x28, 0x5F, 0x9C, 0xD1, 0x94, 0xE8,
        0x40, 0xD6, 0x29, 0x39, 0x7A, 0xF5, 0x56, 0x96,
        0x62, 0xFD, 0xE4, 0x46, 0x49, 0x99, 0x59, 0x63,
        0x17, 0x9A, 0x7D, 0xD1, 0x7B, 0xD2, 0x35, 0x32,
        0x4B, 0x1B, 0x7D, 0xF3, 0x4C, 0xE1, 0xF6, 0x8E,
        0x69, 0x4F, 0xF6, 0xF1, 0x1A, 0xC7, 0x51, 0xDD,
        0x7D, 0xD7, 0x3E, 0x38, 0x7E, 0xE4, 0xFC, 0x86,
        0x6E, 0x1B, 0xE8, 0xEC, 0xC7, 0xDD, 0x95, 0x57
    };
    secp256k1_pubkey pubkey;
    /* signature (r,s) = (4,4), which can be recovered with all 4 recids. */
    const unsigned char sigb64[64] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    };
    secp256k1_pubkey pubkeyb;
    secp256k1_ecdsa_recoverable_signature rsig;
    secp256k1_ecdsa_signature sig;
    int recid;

    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sig64, 0));
    CHECK(!secp256k1_ecdsa_recover(CTX, &pubkey, &rsig, msg32));
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sig64, 1));
    CHECK(secp256k1_ecdsa_recover(CTX, &pubkey, &rsig, msg32));
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sig64, 2));
    CHECK(!secp256k1_ecdsa_recover(CTX, &pubkey, &rsig, msg32));
    CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sig64, 3));
    CHECK(!secp256k1_ecdsa_recover(CTX, &pubkey, &rsig, msg32));

    for (recid = 0; recid < 4; recid++) {
        int i;
        int recid2;
        /* (4,4) encoded in DER. */
        unsigned char sigbder[8] = {0x30, 0x06, 0x02, 0x01, 0x04, 0x02, 0x01, 0x04};
        unsigned char sigcder_zr[7] = {0x30, 0x05, 0x02, 0x00, 0x02, 0x01, 0x01};
        unsigned char sigcder_zs[7] = {0x30, 0x05, 0x02, 0x01, 0x01, 0x02, 0x00};
        unsigned char sigbderalt1[39] = {
            0x30, 0x25, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x04, 0x02, 0x01, 0x04,
        };
        unsigned char sigbderalt2[39] = {
            0x30, 0x25, 0x02, 0x01, 0x04, 0x02, 0x20, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
        };
        unsigned char sigbderalt3[40] = {
            0x30, 0x26, 0x02, 0x21, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x01, 0x04,
        };
        unsigned char sigbderalt4[40] = {
            0x30, 0x26, 0x02, 0x01, 0x04, 0x02, 0x21, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
        };
        /* (order + r,4) encoded in DER. */
        unsigned char sigbderlong[40] = {
            0x30, 0x26, 0x02, 0x21, 0x00, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xBA, 0xAE, 0xDC,
            0xE6, 0xAF, 0x48, 0xA0, 0x3B, 0xBF, 0xD2, 0x5E,
            0x8C, 0xD0, 0x36, 0x41, 0x45, 0x02, 0x01, 0x04
        };
        CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sigb64, recid) == 1);
        CHECK(secp256k1_ecdsa_recover(CTX, &pubkeyb, &rsig, msg32) == 1);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbder, sizeof(sigbder)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyb) == 1);
        for (recid2 = 0; recid2 < 4; recid2++) {
            secp256k1_pubkey pubkey2b;
            CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sigb64, recid2) == 1);
            CHECK(secp256k1_ecdsa_recover(CTX, &pubkey2b, &rsig, msg32) == 1);
            /* Verifying with (order + r,4) should always fail. */
            CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderlong, sizeof(sigbderlong)) == 1);
            CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyb) == 0);
        }
        /* DER parsing tests. */
        /* Zero length r/s. */
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigcder_zr, sizeof(sigcder_zr)) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigcder_zs, sizeof(sigcder_zs)) == 0);
        /* Leading zeros. */
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderalt1, sizeof(sigbderalt1)) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderalt2, sizeof(sigbderalt2)) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderalt3, sizeof(sigbderalt3)) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderalt4, sizeof(sigbderalt4)) == 0);
        sigbderalt3[4] = 1;
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderalt3, sizeof(sigbderalt3)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyb) == 0);
        sigbderalt4[7] = 1;
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbderalt4, sizeof(sigbderalt4)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyb) == 0);
        /* Damage signature. */
        sigbder[7]++;
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbder, sizeof(sigbder)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyb) == 0);
        sigbder[7]--;
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbder, 6) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbder, sizeof(sigbder) - 1) == 0);
        for(i = 0; i < 8; i++) {
            int c;
            unsigned char orig = sigbder[i];
            /*Try every single-byte change.*/
            for (c = 0; c < 256; c++) {
                if (c == orig ) {
                    continue;
                }
                sigbder[i] = c;
                CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigbder, sizeof(sigbder)) == 0 || secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyb) == 0);
            }
            sigbder[i] = orig;
        }
    }

    /* Test r/s equal to zero */
    {
        /* (1,1) encoded in DER. */
        unsigned char sigcder[8] = {0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01};
        unsigned char sigc64[64] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        };
        secp256k1_pubkey pubkeyc;
        CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sigc64, 0) == 1);
        CHECK(secp256k1_ecdsa_recover(CTX, &pubkeyc, &rsig, msg32) == 1);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigcder, sizeof(sigcder)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyc) == 1);
        sigcder[4] = 0;
        sigc64[31] = 0;
        CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sigc64, 0) == 1);
        CHECK(secp256k1_ecdsa_recover(CTX, &pubkeyb, &rsig, msg32) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigcder, sizeof(sigcder)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyc) == 0);
        sigcder[4] = 1;
        sigcder[7] = 0;
        sigc64[31] = 1;
        sigc64[63] = 0;
        CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(CTX, &rsig, sigc64, 0) == 1);
        CHECK(secp256k1_ecdsa_recover(CTX, &pubkeyb, &rsig, msg32) == 0);
        CHECK(secp256k1_ecdsa_signature_parse_der(CTX, &sig, sigcder, sizeof(sigcder)) == 1);
        CHECK(secp256k1_ecdsa_verify(CTX, &sig, msg32, &pubkeyc) == 0);
    }
}

static void run_recovery_tests(void) {
    int i;
    for (i = 0; i < COUNT; i++) {
        test_ecdsa_recovery_api();
    }
    for (i = 0; i < 64*COUNT; i++) {
        test_ecdsa_recovery_end_to_end();
    }
    test_ecdsa_recovery_edge_cases();
}

#endif /* SECP256K1_MODULE_RECOVERY_TESTS_H */
