 /*********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "ctaes.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct {
    int keysize;
    const char* key;
    const char* plain;
    const char* cipher;
} ctaes_test;

static const ctaes_test ctaes_tests[] = {
    /* AES test vectors from FIPS 197. */
    {128, "000102030405060708090a0b0c0d0e0f", "00112233445566778899aabbccddeeff", "69c4e0d86a7b0430d8cdb78070b4c55a"},
    {192, "000102030405060708090a0b0c0d0e0f1011121314151617", "00112233445566778899aabbccddeeff", "dda97ca4864cdfe06eaf70a0ec0d7191"},
    {256, "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", "00112233445566778899aabbccddeeff", "8ea2b7ca516745bfeafc49904b496089"},

    /* AES-ECB test vectors from NIST sp800-38a. */
    {128, "2b7e151628aed2a6abf7158809cf4f3c", "6bc1bee22e409f96e93d7e117393172a", "3ad77bb40d7a3660a89ecaf32466ef97"},
    {128, "2b7e151628aed2a6abf7158809cf4f3c", "ae2d8a571e03ac9c9eb76fac45af8e51", "f5d3d58503b9699de785895a96fdbaaf"},
    {128, "2b7e151628aed2a6abf7158809cf4f3c", "30c81c46a35ce411e5fbc1191a0a52ef", "43b1cd7f598ece23881b00e3ed030688"},
    {128, "2b7e151628aed2a6abf7158809cf4f3c", "f69f2445df4f9b17ad2b417be66c3710", "7b0c785e27e8ad3f8223207104725dd4"},
    {192, "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", "6bc1bee22e409f96e93d7e117393172a", "bd334f1d6e45f25ff712a214571fa5cc"},
    {192, "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", "ae2d8a571e03ac9c9eb76fac45af8e51", "974104846d0ad3ad7734ecb3ecee4eef"},
    {192, "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", "30c81c46a35ce411e5fbc1191a0a52ef", "ef7afd2270e2e60adce0ba2face6444e"},
    {192, "8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b", "f69f2445df4f9b17ad2b417be66c3710", "9a4b41ba738d6c72fb16691603c18e0e"},
    {256, "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", "6bc1bee22e409f96e93d7e117393172a", "f3eed1bdb5d2a03c064b5a7e3db181f8"},
    {256, "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", "ae2d8a571e03ac9c9eb76fac45af8e51", "591ccb10d410ed26dc5ba74a31362870"},
    {256, "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", "30c81c46a35ce411e5fbc1191a0a52ef", "b6ed21b99ca6f4f9f153e7b1beafed1d"},
    {256, "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4", "f69f2445df4f9b17ad2b417be66c3710", "23304b7a39f9f3ff067d8d8f9e24ecc7"}
};

static void from_hex(unsigned char* data, int len, const char* hex) {
    int p;
    for (p = 0; p < len; p++) {
        int v = 0;
        int n;
        for (n = 0; n < 2; n++) {
            assert((*hex >= '0' && *hex <= '9') || (*hex >= 'a' && *hex <= 'f'));
            if (*hex >= '0' && *hex <= '9') {
                v |= (*hex - '0') << (4 * (1 - n));
            } else {
                v |= (*hex - 'a' + 10) << (4 * (1 - n));
            }
            hex++;
        }
        *(data++) = v;
    }
    assert(*hex == 0);
}

int main(void) {
    int i;
    int fail = 0;
    for (i = 0; i < sizeof(ctaes_tests) / sizeof(ctaes_tests[0]); i++) {
        unsigned char key[32], plain[16], cipher[16], ciphered[16], deciphered[16];
        const ctaes_test* test = &ctaes_tests[i];
        assert(test->keysize == 128 || test->keysize == 192 || test->keysize == 256);
        from_hex(plain, 16, test->plain);
        from_hex(cipher, 16, test->cipher);
        switch (test->keysize) {
            case 128: {
                AES128_ctx ctx;
                from_hex(key, 16, test->key);
                AES128_init(&ctx, key);
                AES128_encrypt(&ctx, 1, ciphered, plain);
                AES128_decrypt(&ctx, 1, deciphered, cipher);
                break;
            }
            case 192: {
                AES192_ctx ctx;
                from_hex(key, 24, test->key);
                AES192_init(&ctx, key);
                AES192_encrypt(&ctx, 1, ciphered, plain);
                AES192_decrypt(&ctx, 1, deciphered, cipher);
                break;
            }
            case 256: {
                AES256_ctx ctx;
                from_hex(key, 32, test->key);
                AES256_init(&ctx, key);
                AES256_encrypt(&ctx, 1, ciphered, plain);
                AES256_decrypt(&ctx, 1, deciphered, cipher);
                break;
            }
        }
        if (memcmp(cipher, ciphered, 16)) {
            fprintf(stderr, "E(key=\"%s\", plain=\"%s\") != \"%s\"\n", test->key, test->plain, test->cipher);
            fail++;
        }
        if (memcmp(plain, deciphered, 16)) {
            fprintf(stderr, "D(key=\"%s\", cipher=\"%s\") != \"%s\"\n", test->key, test->cipher, test->plain);
            fail++;
        }
    }
    if (fail == 0) {
        fprintf(stderr, "All tests successful\n");
    } else {
        fprintf(stderr, "%i tests failed\n", fail);
    }
    return (fail != 0);
}
