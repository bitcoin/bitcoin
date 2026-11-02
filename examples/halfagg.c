/*************************************************************************
 * Written in 2026 by the libsecp256k1 developers                        *
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

/** This file demonstrates how to use the schnorrsig_halfagg module to
 *  half-aggregate Schnorr signatures and verify the resulting aggregate
 *  signature. Additionally, see the documentation in
 *  include/secp256k1_schnorrsig_halfagg.h and doc/halfagg.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_schnorrsig_halfagg.h>

#include "examples_util.h"

#define N_SIGS 3

int main(void) {
    secp256k1_context* ctx;
    secp256k1_xonly_pubkey pubkeys[N_SIGS];
    unsigned char msgs32[N_SIGS * 32];
    unsigned char sigs64[N_SIGS * 64];
    unsigned char aggsig[32 * (N_SIGS + 1)];
    size_t aggsig_len;
    int i;

    /* Create a secp256k1 context */
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    /* Half-aggregation is non-interactive: the signatures to aggregate can
     * come from anywhere. Here, they are created on the spot, each with its
     * own key pair and message. */
    printf("Creating signatures.....");
    fflush(stdout);
    for (i = 0; i < N_SIGS; i++) {
        unsigned char seckey[32];
        unsigned char aux_rand[32];
        secp256k1_keypair keypair;

        if (!fill_random(seckey, sizeof(seckey)) ||
            !fill_random(aux_rand, sizeof(aux_rand)) ||
            !fill_random(&msgs32[i * 32], 32)) {
            printf("FAILED\n");
            return EXIT_FAILURE;
        }
        /* Try to create a keypair with a valid context. This only fails if the
         * secret key is zero or out of range (greater than secp256k1's order). Note
         * that the probability of this occurring is negligible with a properly
         * functioning random number generator. */
        if (!secp256k1_keypair_create(ctx, &keypair, seckey)) {
            printf("FAILED\n");
            return EXIT_FAILURE;
        }
        if (!secp256k1_keypair_xonly_pub(ctx, &pubkeys[i], NULL, &keypair)) {
            printf("FAILED\n");
            return EXIT_FAILURE;
        }
        if (!secp256k1_schnorrsig_sign32(ctx, &sigs64[i * 64], &msgs32[i * 32], &keypair, aux_rand)) {
            printf("FAILED\n");
            return EXIT_FAILURE;
        }

        /* It's best practice to try to clear secrets from memory after using them.
         * This is done because some bugs can allow an attacker to leak memory, for
         * example through "out of bounds" array access (see Heartbleed), or the OS
         * swapping them to disk. Hence, we overwrite secret key material with zeros.
         *
         * Here we are preventing these writes from being optimized out, as any good compiler
         * will remove any writes that aren't used. */
        secure_erase(seckey, sizeof(seckey));
        secure_erase(&keypair, sizeof(keypair));
    }
    printf("ok\n");

    /* The aggregate signature depends on the order of the lists of public
     * keys, messages and signatures, so the aggregator and the verifier must
     * use the same order. */
    printf("Aggregating signatures..");
    fflush(stdout);
    aggsig_len = sizeof(aggsig);
    if (!secp256k1_schnorrsig_aggregate(ctx, aggsig, &aggsig_len, pubkeys, msgs32, sigs64, N_SIGS - 1)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    /* Signatures can also be added to an existing aggregate signature. Here,
     * the last signature is added incrementally. */
    aggsig_len = sizeof(aggsig);
    if (!secp256k1_schnorrsig_inc_aggregate(ctx, aggsig, &aggsig_len, pubkeys, msgs32, &sigs64[(N_SIGS - 1) * 64], N_SIGS - 1, 1)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");

    printf("Verifying signature.....");
    fflush(stdout);
    if (!secp256k1_schnorrsig_aggverify(ctx, pubkeys, msgs32, N_SIGS, aggsig, aggsig_len)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");

    secp256k1_context_destroy(ctx);
    return EXIT_SUCCESS;
}
