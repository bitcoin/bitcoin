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
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include "random.h"

int main(void) {
    /* Instead of signing the message directly, we must sign a 32-byte hash.
     * Here the message is "Hello, world!" and the hash function was SHA-256.
     * An actual implementation should just call SHA-256, but this example
     * hardcodes the output to avoid depending on an additional library. */
    unsigned char msg_hash[32] = {
        0x31, 0x5F, 0x5B, 0xDB, 0x76, 0xD0, 0x78, 0xC4,
        0x3B, 0x8A, 0xC0, 0x06, 0x4E, 0x4A, 0x01, 0x64,
        0x61, 0x2B, 0x1F, 0xCE, 0x77, 0xC8, 0x69, 0x34,
        0x5B, 0xFC, 0x94, 0xC7, 0x58, 0x94, 0xED, 0xD3,
    };
    unsigned char seckey[32];
    unsigned char randomize[32];
    unsigned char auxiliary_rand[32];
    unsigned char serialized_pubkey[32];
    unsigned char signature[64];
    int is_signature_valid;
    int return_val;
    secp256k1_xonly_pubkey pubkey;
    secp256k1_keypair keypair;
    /* The specification in secp256k1_extrakeys.h states that `secp256k1_keypair_create`
     * needs a context object initialized for signing. And in secp256k1_schnorrsig.h
     * they state that `secp256k1_schnorrsig_verify` needs a context initialized for
     * verification, which is why we create a context for both signing and verification
     * with the SECP256K1_CONTEXT_SIGN and SECP256K1_CONTEXT_VERIFY flags. */
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
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

    /* If the secret key is zero or out of range (bigger than secp256k1's
     * order), we try to sample a new key. Note that the probability of this
     * happening is negligible. */
    while (1) {
        if (!fill_random(seckey, sizeof(seckey))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        /* Try to create a keypair with a valid context, it should only fail if
         * the secret key is zero or out of range. */
        if (secp256k1_keypair_create(ctx, &keypair, seckey)) {
            break;
        }
    }

    /* Extract the X-only public key from the keypair. We pass NULL for
     * `pk_parity` as the parity isn't needed for signing or verification.
     * `secp256k1_keypair_xonly_pub` supports returning the parity for
     * other use cases such as tests or verifying Taproot tweaks.
     * This should never fail with a valid context and public key. */
    return_val = secp256k1_keypair_xonly_pub(ctx, &pubkey, NULL, &keypair);
    assert(return_val);

    /* Serialize the public key. Should always return 1 for a valid public key. */
    return_val = secp256k1_xonly_pubkey_serialize(ctx, serialized_pubkey, &pubkey);
    assert(return_val);

    /*** Signing ***/

    /* Generate 32 bytes of randomness to use with BIP-340 schnorr signing. */
    if (!fill_random(auxiliary_rand, sizeof(auxiliary_rand))) {
        printf("Failed to generate randomness\n");
        return 1;
    }

    /* Generate a Schnorr signature `noncefp` and `ndata` allows you to pass a
     * custom nonce function, passing `NULL` will use the BIP-340 safe default.
     * BIP-340 recommends passing 32 bytes of randomness to the nonce function to
     * improve security against side-channel attacks. Signing with a valid
     * context, verified keypair and the default nonce function should never
     * fail. */
    return_val = secp256k1_schnorrsig_sign(ctx, signature, msg_hash, &keypair, auxiliary_rand);
    assert(return_val);

    /*** Verification ***/

    /* Deserialize the public key. This will return 0 if the public key can't
     * be parsed correctly */
    if (!secp256k1_xonly_pubkey_parse(ctx, &pubkey, serialized_pubkey)) {
        printf("Failed parsing the public key\n");
        return 1;
    }

    /* Verify a signature. This will return 1 if it's valid and 0 if it's not. */
    is_signature_valid = secp256k1_schnorrsig_verify(ctx, signature, msg_hash, 32, &pubkey);


    printf("Is the signature valid? %s\n", is_signature_valid ? "true" : "false");
    printf("Secret Key: ");
    print_hex(seckey, sizeof(seckey));
    printf("Public Key: ");
    print_hex(serialized_pubkey, sizeof(serialized_pubkey));
    printf("Signature: ");
    print_hex(signature, sizeof(signature));

    /* This will clear everything from the context and free the memory */
    secp256k1_context_destroy(ctx);

    /* It's best practice to try to clear secrets from memory after using them.
     * This is done because some bugs can allow an attacker to leak memory, for
     * example through "out of bounds" array access (see Heartbleed), Or the OS
     * swapping them to disk. Hence, we overwrite the secret key buffer with zeros.
     *
     * TODO: Prevent these writes from being optimized out, as any good compiler
     * will remove any writes that aren't used. */
    memset(seckey, 0, sizeof(seckey));

    return 0;
}
