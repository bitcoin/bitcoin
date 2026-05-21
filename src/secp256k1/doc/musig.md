Notes on the musig module API
===========================

The following sections contain additional notes on the API of the musig module (`include/secp256k1_musig.h`).
A usage example can be found in `examples/musig.c`.

## API misuse

The musig API is designed with a focus on misuse resistance.
However, due to the interactive nature of the MuSig protocol, there are additional failure modes that are not present in regular (single-party) Schnorr signature creation.
While the results can be catastrophic (e.g. leaking of the secret key), it is unfortunately not possible for the musig implementation to prevent all such failure modes.

Therefore, users of the musig module must take great care to make sure of the following:

1. A unique nonce per signing session is generated in `secp256k1_musig_nonce_gen`.
   See the corresponding comment in `include/secp256k1_musig.h` for how to ensure that.
2. The `secp256k1_musig_secnonce` structure is never copied or serialized.
   See also the comment on `secp256k1_musig_secnonce` in `include/secp256k1_musig.h`.
3. Opaque data structures are never written to or read from directly.
   Instead, only the provided accessor functions are used.

## Key Aggregation and (Taproot) Tweaking

Given a set of public keys, the aggregate public key is computed with `secp256k1_musig_pubkey_agg`.
A plain tweak can be added to the resulting public key with `secp256k1_ec_pubkey_tweak_add` by setting the `tweak32` argument to the hash defined in BIP 32. Similarly, a Taproot tweak can be added with `secp256k1_xonly_pubkey_tweak_add` by setting the `tweak32` argument to the TapTweak hash defined in BIP 341.
Both types of tweaking can be combined and invoked multiple times if the specific application requires it.

## Signing

This is covered by `examples/musig.c`.
Essentially, the protocol proceeds in the following steps:

1. Generate a keypair with `secp256k1_keypair_create` and obtain the public key with `secp256k1_keypair_pub`.
2. Call `secp256k1_musig_pubkey_agg` with the pubkeys of all participants.
3. Optionally add a (Taproot) tweak with `secp256k1_musig_pubkey_xonly_tweak_add` and a plain tweak with `secp256k1_musig_pubkey_ec_tweak_add`.
4. Generate a pair of secret and public nonce with `secp256k1_musig_nonce_gen` and send the public nonce to the other signers.
5. Someone (not necessarily the signer) aggregates the public nonces with `secp256k1_musig_nonce_agg` and sends it to the signers.
6. Process the aggregate nonce with `secp256k1_musig_nonce_process`.
7. Create a partial signature with `secp256k1_musig_partial_sign`.
8. Verify the partial signatures (optional in some scenarios) with `secp256k1_musig_partial_sig_verify`.
9. Someone (not necessarily the signer) obtains all partial signatures and aggregates them into the final Schnorr signature using `secp256k1_musig_partial_sig_agg`.

The aggregate signature can be verified with `secp256k1_schnorrsig_verify`.

Steps 1 through 5 above can occur before or after the signers are aware of the message to be signed.
Whenever possible, it is recommended to generate the nonces only after the message is known.
This provides enhanced defense-in-depth measures, protecting against potential API misuse in certain scenarios.
However, it does require two rounds of communication during the signing process.
The alternative, generating the nonces in a pre-processing step before the message is known, eliminates these additional protective measures but allows for non-interactive signing.
Similarly, the API supports an alternative protocol flow where generating the aggregate key (steps 1 to 3) is allowed to happen after exchanging nonces (steps 4 to 5).

## Verification

A participant who wants to verify the partial signatures, but does not sign itself may do so using the above instructions except that the verifier skips steps 1, 4 and 7.
