Wallet RPC
----------

The new `exportaddresskey "address" ( "key_type" )` RPC returns local private keys for a
recognized non-Taproot address in the selected descriptor wallet. This includes
P2PKH, native P2WPKH, P2SH-P2WPKH, recognized P2SH scripts, P2WSH and
P2SH-P2WSH, with pk/pkh, multisig/sorted multisig and supported Miniscript.
Every locally available matching key is returned, including partial multisig
ownership and keys on branches requiring unavailable preimages or timelocks.
Matching metadata and keys are combined across the selected wallet's descriptors.
Already-known stored/cached keys can also be exported for directly verified
P2PKH, P2WPKH and P2SH-P2WPKH forms without generating new addresses.
Encrypted wallets
must be unlocked; wallets with private keys disabled or external signers are
not supported. `key_type` defaults to `"wallet"` when omitted or null. The only
other accepted value is `"taproot_output"`, which requires a Taproot address;
Taproot export is currently unsupported.

The result contains a canonical `keys` array sorted by full serialized public
key. Each record contains `pubkey` (the full SEC public key), `private_key_hex`
(the 32-byte unsigned big-endian scalar, encoded as 64 lowercase hex characters,
including leading zeros), `compressed` (the public-key compression flag), and
sorted unique `roles` (currently `"script"`). Identical public keys from multiple
matching descriptors produce one record. For the original P2PKH/native P2WPKH
wallet-mode exports, the top-level `private_key_hex` and `compressed` fields
remain compatibility aliases with their original values.

Export is limited to existing wallet metadata and known derivation indices.
For script-hash addresses, the wallet must know the committed redeem/witness
scripts. Missing script information produces an error explaining that the wallet
lacks the spending/script information needed to identify/export keys. This differs
from a recognized construction with no local private key, or unsupported script
semantics. The RPC does not recover unknown script hashes, search unknown HD
paths, or infer signing roles from arbitrary key-shaped bytes.

An exported scalar is a spending secret, not WIF. The result is not a wallet
backup; scripts, cosigners or other spending data may still be required.
Disclosure of a non-hardened child key together with the corresponding
ancestor xpub can compromise that ancestor's private subtree. The RPC console
displays the result; use protected, authenticated RPC when handling it.

Ported from Bitcoin Roots by Jaromil (`fad94e12f7`), including the destination
comparison fix (`59185c317e`).
