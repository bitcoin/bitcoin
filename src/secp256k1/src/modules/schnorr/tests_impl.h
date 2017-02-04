/**********************************************************************
 * Copyright (c) 2014-2015 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef SECP256K1_MODULE_SCHNORR_TESTS
#define SECP256K1_MODULE_SCHNORR_TESTS

#include "include/secp256k1_schnorr.h"

void test_schnorr_end_to_end(void) {
    unsigned char privkey[32];
    unsigned char message[32];
    unsigned char schnorr_signature[64];
    secp256k1_pubkey pubkey, recpubkey;

    /* Generate a random key and message. */
    {
        secp256k1_scalar key;
        random_scalar_order_test(&key);
        secp256k1_scalar_get_b32(privkey, &key);
        secp256k1_rand256_test(message);
    }

    /* Construct and verify corresponding public key. */
    CHECK(secp256k1_ec_seckey_verify(ctx, privkey) == 1);
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, privkey) == 1);

    /* Schnorr sign. */
    CHECK(secp256k1_schnorr_sign(ctx, schnorr_signature, message, privkey, NULL, NULL) == 1);
    CHECK(secp256k1_schnorr_verify(ctx, schnorr_signature, message, &pubkey) == 1);
    CHECK(secp256k1_schnorr_recover(ctx, &recpubkey, schnorr_signature, message) == 1);
    CHECK(memcmp(&pubkey, &recpubkey, sizeof(pubkey)) == 0);
    /* Destroy signature and verify again. */
    schnorr_signature[secp256k1_rand_bits(6)] += 1 + secp256k1_rand_int(255);
    CHECK(secp256k1_schnorr_verify(ctx, schnorr_signature, message, &pubkey) == 0);
    CHECK(secp256k1_schnorr_recover(ctx, &recpubkey, schnorr_signature, message) != 1 ||
          memcmp(&pubkey, &recpubkey, sizeof(pubkey)) != 0);
}

/** Horribly broken hash function. Do not use for anything but tests. */
void test_schnorr_hash(unsigned char *h32, const unsigned char *r32, const unsigned char *msg32) {
    int i;
    for (i = 0; i < 32; i++) {
        h32[i] = r32[i] ^ msg32[i];
    }
}

void test_schnorr_sign_verify(void) {
    unsigned char msg32[32];
    unsigned char sig64[3][64];
    secp256k1_gej pubkeyj[3];
    secp256k1_ge pubkey[3];
    secp256k1_scalar nonce[3], key[3];
    int i = 0;
    int k;

    secp256k1_rand256_test(msg32);

    for (k = 0; k < 3; k++) {
        random_scalar_order_test(&key[k]);

        do {
            random_scalar_order_test(&nonce[k]);
            if (secp256k1_schnorr_sig_sign(&ctx->ecmult_gen_ctx, sig64[k], &key[k], &nonce[k], NULL, &test_schnorr_hash, msg32)) {
                break;
            }
        } while(1);

        secp256k1_ecmult_gen(&ctx->ecmult_gen_ctx, &pubkeyj[k], &key[k]);
        secp256k1_ge_set_gej_var(&pubkey[k], &pubkeyj[k]);
        CHECK(secp256k1_schnorr_sig_verify(&ctx->ecmult_ctx, sig64[k], &pubkey[k], &test_schnorr_hash, msg32));

        for (i = 0; i < 4; i++) {
            int pos = secp256k1_rand_bits(6);
            int mod = 1 + secp256k1_rand_int(255);
            sig64[k][pos] ^= mod;
            CHECK(secp256k1_schnorr_sig_verify(&ctx->ecmult_ctx, sig64[k], &pubkey[k], &test_schnorr_hash, msg32) == 0);
            sig64[k][pos] ^= mod;
        }
    }
}

void test_schnorr_threshold(void) {
    unsigned char msg[32];
    unsigned char sec[5][32];
    secp256k1_pubkey pub[5];
    unsigned char nonce[5][32];
    secp256k1_pubkey pubnonce[5];
    unsigned char sig[5][64];
    const unsigned char* sigs[5];
    unsigned char allsig[64];
    const secp256k1_pubkey* pubs[5];
    secp256k1_pubkey allpub;
    int n, i;
    int damage;
    int ret = 0;

    damage = secp256k1_rand_bits(1) ? (1 + secp256k1_rand_int(4)) : 0;
    secp256k1_rand256_test(msg);
    n = 2 + secp256k1_rand_int(4);
    for (i = 0; i < n; i++) {
        do {
            secp256k1_rand256_test(sec[i]);
        } while (!secp256k1_ec_seckey_verify(ctx, sec[i]));
        CHECK(secp256k1_ec_pubkey_create(ctx, &pub[i], sec[i]));
        CHECK(secp256k1_schnorr_generate_nonce_pair(ctx, &pubnonce[i], nonce[i], msg, sec[i], NULL, NULL));
        pubs[i] = &pub[i];
    }
    if (damage == 1) {
        nonce[secp256k1_rand_int(n)][secp256k1_rand_int(32)] ^= 1 + secp256k1_rand_int(255);
    } else if (damage == 2) {
        sec[secp256k1_rand_int(n)][secp256k1_rand_int(32)] ^= 1 + secp256k1_rand_int(255);
    }
    for (i = 0; i < n; i++) {
        secp256k1_pubkey allpubnonce;
        const secp256k1_pubkey *pubnonces[4];
        int j;
        for (j = 0; j < i; j++) {
            pubnonces[j] = &pubnonce[j];
        }
        for (j = i + 1; j < n; j++) {
            pubnonces[j - 1] = &pubnonce[j];
        }
        CHECK(secp256k1_ec_pubkey_combine(ctx, &allpubnonce, pubnonces, n - 1));
        ret |= (secp256k1_schnorr_partial_sign(ctx, sig[i], msg, sec[i], &allpubnonce, nonce[i]) != 1) * 1;
        sigs[i] = sig[i];
    }
    if (damage == 3) {
        sig[secp256k1_rand_int(n)][secp256k1_rand_bits(6)] ^= 1 + secp256k1_rand_int(255);
    }
    ret |= (secp256k1_ec_pubkey_combine(ctx, &allpub, pubs, n) != 1) * 2;
    if ((ret & 1) == 0) {
        ret |= (secp256k1_schnorr_partial_combine(ctx, allsig, sigs, n) != 1) * 4;
    }
    if (damage == 4) {
        allsig[secp256k1_rand_int(32)] ^= 1 + secp256k1_rand_int(255);
    }
    if ((ret & 7) == 0) {
        ret |= (secp256k1_schnorr_verify(ctx, allsig, msg, &allpub) != 1) * 8;
    }
    CHECK((ret == 0) == (damage == 0));
}

void test_schnorr_recovery(void) {
    unsigned char msg32[32];
    unsigned char sig64[64];
    secp256k1_ge Q;

    secp256k1_rand256_test(msg32);
    secp256k1_rand256_test(sig64);
    secp256k1_rand256_test(sig64 + 32);
    if (secp256k1_schnorr_sig_recover(&ctx->ecmult_ctx, sig64, &Q, &test_schnorr_hash, msg32) == 1) {
        CHECK(secp256k1_schnorr_sig_verify(&ctx->ecmult_ctx, sig64, &Q, &test_schnorr_hash, msg32) == 1);
    }
}

void run_schnorr_tests(void) {
    int i;
    for (i = 0; i < 32*count; i++) {
        test_schnorr_end_to_end();
    }
    for (i = 0; i < 32 * count; i++) {
         test_schnorr_sign_verify();
    }
    for (i = 0; i < 16 * count; i++) {
         test_schnorr_recovery();
    }
    for (i = 0; i < 10 * count; i++) {
         test_schnorr_threshold();
    }
}

#endif
