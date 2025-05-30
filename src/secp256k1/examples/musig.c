/*************************************************************************
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

/** This file demonstrates how to use the MuSig module to create a
 *  3-of-3 multisignature. Additionally, see the documentation in
 *  include/secp256k1_musig.h and doc/musig.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_musig.h>
#include <secp256k1_schnorrsig.h>

#include "examples_util.h"

struct signer_secrets {
    secp256k1_keypair keypair;
    secp256k1_musig_secnonce secnonce;
};

struct signer {
    secp256k1_pubkey pubkey;
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_partial_sig partial_sig;
};

 /* Number of public keys involved in creating the aggregate signature */
#define N_SIGNERS 3
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
    if (!secp256k1_keypair_pub(ctx, &signer->pubkey, &signer_secrets->keypair)) {
        return 0;
    }

    secure_erase(seckey, sizeof(seckey));
    return 1;
}

/* Tweak the pubkey corresponding to the provided keyagg cache, update the cache
 * and return the tweaked aggregate pk. */
static int tweak(const secp256k1_context* ctx, secp256k1_xonly_pubkey *agg_pk, secp256k1_musig_keyagg_cache *cache) {
    secp256k1_pubkey output_pk;
    /* For BIP 32 tweaking the plain_tweak is set to a hash as defined in BIP
     * 32. */
    unsigned char plain_tweak[32] = "this could be a BIP32 tweak....";
    /* For Taproot tweaking the xonly_tweak is set to the TapTweak hash as
     * defined in BIP 341 */
    unsigned char xonly_tweak[32] = "this could be a Taproot tweak..";


    /* Plain tweaking which, for example, allows deriving multiple child
     * public keys from a single aggregate key using BIP32 */
    if (!secp256k1_musig_pubkey_ec_tweak_add(ctx, NULL, cache, plain_tweak)) {
        return 0;
    }
    /* Note that we did not provide an output_pk argument, because the
     * resulting pk is also saved in the cache and so if one is just interested
     * in signing, the output_pk argument is unnecessary. On the other hand, if
     * one is not interested in signing, the same output_pk can be obtained by
     * calling `secp256k1_musig_pubkey_get` right after key aggregation to get
     * the full pubkey and then call `secp256k1_ec_pubkey_tweak_add`. */

    /* Xonly tweaking which, for example, allows creating Taproot commitments */
    if (!secp256k1_musig_pubkey_xonly_tweak_add(ctx, &output_pk, cache, xonly_tweak)) {
        return 0;
    }
    /* Note that if we wouldn't care about signing, we can arrive at the same
     * output_pk by providing the untweaked public key to
     * `secp256k1_xonly_pubkey_tweak_add` (after converting it to an xonly pubkey
     * if necessary with `secp256k1_xonly_pubkey_from_pubkey`). */

    /* Now we convert the output_pk to an xonly pubkey to allow to later verify
     * the Schnorr signature against it. For this purpose we can ignore the
     * `pk_parity` output argument; we would need it if we would have to open
     * the Taproot commitment. */
    if (!secp256k1_xonly_pubkey_from_pubkey(ctx, agg_pk, NULL, &output_pk)) {
        return 0;
    }
    return 1;
}

/* Sign a message hash with the given key pairs and store the result in sig */
static int sign(const secp256k1_context* ctx, struct signer_secrets *signer_secrets, struct signer *signer, const secp256k1_musig_keyagg_cache *cache, const unsigned char *msg32, unsigned char *sig64) {
    int i;
    const secp256k1_musig_pubnonce *pubnonces[N_SIGNERS];
    const secp256k1_musig_partial_sig *partial_sigs[N_SIGNERS];
    /* The same for all signers */
    secp256k1_musig_session session;
    secp256k1_musig_aggnonce agg_pubnonce;

    for (i = 0; i < N_SIGNERS; i++) {
        unsigned char seckey[32];
        unsigned char session_secrand[32];
        /* Create random session ID. It is absolutely necessary that the session ID
         * is unique for every call of secp256k1_musig_nonce_gen. Otherwise
         * it's trivial for an attacker to extract the secret key! */
        if (!fill_random(session_secrand, sizeof(session_secrand))) {
            return 0;
        }
        if (!secp256k1_keypair_sec(ctx, seckey, &signer_secrets[i].keypair)) {
            return 0;
        }
        /* Initialize session and create secret nonce for signing and public
         * nonce to send to the other signers. */
        if (!secp256k1_musig_nonce_gen(ctx, &signer_secrets[i].secnonce, &signer[i].pubnonce, session_secrand, seckey, &signer[i].pubkey, msg32, NULL, NULL)) {
            return 0;
        }
        pubnonces[i] = &signer[i].pubnonce;

        secure_erase(seckey, sizeof(seckey));
    }

    /* Communication round 1: Every signer sends their pubnonce to the
     * coordinator. The coordinator runs secp256k1_musig_nonce_agg and sends
     * agg_pubnonce to each signer */
    if (!secp256k1_musig_nonce_agg(ctx, &agg_pubnonce, pubnonces, N_SIGNERS)) {
        return 0;
    }

    /* Every signer creates a partial signature */
    for (i = 0; i < N_SIGNERS; i++) {
        /* Initialize the signing session by processing the aggregate nonce */
        if (!secp256k1_musig_nonce_process(ctx, &session, &agg_pubnonce, msg32, cache)) {
            return 0;
        }
        /* partial_sign will clear the secnonce by setting it to 0. That's because
         * you must _never_ reuse the secnonce (or use the same session_secrand to
         * create a secnonce). If you do, you effectively reuse the nonce and
         * leak the secret key. */
        if (!secp256k1_musig_partial_sign(ctx, &signer[i].partial_sig, &signer_secrets[i].secnonce, &signer_secrets[i].keypair, cache, &session)) {
            return 0;
        }
        partial_sigs[i] = &signer[i].partial_sig;
    }
    /* Communication round 2: Every signer sends their partial signature to the
     * coordinator, who verifies the partial signatures and aggregates them. */
    for (i = 0; i < N_SIGNERS; i++) {
        /* To check whether signing was successful, it suffices to either verify
         * the aggregate signature with the aggregate public key using
         * secp256k1_schnorrsig_verify, or verify all partial signatures of all
         * signers individually. Verifying the aggregate signature is cheaper but
         * verifying the individual partial signatures has the advantage that it
         * can be used to determine which of the partial signatures are invalid
         * (if any), i.e., which of the partial signatures cause the aggregate
         * signature to be invalid and thus the protocol run to fail. It's also
         * fine to first verify the aggregate sig, and only verify the individual
         * sigs if it does not work.
         */
        if (!secp256k1_musig_partial_sig_verify(ctx, &signer[i].partial_sig, &signer[i].pubnonce, &signer[i].pubkey, cache, &session)) {
            return 0;
        }
    }
    return secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sigs, N_SIGNERS);
}

int main(void) {
    secp256k1_context* ctx;
    int i;
    struct signer_secrets signer_secrets[N_SIGNERS];
    struct signer signers[N_SIGNERS];
    const secp256k1_pubkey *pubkeys_ptr[N_SIGNERS];
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_musig_keyagg_cache cache;
    unsigned char msg[32] = "this_could_be_the_hash_of_a_msg";
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

    /* The aggregate public key produced by secp256k1_musig_pubkey_agg depends
     * on the order of the provided public keys. If there is no canonical order
     * of the signers, the individual public keys can optionally be sorted with
     * secp256k1_ec_pubkey_sort to ensure that the aggregate public key is
     * independent of the order of signers. */
    printf("Sorting public keys.....");
    fflush(stdout);
    if (!secp256k1_ec_pubkey_sort(ctx, pubkeys_ptr, N_SIGNERS)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");

    printf("Combining public keys...");
    fflush(stdout);
    /* If you just want to aggregate and not sign, you can call
     * secp256k1_musig_pubkey_agg with the keyagg_cache argument set to NULL
     * while providing a non-NULL agg_pk argument. */
    if (!secp256k1_musig_pubkey_agg(ctx, NULL, &cache, pubkeys_ptr, N_SIGNERS)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");
    printf("Tweaking................");
    fflush(stdout);
    /* Optionally tweak the aggregate key */
    if (!tweak(ctx, &agg_pk, &cache)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");
    printf("Signing message.........");
    fflush(stdout);
    if (!sign(ctx, signer_secrets, signers, &cache, msg, sig)) {
        printf("FAILED\n");
        return EXIT_FAILURE;
    }
    printf("ok\n");
    printf("Verifying signature.....");
    fflush(stdout);
    if (!secp256k1_schnorrsig_verify(ctx, sig, msg, 32, &agg_pk)) {
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
