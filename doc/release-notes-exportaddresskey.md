Wallet RPC
----------

The new `exportaddresskey "address" ( "key_type" )` RPC exports all verifiably
matching local private keys from the selected descriptor wallet. ADDRESS is
required. It supports P2PKH (both SEC serializations), native P2WPKH,
P2SH-P2WPKH, recognized P2SH/P2WSH/P2SH-P2WSH scripts, and Taproot `tr()`
and `rawtr()`. Recognized constructions include pk/pkh, multisig/sorted
multisig, supported Miniscript, Taproot multi_a/sortedmulti_a, and original
`musig()` participants. Matching metadata and concrete keys are combined
across the selected wallet's descriptors at already-known indices. Directly
verifiable stored/cached keys can be exported without generating an address.

Encrypted wallets must be unlocked. Private-key-disabled and external-signer
wallets cannot export locally. Omitted or null `key_type` means `"wallet"`:

- `wallet` returns original stored/derived scalars and their full SEC
  serializations. It includes internal, direct output, recognized leaf and
  MuSig2 participant keys. A single available key is enough for success, even
  when other keys, preimages, timelocks or cooperation are required to spend.
- `taproot_output` requires a Taproot address and returns locally available
  effective key-path scalars. It applies Core's TapTweak to a verified internal
  key with the exact known commitment, including tree-less `tr(KEY)`. A
  directly held output/`rawtr()` key is returned without another tweak.
  Missing tree data is not treated as an empty tree. Script-only or MuSig2
  participant-only ownership can succeed in wallet mode while output mode
  fails with guidance to use wallet mode. Participant secrets are never
  combined into an aggregate private key, even when every participant is local.

The result is an object with a canonical `keys` array sorted by full serialized
public key. Each record contains `pubkey` (the full SEC public key corresponding
to the returned scalar), `private_key_hex` (a 32-byte unsigned big-endian scalar
encoded as 64 lowercase hex characters, including leading zeros), `compressed`
(the SEC compression flag), and sorted unique `roles`:

| Role | Meaning |
|------|---------|
| `script` | Key used by a recognized non-Taproot construction. |
| `taproot_internal` | Original wallet internal key, without silent tweaking or parity normalization. |
| `taproot_script` | Key used by a recognized committed Taproot leaf. |
| `taproot_output` | Directly held or explicitly derived effective output key. |
| `musig2_participant` | Original concrete participant key, not an aggregate/output secret. |

Identical full public keys merge across descriptors and roles; different held
SEC identities remain distinct. For x-only roles, both held parity
representatives are considered. MuSig2 participant matching preserves the full
point, including parity, while allowing both held SEC serializations of that
point. Output mode reports actual scalar/public-key parity without an extra
even-Y convention. For original P2PKH/native P2WPKH wallet-mode exports,
top-level `private_key_hex` and `compressed` remain compatibility aliases with
their original values. Newly supported addresses use only `keys`.

For example, `exportaddresskey "ADDRESS"` exports matching wallet keys;
`exportaddresskey "TAPROOT_ADDRESS" "taproot_output"` explicitly requests
effective output keys. The result is a key export, not a self-contained backup
or spending recipe: scripts, control blocks, cosigners, preimages and other
spending data may still be needed.

Export is bounded by existing wallet metadata, stored/cached public keys,
recognized semantics and known derivation indices. Missing committed
redeem/witness/Taproot information has a different error from a recognized
association with no local key or unsupported semantics. The RPC does not recover
unknown script hashes, search unknown HD paths, recognize future witness/leaf
semantics, or infer keys from arbitrary key-shaped bytes. Corrupt known
derivations or inconsistent commitments fail the whole call without partial
success.

Exported scalars are spending secrets, not WIF. A participant reused before
post-aggregation BIP32 derivation can affect many outputs, as can an explicitly
reused key; exports are not necessarily address-exclusive or leaf-only.
Disclosure of a non-hardened child scalar together with its corresponding
ancestor xpub can compromise the ancestor's private subtree. Use protected,
authenticated RPC. Responses, JSON/client buffers and console output can retain
secrets; this RPC does not guarantee complete erasure of those copies.

Ported from Bitcoin Roots by Jaromil (`fad94e12f7`), including the destination
comparison fix (`59185c317e`).
