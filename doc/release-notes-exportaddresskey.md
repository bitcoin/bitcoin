Wallet RPC
----------

The new `exportaddresskey "address" ( "key_type" )` RPC returns local private keys for a
known P2PKH or native P2WPKH address in the selected wallet. Encrypted wallets
must be unlocked; wallets with private keys disabled or external signers are
not supported. `key_type` defaults to `"wallet"` when omitted or null. The only
other accepted value is `"taproot_output"`, which requires a Taproot address;
Taproot export is currently unsupported. P2SH, P2WSH and P2TR addresses remain
unsupported in this release.

The result contains a canonical `keys` array sorted by full serialized public
key. Each record contains `pubkey` (the full SEC public key), `private_key_hex`
(the 32-byte unsigned big-endian scalar, encoded as 64 lowercase hex characters,
including leading zeros), `compressed` (the public-key compression flag), and
sorted unique `roles` (currently `"script"`). Identical public keys from multiple
matching descriptors produce one record. For the original P2PKH/native P2WPKH
wallet-mode exports, the top-level `private_key_hex` and `compressed` fields
remain compatibility aliases with their original values.

An exported scalar is a spending secret, not WIF. The result is not a wallet
backup; scripts, cosigners or other spending data may still be required.
Disclosure of a non-hardened child key together with the corresponding
ancestor xpub can compromise that ancestor's private subtree. The RPC console
displays the result; use protected, authenticated RPC when handling it.

Ported from Bitcoin Roots by Jaromil (`fad94e12f7`), including the destination
comparison fix (`59185c317e`).
