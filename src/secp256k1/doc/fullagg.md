Notes on the fullagg module API
===============================

The following sections contain additional notes on the API of the fullagg module (`include/secp256k1_fullagg.h`).
A usage example can be found in `examples/fullagg.c`.

## API misuse

The fullagg API is designed with a focus on misuse resistance.
However, due to the interactive nature of the signing protocol, there are additional failure modes that are not present in regular (single-party) signature creation.
While the results can be catastrophic (e.g. leaking of the secret key), it is unfortunately not possible for the fullagg implementation to prevent all such failure modes.

Therefore, users of the fullagg module must take great care to make sure of the following:

1. A unique nonce per signing session is generated in `secp256k1_fullagg_nonce_gen`.
   See the corresponding comment in `include/secp256k1_fullagg.h` for how to ensure that.
2. The `secp256k1_fullagg_secnonce` structure is never copied or serialized.
   See also the comment on `secp256k1_fullagg_secnonce` in `include/secp256k1_fullagg.h`.
3. Opaque data structures are never written to or read from directly.
   Instead, only the provided accessor functions are used.
4. The session is initialized with `secp256k1_fullagg_session_init` by the signer itself, using the lists of public keys, messages, and public nonces it received.
   A session obtained from an untrusted third party such as the coordinator must not be used for signing.

## Signing

Each signer signs their own message under their own public key; there is no aggregate public key.
Essentially, the protocol proceeds in the following steps:

1. Generate a keypair with `secp256k1_keypair_create` and obtain the x-only public key with `secp256k1_keypair_xonly_pub`.
2. Generate a pair of secret and public nonce with `secp256k1_fullagg_nonce_gen` and send the public nonce to the coordinator.
3. The coordinator aggregates the public nonces of all signers with `secp256k1_fullagg_nonce_agg` and sends the aggregate nonce back to the signers, along with the ordered lists of all public keys, messages, and public nonces.
4. Initialize the session with `secp256k1_fullagg_session_init`.
5. Create a partial signature with `secp256k1_fullagg_partial_sign`.
6. Verify the partial signatures (optional in some scenarios) with `secp256k1_fullagg_partial_sig_verify`.
7. The coordinator obtains all partial signatures and aggregates them into the final signature using `secp256k1_fullagg_partial_sig_agg`.

The aggregate signature can be verified with `secp256k1_fullagg_verify`.
The signature depends on the order of the lists of public keys, messages, and public nonces, so all signers, the coordinator, and the verifier must use the same order.

Steps 1 and 2 above can occur as a preprocessing step before the messages or the set of signers are known.
However, applications that use this method presumably store the nonces for a longer time and must be even more careful not to reuse them.

## Tweaking

The scheme supports additive tweaking of individual key pairs without changes to the verification algorithm.
Since the keys are individual keys rather than an aggregate, the existing key APIs are used and no fullagg-specific functions are needed.
A plain tweak as used for BIP 32 unhardened derivation can be applied to the secret key with `secp256k1_ec_seckey_tweak_add`.
A Taproot tweak as defined in BIP 341 can be applied to the keypair with `secp256k1_keypair_xonly_tweak_add`.
The tweaked x-only public key appears in the public key list and the signer uses the tweaked keypair in `secp256k1_fullagg_partial_sign`.

## Verification

A participant who wants to verify the partial signatures, but does not sign itself, may do so using the above instructions except that the participant skips steps 1, 2, 4, 5 and 7.
A valid partial signature does not prove that its signer knows the corresponding secret key, so partial signature verification only serves to identify disruptive signers, and only if the public nonces were received over authenticated channels.
