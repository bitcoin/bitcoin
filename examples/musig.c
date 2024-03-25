/*************************************************************************
 * Written in 2018 by Jonas Nick                                         *
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

/** This file demonstrates how to use the MuSig module to create a
 *  3-of-3 multisignature. Additionally, see the documentation in
 *  include/secp256k1_musig.h and src/modules/musig/musig.md.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_musig.h>

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
    while (1) {
        if (!fill_random(seckey, sizeof(seckey))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        if (secp256k1_keypair_create(ctx, &signer_secrets->keypair, seckey)) {
            break;
        }
    }
    if (!secp256k1_keypair_pub(ctx, &signer->pubkey, &signer_secrets->keypair)) {
        return 0;
    }
    return 1;
}

/* Tweak the pubkey corresponding to the provided keyagg cache, update the cache
 * and return the tweaked aggregate pk. */
static int tweak(const secp256k1_context* ctx, secp256k1_xonly_pubkey *agg_pk, secp256k1_musig_keyagg_cache *cache) {
    secp256k1_pubkey output_pk;
    unsigned char plain_tweak[32] = "this could be a BIP32 tweak....";
    unsigned char xonly_tweak[32] = "this could be a taproot tweak..";


    /* Plain tweaking which, for example, allows deriving multiple child
     * public keys from a single aggregate key using BIP32 */
    if (!secp256k1_musig_pubkey_ec_tweak_add(ctx, NULL, cache, plain_tweak)) {
        return 0;
    }
    /* Note that we did not provided an output_pk argument, because the
     * resulting pk is also saved in the cache and so if one is just interested
     * in signing the output_pk argument is unnecessary. On the other hand, if
     * one is not interested in signing, the same output_pk can be obtained by
     * calling `secp256k1_musig_pubkey_get` right after key aggregation to get
     * the full pubkey and then call `secp256k1_ec_pubkey_tweak_add`. */

    /* Xonly tweaking which, for example, allows creating taproot commitments */
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
     * the taproot commitment. */
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
    }
    /* Communication round 1: A production system would exchange public nonces
     * here before moving on. */
    for (i = 0; i < N_SIGNERS; i++) {
        secp256k1_musig_aggnonce agg_pubnonce;

        /* Create aggregate nonce and initialize the session */
        if (!secp256k1_musig_nonce_agg(ctx, &agg_pubnonce, pubnonces, N_SIGNERS)) {
            return 0;
        }
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
    /* Communication round 2: A production system would exchange
     * partial signatures here before moving on. */
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
    unsigned char msg[32] = "this_could_be_the_hash_of_a_msg!";
    unsigned char sig[64];

    /* Create a secp256k1 context */
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    printf("Creating key pairs......");
    for (i = 0; i < N_SIGNERS; i++) {
        if (!create_keypair(ctx, &signer_secrets[i], &signers[i])) {
            printf("FAILED\n");
            return 1;
        }
        pubkeys_ptr[i] = &signers[i].pubkey;
    }
    printf("ok\n");
    printf("Combining public keys...");
    /* If you just want to aggregate and not sign the cache can be NULL */
    if (!secp256k1_musig_pubkey_agg(ctx, &agg_pk, &cache, pubkeys_ptr, N_SIGNERS)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    printf("Tweaking................");
    /* Optionally tweak the aggregate key */
    if (!tweak(ctx, &agg_pk, &cache)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    printf("Signing message.........");
    if (!sign(ctx, signer_secrets, signers, &cache, msg, sig)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    printf("Verifying signature.....");
    if (!secp256k1_schnorrsig_verify(ctx, sig, msg, 32, &agg_pk)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");
    secp256k1_context_destroy(ctx);
    return 0;
}
