Wallet RPC
----------

The new `exportaddresskey "address"` RPC returns the local spending key for a
known P2PKH or native P2WPKH address in the selected wallet. Encrypted wallets
must be unlocked; wallets with private keys disabled or external signers are
not supported. P2SH, P2WSH and P2TR addresses are not supported.

The result contains `private_key_hex` (the 32-byte unsigned big-endian scalar,
encoded as 64 lowercase hex characters, including leading zeros) and
`compressed` (the public-key compression flag). This is a spending secret,
not WIF. Disclosure of a non-hardened child key together with the corresponding
ancestor xpub can compromise that ancestor's private subtree. The RPC console
displays the result; use protected, authenticated RPC when handling it.

Ported from Bitcoin Roots by Jaromil (`fad94e12f7`), including the destination
comparison fix (`59185c317e`).
