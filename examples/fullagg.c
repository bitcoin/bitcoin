/*************************************************************************
 * Written in 2025 by Fabian Jahr                                        *
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

/** This file demonstrates how to use the fullagg module to create an
 *  aggregate signature for three signers who each sign their own message.
 *  Additionally, see the documentation in
 *  include/secp256k1_fullagg.h and doc/fullagg.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_fullagg.h>

#include "examples_util.h"

#define N_SIGNERS 3

struct signer_secrets {
    secp256k1_keypair keypair;
    secp256k1_fullagg_secnonce secnonce;
};

struct signer {
    secp256k1_xonly_pubkey pubkey;
    secp256k1_fullagg_pubnonce pubnonce;
    secp256k1_fullagg_partial_sig partial_sig;
    unsigned char message[32];
};

/* Create a key pair, store it in signer_secrets->keypair and signer->pubkey */
static int create_keypair(const secp256k1_context* ctx, struct signer_secrets *signer_secrets, struct signer *signer) {
    unsigned char seckey[32];

    if (!fill_random(seckey, sizeof(seckey))) {
        printf("Failed to generate randomness\n");
        return 0;
    }
    /* Try to create a keypair with a valid context. This only fails if the
     * secret key is zero or out of range (greater than secp256k1's order). Note
     * that the probability of this occurring is negligible with a properly
     * functioning random number generator. */
    if (!secp256k1_keypair_create(ctx, &signer_secrets->keypair, seckey)) {
        return 0;
    }
    if (!secp256k1_keypair_xonly_pub(ctx, &signer->pubkey, NULL, &signer_secrets->keypair)) {
        return 0;
    }

    secure_erase(seckey, sizeof(seckey));
    return 1;
}

/* Every signer signs their own message */
static void setup_messages(struct signer *signers) {
    memset(signers[0].message, 0, 32);
    memset(signers[1].message, 0, 32);
    memset(signers[2].message, 0, 32);
    memcpy(signers[0].message, "jonas", 5);
    memcpy(signers[1].message, "tim", 3);
    memcpy(signers[2].message, "yannick", 7);
}

/* Sign the messages of all signers and store the aggregate signature in sig64 */
static int sign(const secp256k1_context* ctx, struct signer_secrets *signer_secrets, struct signer *signers, const secp256k1_xonly_pubkey **pubkeys, const unsigned char **messages, unsigned char *sig64) {
    int i;
    const secp256k1_fullagg_pubnonce *pubnonces[N_SIGNERS];
    const secp256k1_fullagg_partial_sig *partial_sigs[N_SIGNERS];
    /* The same for all signers */
    secp256k1_fullagg_session session;
    secp256k1_fullagg_aggnonce agg_pubnonce;

    for (i = 0; i < N_SIGNERS; i++) {
        unsigned char seckey[32];
        unsigned char session_secrand[32];
        /* Create random session ID. It is absolutely necessary that the session ID
         * is unique for every call of secp256k1_fullagg_nonce_gen. Otherwise
         * it's trivial for an attacker to extract the secret key! */
        if (!fill_random(session_secrand, sizeof(session_secrand))) {
            return 0;
        }
        if (!secp256k1_keypair_sec(ctx, seckey, &signer_secrets[i].keypair)) {
            return 0;
        }
        /* Initialize session and create secret nonce for signing and public
         * nonce to send to the coordinator. */
        if (!secp256k1_fullagg_nonce_gen(ctx, &signer_secrets[i].secnonce, &signers[i].pubnonce, session_secrand, seckey, &signers[i].pubkey, NULL)) {
            return 0;
        }
        pubnonces[i] = &signers[i].pubnonce;

        secure_erase(seckey, sizeof(seckey));
    }

    /* Communication round 1: Every signer sends their pubnonce to the
     * coordinator. The coordinator runs secp256k1_fullagg_nonce_agg and sends
     * agg_pubnonce back to each signer, along with the ordered lists of all
     * public keys, messages and pubnonces. */
    if (!secp256k1_fullagg_nonce_agg(ctx, &agg_pubnonce, pubnonces, N_SIGNERS)) {
        return 0;
    }

    /* Every signer creates a partial signature for their own message */
    for (i = 0; i < N_SIGNERS; i++) {
        /* Initialize the signing session by processing the aggregate nonce.
         * Every signer computes the session itself and must not use session
         * data from an untrusted third party. */
        if (!secp256k1_fullagg_session_init(ctx, &session, &agg_pubnonce, pubkeys, messages, pubnonces, N_SIGNERS)) {
            return 0;
        }
        /* partial_sign will clear the secnonce by setting it to 0. That's because
         * you must _never_ reuse the secnonce (or use the same session_secrand to
         * create a secnonce). If you do, you effectively reuse the nonce and
         * leak the secret key. */
        if (!secp256k1_fullagg_partial_sign(ctx, &signers[i].partial_sig, &signer_secrets[i].secnonce, &signer_secrets[i].keypair, signers[i].message, &session, pubkeys, messages, pubnonces, N_SIGNERS, i)) {
            return 0;
        }
        partial_sigs[i] = &signers[i].partial_sig;
    }
    /* Communication round 2: Every signer sends their partial signature to the
     * coordinator, who verifies the partial signatures and aggregates them. */
    for (i = 0; i < N_SIGNERS; i++) {
        /* To check whether signing was successful, it suffices to either verify
         * the aggregate signature with secp256k1_fullagg_verify, or verify all
         * partial signatures of all signers individually. Verifying the
         * aggregate signature is cheaper but verifying the individual partial
         * signatures has the advantage that it can be used to determine which
         * of the partial signatures are invalid (if any), i.e., which of the
         * partial signatures cause the aggregate signature to be invalid and
         * thus the protocol run to fail. It's also fine to first verify the
         * aggregate sig, and only verify the individual sigs if it does not
         * work.
         */
        if (!secp256k1_fullagg_partial_sig_verify(ctx, &signers[i].partial_sig, pubnonces, pubkeys, messages, N_SIGNERS, i)) {
            return 0;
        }
    }
    return secp256k1_fullagg_partial_sig_agg(ctx, sig64, &session, partial_sigs, N_SIGNERS);
}

int main(void) {
    secp256k1_context* ctx;
    int i;
    struct signer_secrets signer_secrets[N_SIGNERS];
    struct signer signers[N_SIGNERS];
    const secp256k1_xonly_pubkey *pubkeys_ptr[N_SIGNERS];
    const unsigned char *messages_ptr[N_SIGNERS];
    unsigned char sig[64];

    /* Create a secp256k1 context */
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    printf("Creating key pairs......");
    fflush(stdout);
    for (i = 0; i < N_SIGNERS; i++) {
        if (!create_keypair(ctx, &signer_secrets[i], &signers[i])) {
            printf("FAILED\n");
            return EXIT_FAILURE;
        }
        pubkeys_ptr[i] = &signers[i].pubkey;
    }
    printf("ok\n");

    /* The aggregate signature and the challenges depend on the order of the
     * lists of public keys, messages and pubnonces. All signers, the
     * coordinator and the verifier must use the same order. */
    setup_messages(signers);
    for (i = 0; i < N_SIGNERS; i++) {
        messages_ptr[i] = signers[i].message;
    }

    printf("Signing messages........");
    fflush(stdout);
    if (!sign(ctx, signer_secrets, signers, pubkeys_ptr, messages_ptr, sig)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");
    printf("Verifying signature.....");
    fflush(stdout);
    if (!secp256k1_fullagg_verify(ctx, sig, pubkeys_ptr, messages_ptr, N_SIGNERS)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");

    /* It's best practice to try to clear secrets from memory after using them.
     * This is done because some bugs can allow an attacker to leak memory, for
     * example through "out of bounds" array access (see Heartbleed), or the OS
     * swapping them to disk. Hence, we overwrite secret key material with zeros.
     *
     * Here we are preventing these writes from being optimized out, as any good compiler
     * will remove any writes that aren't used. */
    for (i = 0; i < N_SIGNERS; i++) {
        secure_erase(&signer_secrets[i], sizeof(signer_secrets[i]));
    }
    secp256k1_context_destroy(ctx);
    return EXIT_SUCCESS;
}
