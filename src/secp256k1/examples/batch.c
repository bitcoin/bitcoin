#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_batch.h>
#include <secp256k1_schnorrsig_batch.h>
#include <secp256k1_tweak_check_batch.h>

#include "random.h"

/* key pair data */
unsigned char sk[32];
secp256k1_keypair keypair;
secp256k1_xonly_pubkey pk;

/* schnorrsig verification data */
#define N_SIGS 10
unsigned char msg[N_SIGS][32];
unsigned char sig[N_SIGS][64];

/* xonly pubkey tweak checks data */
#define N_CHECKS 10
unsigned char tweaked_pubkey[N_CHECKS][32];
int tweaked_pk_parity[N_CHECKS];
unsigned char tweak[N_CHECKS][32];

/* 2*N_SIGS since one schnorrsig creates two scalar-point pairs in batch
 * whereas one tweak check creates one scalar-point pair in batch */
#define N_TERMS (N_CHECKS + 2*N_SIGS)

/* generate key pair required for sign and verify */
int create_keypair(secp256k1_context *ctx) {
    while(1) {
        if (!fill_random(sk, sizeof(sk))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        if (secp256k1_keypair_create(ctx, &keypair, sk)) {
            break;
        }
    }
    if (!secp256k1_keypair_xonly_pub(ctx, &pk, NULL, &keypair)) {
        return 0;
    }

    return 1;
}

/* create valid schnorrsigs for N_SIGS random messages */
int generate_schnorrsigs(secp256k1_context *ctx) {
    size_t i;

    for (i = 0; i < N_SIGS; i++) {
        if(!fill_random(msg[i], sizeof(msg[i]))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        assert(secp256k1_schnorrsig_sign32(ctx, sig[i], msg[i], &keypair, NULL));
        assert(secp256k1_schnorrsig_verify(ctx, sig[i], msg[i], sizeof(msg[i]), &pk));
    }

    return 1;
}

/* create valid N_CHECKS number of xonly pukey tweak checks */
int generate_xonlypub_tweak_checks(secp256k1_context *ctx) {
    secp256k1_pubkey output_pk;
    secp256k1_xonly_pubkey output_xonly_pk;
    size_t i;

    for (i = 0; i < N_CHECKS; i++) {
        if (!fill_random(tweak[i], sizeof(tweak[i]))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        assert(secp256k1_xonly_pubkey_tweak_add(ctx, &output_pk, &pk, tweak[i]));
        assert(secp256k1_xonly_pubkey_from_pubkey(ctx, &output_xonly_pk, &tweaked_pk_parity[i], &output_pk));
        assert(secp256k1_xonly_pubkey_serialize(ctx, tweaked_pubkey[i], &output_xonly_pk));
        assert(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked_pubkey[i], tweaked_pk_parity[i], &pk, tweak[i]));
    }

    return 1;
}

int main(void) {
    int ret;
    size_t i;
    /* batch object uses secp256k1_context only for the error callback function
     * here, we create secp256k1_context that can sign and verify, only to generate
     * input data (schnorrsigs, tweak checks) required for the batch */
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_batch *batch;
    unsigned char auxiliary_rand[16];

    /* Generate 16 bytes of randomness to use during batch creation. */
    if (!fill_random(auxiliary_rand, sizeof(auxiliary_rand))) {
        printf("Failed to generate randomness\n");
        return 1;
    }

    batch = secp256k1_batch_create(ctx, N_TERMS, auxiliary_rand);

    assert(ctx != NULL);
    assert(batch != NULL);

    /* key pair generation */
    printf("Creating a key pair.........................");
    if(!create_keypair(ctx)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");

    /* create schnorrsigs for N_SIGS random messages */
    printf("Signing messages............................");
    if(!generate_schnorrsigs(ctx)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");

    printf("Adding signatures to the batch object.......");
    for (i = 0; i < N_SIGS; i++) {
        /* It is recommended to check the validity of the batch before adding a
         * new input (schnorrsig/tweak check) to it. The `secp256k1_batch_add_` APIs
         * won't add any new input to invalid batch since the final `secp256k1_batch_verify`
         * API call will fail even if the new input is valid. */
        if(secp256k1_batch_usable(ctx, batch)) {
            ret = secp256k1_batch_add_schnorrsig(ctx, batch, sig[i], msg[i], sizeof(msg[i]), &pk);
        } else {
            printf("INVALID BATCH\n");
            return 1;
        }

        if(!ret) {
            printf("FAILED\n");
            return 1;
        }
    }
    printf("ok\n");

    printf("Generating xonlypub tweak checks............");
    if(!generate_xonlypub_tweak_checks(ctx)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");

    printf("Adding tweak checks to the batch object.....");
    for (i = 0; i < N_CHECKS; i++) {
        /* It is recommended to check the validity of the batch before adding a
         * new input (schnorrsig/tweak check) to it. The `secp256k1_batch_add_` APIs
         * won't add any new input to invalid batch since the final `secp256k1_batch_verify`
         * API call will fail even if the new input is valid. */
        if(secp256k1_batch_usable(ctx, batch)) {
            ret = secp256k1_batch_add_xonlypub_tweak_check(ctx, batch, tweaked_pubkey[i], tweaked_pk_parity[i], &pk, tweak[i]);
        } else {
            printf("INVALID BATCH\n");
            return 1;
        }

        if(!ret) {
            printf("FAILED\n");
            return 1;
        }
    }
    printf("ok\n");

    printf("Verifying the batch object..................");
    if(!secp256k1_batch_verify(ctx, batch)) {
        printf("FAILED\n");
        return 1;
    }
    printf("ok\n");

    secp256k1_batch_destroy(ctx, batch);
    secp256k1_context_destroy(ctx);

    return 0;
}
