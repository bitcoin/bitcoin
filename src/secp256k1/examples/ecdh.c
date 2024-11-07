/*************************************************************************
 * Written in 2020-2022 by Elichai Turkel                                *
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>

#include "examples_util.h"

int main(void) {
    unsigned char seckey1[32];
    unsigned char seckey2[32];
    unsigned char compressed_pubkey1[33];
    unsigned char compressed_pubkey2[33];
    unsigned char shared_secret1[32];
    unsigned char shared_secret2[32];
    unsigned char randomize[32];
    int return_val;
    size_t len;
    secp256k1_pubkey pubkey1;
    secp256k1_pubkey pubkey2;

    /* Before we can call actual API functions, we need to create a "context". */
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return 1;
    }
    /* Randomizing the context is recommended to protect against side-channel
     * leakage See `secp256k1_context_randomize` in secp256k1.h for more
     * information about it. This should never fail. */
    return_val = secp256k1_context_randomize(ctx, randomize);
    assert(return_val);

    /*** Key Generation ***/
    if (!fill_random(seckey1, sizeof(seckey1)) || !fill_random(seckey2, sizeof(seckey2))) {
        printf("Failed to generate randomness\n");
        return 1;
    }
    /* If the secret key is zero or out of range (greater than secp256k1's
    * order), we fail. Note that the probability of this occurring is negligible
    * with a properly functioning random number generator. */
    if (!secp256k1_ec_seckey_verify(ctx, seckey1) || !secp256k1_ec_seckey_verify(ctx, seckey2)) {
        printf("Generated secret key is invalid. This indicates an issue with the random number generator.\n");
        return 1;
    }

    /* Public key creation using a valid context with a verified secret key should never fail */
    return_val = secp256k1_ec_pubkey_create(ctx, &pubkey1, seckey1);
    assert(return_val);
    return_val = secp256k1_ec_pubkey_create(ctx, &pubkey2, seckey2);
    assert(return_val);

    /* Serialize pubkey1 in a compressed form (33 bytes), should always return 1 */
    len = sizeof(compressed_pubkey1);
    return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey1, &len, &pubkey1, SECP256K1_EC_COMPRESSED);
    assert(return_val);
    /* Should be the same size as the size of the output, because we passed a 33 byte array. */
    assert(len == sizeof(compressed_pubkey1));

    /* Serialize pubkey2 in a compressed form (33 bytes) */
    len = sizeof(compressed_pubkey2);
    return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey2, &len, &pubkey2, SECP256K1_EC_COMPRESSED);
    assert(return_val);
    /* Should be the same size as the size of the output, because we passed a 33 byte array. */
    assert(len == sizeof(compressed_pubkey2));

    /*** Creating the shared secret ***/

    /* Perform ECDH with seckey1 and pubkey2. Should never fail with a verified
     * seckey and valid pubkey */
    return_val = secp256k1_ecdh(ctx, shared_secret1, &pubkey2, seckey1, NULL, NULL);
    assert(return_val);

    /* Perform ECDH with seckey2 and pubkey1. Should never fail with a verified
     * seckey and valid pubkey */
    return_val = secp256k1_ecdh(ctx, shared_secret2, &pubkey1, seckey2, NULL, NULL);
    assert(return_val);

    /* Both parties should end up with the same shared secret */
    return_val = memcmp(shared_secret1, shared_secret2, sizeof(shared_secret1));
    assert(return_val == 0);

    printf("Secret Key1: ");
    print_hex(seckey1, sizeof(seckey1));
    printf("Compressed Pubkey1: ");
    print_hex(compressed_pubkey1, sizeof(compressed_pubkey1));
    printf("\nSecret Key2: ");
    print_hex(seckey2, sizeof(seckey2));
    printf("Compressed Pubkey2: ");
    print_hex(compressed_pubkey2, sizeof(compressed_pubkey2));
    printf("\nShared Secret: ");
    print_hex(shared_secret1, sizeof(shared_secret1));

    /* This will clear everything from the context and free the memory */
    secp256k1_context_destroy(ctx);

    /* It's best practice to try to clear secrets from memory after using them.
     * This is done because some bugs can allow an attacker to leak memory, for
     * example through "out of bounds" array access (see Heartbleed), or the OS
     * swapping them to disk. Hence, we overwrite the secret key buffer with zeros.
     *
     * Here we are preventing these writes from being optimized out, as any good compiler
     * will remove any writes that aren't used. */
    secure_erase(seckey1, sizeof(seckey1));
    secure_erase(seckey2, sizeof(seckey2));
    secure_erase(shared_secret1, sizeof(shared_secret1));
    secure_erase(shared_secret2, sizeof(shared_secret2));

    return 0;
}
