# Support for Output Descriptors in Dash Core

Since Dash Core v0.17, there is support for Output Descriptors. This is a
simple language which can be used to describe collections of output scripts.
Supporting RPCs are:
- `scantxoutset` takes as input descriptors to scan for, and also reports
  specialized descriptors for the matching UTXOs.
- `getdescriptorinfo` analyzes a descriptor, and reports a canonicalized version
  with checksum added.
- `deriveaddresses` takes as input a descriptor and computes the corresponding
  addresses.
- `listunspent` outputs a specialized descriptor for the reported unspent outputs.
- `getaddressinfo` outputs a descriptor for solvable addresses (since v0.18).
- `importmulti` takes as input descriptors to import into a legacy wallet
  (since v0.18).
- `generatetodescriptor` takes as input a descriptor and generates coins to it
  (`regtest` only, since v0.19).
- `utxoupdatepsbt` takes as input descriptors to add information to the psbt
  (since v0.19).
- `createmultisig` and `addmultisigaddress` return descriptors as well (since v0.20).
- `importdescriptors` takes as input descriptors to import into a descriptor wallet
  (since v0.21).
- `listdescriptors` outputs descriptors imported into a descriptor wallet (since v22).

This document describes the language. For the specifics on usage, see the RPC
documentation for the functions mentioned above.

## Features

Output descriptors currently support:
- Pay-to-pubkey scripts (P2PK), through the `pk` function.
- Pay-to-pubkey-hash scripts (P2PKH), through the `pkh` function.
- Pay-to-script-hash scripts (P2SH), through the `sh` function.
- Multisig scripts, through the `multi` function.
- Multisig scripts where the public keys are sorted lexicographically, through the `sortedmulti` function.
- Any type of supported address through the `addr` function.
- Raw hex scripts through the `raw` function.
- Public keys (compressed and uncompressed) in hex notation, or BIP32 extended pubkeys with derivation paths.

## Examples

- `pk(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)` describes a P2PK output with the specified public key.
- `combo(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)` describes any P2PK, P2PKH with the specified public key.
- `multi(1,022f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)` describes a bare *1-of-2* multisig with the specified public key.
- `sortedmulti(1,022f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)` describes a bare *1-of-2* multisig with keys sorted lexicographically in the resulting redeemScript.
- `pkh(xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw/1/2)` describes a P2PKH output with child key *1/2* of the specified xpub.
- `pkh([d34db33f/44'/0'/0']xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL/1/*)` describes a set of P2PKH outputs, but additionally specifies that the specified xpub is a child of a master with fingerprint `d34db33f`, and derived using path `44'/0'/0'`.

## Reference

Descriptors consist of several types of expressions. The top level expression is either a `SCRIPT`, or `SCRIPT#CHECKSUM` where `CHECKSUM` is an 8-character alphanumeric descriptor checksum.

`SCRIPT` expressions:
- `pk(KEY)` (anywhere): P2PK output for the given public key.
- `pkh(KEY)` (anywhere): P2PKH output for the given public key (use `addr` if you only know the pubkey hash).
- `sh(SCRIPT)` (top level only): P2SH embed the argument.
- `combo(KEY)` (top level only): an alias for the collection of `pk(KEY)` and `pkh(KEY)`.
- `multi(k,KEY_1,KEY_2,...,KEY_n)` (anywhere): k-of-n multisig script.
- `sortedmulti(k,KEY_1,KEY_2,...,KEY_n)` (anywhere): k-of-n multisig script with keys sorted lexicographically in the resulting script.
- `addr(ADDR)` (top level only): the script which ADDR expands to.
- `raw(HEX)` (top level only): the script whose hex encoding is HEX.

`KEY` expressions:
- Optionally, key origin information, consisting of:
  - An open bracket `[`
  - Exactly 8 hex characters for the fingerprint of the key where the derivation starts (see BIP32 for details)
  - Followed by zero or more `/NUM` or `/NUM'` path elements to indicate unhardened or hardened derivation steps between the fingerprint and the key or xpub/xprv root that follows
  - A closing bracket `]`
- Followed by the actual key, which is either:
  - Hex encoded public keys (66 characters starting with `02` or `03`, or 130 characters starting with `04`).
  - [WIF](https://en.bitcoin.it/wiki/Wallet_import_format) encoded private keys may be specified instead of the corresponding public key, with the same meaning.
  -`xpub` encoded extended public key or `xprv` encoded private key (as defined in [BIP 32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)).
    - Followed by zero or more `/NUM` unhardened and `/NUM'` hardened BIP32 derivation steps.
    - Optionally followed by a single `/*` or `/*'` final step to denote all (direct) unhardened or hardened children.
    - The usage of hardened derivation steps requires providing the private key.

(Anywhere a `'` suffix is permitted to denote hardened derivation, the suffix `h` can be used instead.)

`ADDR` expressions are any type of supported address:
- P2PKH addresses (base58, of the form `X...`). Note that P2PKH addresses in descriptors cannot be used for P2PK outputs (use the `pk` function instead).
- P2SH addresses (base58, of the form `7...`, defined in [BIP 13](https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki)).

## Explanation

### Single-key scripts

Many single-key constructions are used in practice, generally including
P2PK and P2PKH. More combinations are
imaginable, though they may not be optimal: P2SH-P2PK and P2SH-P2PKH.

To describe these, we model these as functions. The functions `pk`
(P2PK) and `pkh` (P2PKH) take as input a public key in
hexadecimal notation (which will be extended later), and return the
corresponding *scriptPubKey*. The `sh` (P2SH) function
takes as input a script, and returns the script describing P2SH
outputs with the input as embedded script. The name of the function does
not contain "p2" for brevity.

### Multisig

Several pieces of software use multi-signature (multisig) scripts based
on Bitcoin's OP_CHECKMULTISIG opcode. To support these, we introduce the
`multi(k,key_1,key_2,...,key_n)` and `sortedmulti(k,key_1,key_2,...,key_n)`
functions. They represents a *k-of-n*
multisig policy, where any *k* out of the *n* provided public keys must
sign.

Key order is significant for `multi()`. A `multi()` expression describes a multisig script
with keys in the specified order, and in a search for TXOs, it will not match
outputs with multisig scriptPubKeys that have the same keys in a different
order. Also, to prevent a combinatorial explosion of the search space, if more
than one of the `multi()` key arguments is a BIP32 wildcard path ending in `/*`
or `*'`, the `multi()` expression only matches multisig scripts with the `i`th
child key from each wildcard path in lockstep, rather than scripts with any
combination of child keys from each wildcard path.

Key order does not matter for `sortedmulti()`. `sortedmulti()` behaves in the same way
as `multi()` does but the keys are reordered in the resulting script such that they
are lexicographically ordered as described in BIP67.

#### Basic multisig example

For a good example of a basic M-of-N multisig between multiple participants using descriptor
wallets and PSBTs, as well as a signing flow, see [this functional test](/test/functional/wallet_multisig_descriptor_psbt.py).

Disclaimers: It is important to note that this example serves as a quick-start and is kept basic for readability. A downside of the approach
outlined here is that each participant must maintain (and backup) two separate wallets: a signer and the corresponding multisig.
It should also be noted that privacy best-practices are not "by default" here - participants should take care to only use the signer to sign
transactions related to the multisig. Lastly, it is not recommended to use anything other than a Bitcoin Core descriptor wallet to serve as your
signer(s). Other wallets, whether hardware or software, likely impose additional checks and safeguards to prevent users from signing transactions that
could lead to loss of funds, or are deemed security hazards. Conforming to various 3rd-party checks and verifications is not in the scope of this example.

The basic steps are:

  1. Every participant generates an xpub. The most straightforward way is to create a new descriptor wallet which we will refer to as
     the participant's signer wallet. Avoid reusing this wallet for any purpose other than signing transactions from the
     corresponding multisig we are about to create. Hint: extract the wallet's xpubs using `listdescriptors` and pick the one from the
     `pkh` descriptor since it's least likely to be accidentally reused (legacy addresses)
  2. Create a watch-only descriptor wallet (blank, private keys disabled). Now the multisig is created by importing the two descriptors:
     `wsh(sortedmulti(<M>,XPUB1/0/*,XPUB2/0/*,…,XPUBN/0/*))` and `wsh(sortedmulti(<M>,XPUB1/1/*,XPUB2/1/*,…,XPUBN/1/*))`
     (one descriptor w/ `0` for receiving addresses and another w/ `1` for change). Every participant does this
  3. A receiving address is generated for the multisig. As a check to ensure step 2 was done correctly, every participant
     should verify they get the same addresses
  4. Funds are sent to the resulting address
  5. A sending transaction from the multisig is created using `walletcreatefundedpsbt` (anyone can initiate this). It is simple to do
     this in the GUI by going to the `Send` tab in the multisig wallet and creating an unsigned transaction (PSBT)
  6. At least `M` participants check the PSBT with their multisig using `decodepsbt` to verify the transaction is OK before signing it.
  7. (If OK) the participant signs the PSBT with their signer wallet using `walletprocesspsbt`. It is simple to do this in the GUI by
     loading the PSBT from file and signing it
  8. The signed PSBTs are collected with `combinepsbt`, finalized w/ `finalizepsbt`, and then the resulting transaction is broadcasted
     to the network. Note that any wallet (eg one of the signers or multisig) is capable of doing this.
  9. Checks that balances are correct after the transaction has been included in a block

You may prefer a daisy chained signing flow where each participant signs the PSBT one after another until
the PSBT has been signed `M` times and is "complete." For the most part, the steps above remain the same, except (6, 7)
change slightly from signing the original PSBT in parallel to signing it in series. `combinepsbt` is not necessary with
this signing flow and the last (`m`th) signer can just broadcast the PSBT after signing. Note that a parallel signing flow may be
preferable in cases where there are more signers. This signing flow is also included in the test / Python example.
[The test](/test/functional/wallet_multisig_descriptor_psbt.py) is meant to be documentation as much as it is a functional test, so
it is kept as simple and readable as possible.

### BIP32 derived keys and chains

Most modern wallet software and hardware uses keys that are derived using
BIP32 ("HD keys"). We support these directly by permitting strings
consisting of an extended public key (commonly referred to as an *xpub*)
plus derivation path anywhere a public key is expected. The derivation
path consists of a sequence of 0 or more integers (in the range
*0..2<sup>31</sup>-1*) each optionally followed by `'` or `h`, and
separated by `/` characters. The string may optionally end with the
literal `/*` or `/*'` (or `/*h`) to refer to all unhardened or hardened
child keys in a configurable range (by default `0-1000`, inclusive).

Whenever a public key is described using a hardened derivation step, the
script cannot be computed without access to the corresponding private
key.

### Key origin identification

In order to describe scripts whose signing keys reside on another device,
it may be necessary to identify the master key and derivation path an
xpub was derived with.

For example, when following BIP44, it would be useful to describe a
change chain directly as `xpub.../44'/0'/0'/1/*` where `xpub...`
corresponds with the master key `m`. Unfortunately, since there are
hardened derivation steps that follow the xpub, this descriptor does not
let you compute scripts without access to the corresponding private keys.
Instead, it should be written as `xpub.../1/*`, where xpub corresponds to
`m/44'/0'/0'`.

When interacting with a hardware device, it may be necessary to include
the entire path from the master down. BIP174 standardizes this by
providing the master key *fingerprint* (first 32 bit of the Hash160 of
the master pubkey), plus all derivation steps. To support constructing
these, we permit providing this key origin information inside the
descriptor language, even though it does not affect the actual
scriptPubKeys it refers to.

Every public key can be prefixed by an 8-character hexadecimal
fingerprint plus optional derivation steps (hardened and unhardened)
surrounded by brackets, identifying the master and derivation path the key or xpub
that follows was derived with.

### Including private keys

Often it is useful to communicate a description of scripts along with the
necessary private keys. For this reason, anywhere a public key or xpub is
supported, a private key in WIF format or xprv may be provided instead.
This is useful when private keys are necessary for hardened derivation
steps, for signing transactions, or for dumping wallet descriptors
including private key material.

For example, after importing the following 2-of-3 multisig descriptor
into a wallet, one could use `signrawtransactionwithwallet`
to sign a transaction with the first key:
```
sh(multi(2,xprv.../84'/0'/0'/0/0,xpub1...,xpub2...))
```
Note how the first key is an xprv private key with a specific derivation path,
while the other two are public keys.


### Compatibility with old wallets

In order to easily represent the sets of scripts currently supported by
existing Dash Core wallets, a convenience function `combo` is
provided, which takes as input a public key, and describes a set of P2PK and
P2PKH scripts for that key.

### Checksums

Descriptors can optionally be suffixed with a checksum to protect against
typos or copy-paste errors.

These checksums consist of 8 alphanumeric characters. As long as errors are
restricted to substituting characters in `0123456789()[],'/*abcdefgh@:$%{}`
for others in that set and changes in letter case, up to 4 errors will always
be detected in descriptors up to 501 characters, and up to 3 errors in longer
ones. For larger numbers of errors, or other types of errors, there is a
roughly 1 in a trillion chance of not detecting the errors.

All RPCs in Dash Core will include the checksum in their output. Only
certain RPCs require checksums on input, including `deriveaddress` and
`importmulti`. The checksum for a descriptor without one can be computed
using the `getdescriptorinfo` RPC.
