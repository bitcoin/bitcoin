/*************************************************************************
 * Written in 2020-2022 by Elichai Turkel                                *
 * To the extent possible under law, the author(s) have dedicated all    *
 * copyright and related and neighboring rights to the software in this  *
 * file to the public domain worldwide. This software is distributed     *
 * without any warranty. For the CC0 Public Domain Dedication, see       *
 * EXAMPLES_COPYING or https://creativecommons.org/publicdomain/zero/1.0 *
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include "examples_util.h"

int main(void) {
    unsigned char msg[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
    unsigned char msg_hash[32];
    unsigned char tag[] = {'m', 'y', '_', 'f', 'a', 'n', 'c', 'y', '_', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l'};
    unsigned char seckey[32];
    unsigned char randomize[32];
    unsigned char auxiliary_rand[32];
    unsigned char serialized_pubkey[32];
    unsigned char signature[64];
    int is_signature_valid, is_signature_valid2;
    int return_val;
    secp256k1_xonly_pubkey pubkey;
    secp256k1_keypair keypair;
    /* Before we can call actual API functions, we need to create a "context". */
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return EXIT_FAILURE;
    }
    /* Randomizing the context is recommended to protect against side-channel
     * leakage See `secp256k1_context_randomize` in secp256k1.h for more
     * information about it. This should never fail. */
    return_val = secp256k1_context_randomize(ctx, randomize);
    assert(return_val);

    /*** Key Generation ***/
    if (!fill_random(seckey, sizeof(seckey))) {
        printf("Failed to generate randomness\n");
        return EXIT_FAILURE;
    }
    /* Try to create a keypair with a valid context. This only fails if the
     * secret key is zero or out of range (greater than secp256k1's order). Note
     * that the probability of this occurring is negligible with a properly
     * functioning random number generator. */
    if (!secp256k1_keypair_create(ctx, &keypair, seckey)) {
        printf("Generated secret key is invalid. This indicates an issue with the random number generator.\n");
        return EXIT_FAILURE;
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

    /* Instead of signing (possibly very long) messages directly, we sign a
     * 32-byte hash of the message in this example.
     *
     * We use secp256k1_tagged_sha256 to create this hash. This function expects
     * a context-specific "tag", which restricts the context in which the signed
     * messages should be considered valid. For example, if protocol A mandates
     * to use the tag "my_fancy_protocol" and protocol B mandates to use the tag
     * "my_boring_protocol", then signed messages from protocol A will never be
     * valid in protocol B (and vice versa), even if keys are reused across
     * protocols. This implements "domain separation", which is considered good
     * practice. It avoids attacks in which users are tricked into signing a
     * message that has intended consequences in the intended context (e.g.,
     * protocol A) but would have unintended consequences if it were valid in
     * some other context (e.g., protocol B). */
    return_val = secp256k1_tagged_sha256(ctx, msg_hash, tag, sizeof(tag), msg, sizeof(msg));
    assert(return_val);

    /* Generate 32 bytes of randomness to use with BIP-340 schnorr signing. */
    if (!fill_random(auxiliary_rand, sizeof(auxiliary_rand))) {
        printf("Failed to generate randomness\n");
        return EXIT_FAILURE;
    }

    /* Generate a Schnorr signature.
     *
     * We use the secp256k1_schnorrsig_sign32 function that provides a simple
     * interface for signing 32-byte messages (which in our case is a hash of
     * the actual message). BIP-340 recommends passing 32 bytes of randomness
     * to the signing function to improve security against side-channel attacks.
     * Signing with a valid context, a 32-byte message, a verified keypair, and
     * any 32 bytes of auxiliary random data should never fail. */
    return_val = secp256k1_schnorrsig_sign32(ctx, signature, msg_hash, &keypair, auxiliary_rand);
    assert(return_val);

    /*** Verification ***/

    /* Deserialize the public key. This will return 0 if the public key can't
     * be parsed correctly */
    if (!secp256k1_xonly_pubkey_parse(ctx, &pubkey, serialized_pubkey)) {
        printf("Failed parsing the public key\n");
        return EXIT_FAILURE;
    }

    /* Compute the tagged hash on the received messages using the same tag as the signer. */
    return_val = secp256k1_tagged_sha256(ctx, msg_hash, tag, sizeof(tag), msg, sizeof(msg));
    assert(return_val);

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

    /* Bonus example: if all we need is signature verification (and no key
       generation or signing), we don't need to use a context created via
       secp256k1_context_create(). We can simply use the static (i.e., global)
       context secp256k1_context_static. See its description in
       include/secp256k1.h for details. */
    is_signature_valid2 = secp256k1_schnorrsig_verify(secp256k1_context_static,
                                                      signature, msg_hash, 32, &pubkey);
    assert(is_signature_valid2 == is_signature_valid);

    /* It's best practice to try to clear secrets from memory after using them.
     * This is done because some bugs can allow an attacker to leak memory, for
     * example through "out of bounds" array access (see Heartbleed), or the OS
     * swapping them to disk. Hence, we overwrite the secret key buffer with zeros.
     *
     * Here we are preventing these writes from being optimized out, as any good compiler
     * will remove any writes that aren't used. */
    secure_erase(seckey, sizeof(seckey));
    return EXIT_SUCCESS;
}
