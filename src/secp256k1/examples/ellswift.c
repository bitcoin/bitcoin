/*************************************************************************
 * Written in 2024 by Sebastian Falbesoner                               *
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

/** This file demonstrates how to use the ElligatorSwift module to perform
 *  a key exchange according to BIP 324. Additionally, see the documentation
 *  in include/secp256k1_ellswift.h and doc/ellswift.md.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_ellswift.h>

#include "examples_util.h"

int main(void) {
    secp256k1_context* ctx;
    unsigned char randomize[32];
    unsigned char auxrand1[32];
    unsigned char auxrand2[32];
    unsigned char seckey1[32];
    unsigned char seckey2[32];
    unsigned char ellswift_pubkey1[64];
    unsigned char ellswift_pubkey2[64];
    unsigned char shared_secret1[32];
    unsigned char shared_secret2[32];
    int return_val;

    /* Create a secp256k1 context */
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return 1;
    }
    /* Randomizing the context is recommended to protect against side-channel
     * leakage. See `secp256k1_context_randomize` in secp256k1.h for more
     * information about it. This should never fail. */
    return_val = secp256k1_context_randomize(ctx, randomize);
    assert(return_val);

    /*** Generate secret keys ***/
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

    /* Generate ElligatorSwift public keys. This should never fail with valid context and
       verified secret keys. Note that providing additional randomness (fourth parameter) is
       optional, but recommended. */
    if (!fill_random(auxrand1, sizeof(auxrand1)) || !fill_random(auxrand2, sizeof(auxrand2))) {
        printf("Failed to generate randomness\n");
        return 1;
    }
    return_val = secp256k1_ellswift_create(ctx, ellswift_pubkey1, seckey1, auxrand1);
    assert(return_val);
    return_val = secp256k1_ellswift_create(ctx, ellswift_pubkey2, seckey2, auxrand2);
    assert(return_val);

    /*** Create the shared secret on each side ***/

    /* Perform x-only ECDH with seckey1 and ellswift_pubkey2. Should never fail
     * with a verified seckey and valid pubkey. Note that both parties pass both
     * EllSwift pubkeys in the same order; the pubkey of the calling party is
     * determined by the "party" boolean (sixth parameter). */
    return_val = secp256k1_ellswift_xdh(ctx, shared_secret1, ellswift_pubkey1, ellswift_pubkey2,
        seckey1, 0, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
    assert(return_val);

    /* Perform x-only ECDH with seckey2 and ellswift_pubkey1. Should never fail
     * with a verified seckey and valid pubkey. */
    return_val = secp256k1_ellswift_xdh(ctx, shared_secret2, ellswift_pubkey1, ellswift_pubkey2,
        seckey2, 1, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
    assert(return_val);

    /* Both parties should end up with the same shared secret */
    return_val = memcmp(shared_secret1, shared_secret2, sizeof(shared_secret1));
    assert(return_val == 0);

    printf(  "     Secret Key1: ");
    print_hex(seckey1, sizeof(seckey1));
    printf(  "EllSwift Pubkey1: ");
    print_hex(ellswift_pubkey1, sizeof(ellswift_pubkey1));
    printf("\n     Secret Key2: ");
    print_hex(seckey2, sizeof(seckey2));
    printf(  "EllSwift Pubkey2: ");
    print_hex(ellswift_pubkey2, sizeof(ellswift_pubkey2));
    printf("\n   Shared Secret: ");
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
