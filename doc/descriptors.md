# Support for Output Descriptors in Bitcoin Core

Since Bitcoin Core v0.17, there is support for Output Descriptors in the
`scantxoutset` RPC call. This is a simple language which can be used to
describe collections of output scripts.

This document describes the language. For the specifics on usage for scanning
the UTXO set, see the `scantxoutset` RPC help.

## Features

Output descriptors currently support:
- Pay-to-pubkey scripts (P2PK), through the `pk` function.
- Pay-to-pubkey-hash scripts (P2PKH), through the `pkh` function.
- Pay-to-witness-pubkey-hash scripts (P2WPKH), through the `wpkh` function.
- Pay-to-script-hash scripts (P2SH), through the `sh` function.
- Pay-to-witness-script-hash scripts (P2WSH), through the `wsh` function.
- Multisig scripts, through the `multi` function.
- Any type of supported address through the `addr` function.
- Raw hex scripts through the `raw` function.
- Public keys (compressed and uncompressed) in hex notation, or BIP32 extended pubkeys with derivation paths.

## Examples

- `pk(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)` represents a P2PK output.
- `pkh(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)` represents a P2PKH output.
- `wpkh(02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9)` represents a P2WPKH output.
- `sh(wpkh(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))` represents a P2SH-P2WPKH output.
- `combo(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)` represents a P2PK, P2PKH, P2WPKH, and P2SH-P2WPKH output.
- `sh(wsh(pkh(02e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13)))` represents a (overly complicated) P2SH-P2WSH-P2PKH output.
- `multi(1,022f8bde4d1a07209355b4a7250a5c5128e88b84bddc619ab7cba8d569b240efe4,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)` represents a bare *1-of-2* multisig.
- `sh(multi(2,022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01,03acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbe))` represents a P2SH *2-of-2* multisig.
- `wsh(multi(2,03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7,03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a))` represents a P2WSH *2-of-3* multisig.
- `sh(wsh(multi(1,03f28773c2d975288bc7d1d205c3748651b075fbc6610e58cddeeddf8f19405aa8,03499fdf9e895e719cfd64e67f07d38e3226aa7b63678949e6e49b241a60e823e4,02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e)))` represents a P2SH-P2WSH *1-of-3* multisig.
- `pk(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8)` refers to a single P2PK output, using the public key part from the specified xpub.
- `pkh(xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw/1'/2)` refers to a single P2PKH output, using child key *1'/2* of the specified xpub.
- `wsh(multi(1,xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/1/0/*,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/0/*))` refers to a chain of *1-of-2* P2WSH multisig outputs, using public keys taken from two HD chains with corresponding derivation paths.

## Reference

Descriptors consist of several types of expressions. The top level expression is always a `SCRIPT`.

`SCRIPT` expressions:
- `sh(SCRIPT)` (top level only): P2SH embed the argument.
- `wsh(SCRIPT)` (not inside another 'wsh'): P2WSH embed the argument.
- `pk(KEY)` (anywhere): P2PK output for the given public key.
- `pkh(KEY)` (anywhere): P2PKH output for the given public key (use `addr` if you only know the pubkey hash).
- `wpkh(KEY)` (not inside `wsh`): P2WPKH output for the given compressed pubkey.
- `combo(KEY)` (top level only): an alias for the collection of `pk(KEY)` and `pkh(KEY)`. If the key is compressed, it also includes `wpkh(KEY)` and `sh(wpkh(KEY))`.
- `multi(k,KEY_1,KEY_2,...,KEY_n)` (anywhere): k-of-n multisig script.
- `addr(ADDR)` (top level only): the script which ADDR expands to.
- `raw(HEX)` (top level only): the script whose hex encoding is HEX.

`KEY` expressions:
- Hex encoded public keys (66 characters starting with `02` or `03`, or 130 characters starting with `04`).
  - Inside `wpkh` and `wsh`, only compressed public keys are permitted.
- [WIF](https://en.bitcoin.it/wiki/Wallet_import_format) encoded private keys may be specified instead of the corresponding public key, with the same meaning.
-`xpub` encoded extended public key or `xprv` encoded private key (as defined in [BIP 32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)).
  - Followed by zero or more `/NUM` unhardened and `/NUM'` hardened BIP32 derivation steps.
  - Optionally followed by a single `/*` or `/*'` final step to denote all (direct) unhardened or hardened children.
  - The usage of hardened derivation steps requires providing the private key.
  - Instead of a `'`, the suffix `h` can be used to denote hardened derivation.

`ADDR` expressions are any type of supported address:
- P2PKH addresses (base58, of the form `1...`). Note that P2PKH addresses in descriptors cannot be used for P2PK outputs (use the `pk` function instead).
- P2SH addresses (base58, of the form `3...`, defined in [BIP 13](https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki)).
- Segwit addresses (bech32, of the form `bc1...`, defined in [BIP 173](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki)).

## Explanation

### Single-key scripts

Many single-key constructions are used in practice, generally including
P2PK, P2PKH, P2WPKH, and P2SH-P2WPKH. Many more combinations are
imaginable, though they may not be optimal: P2SH-P2PK, P2SH-P2PKH,
P2WSH-P2PK, P2WSH-P2PKH, P2SH-P2WSH-P2PK, P2SH-P2WSH-P2PKH.

To describe these, we model these as functions. The functions `pk`
(P2PK), `pkh` (P2PKH) and `wpkh` (P2WPKH) take as input a public key in
hexadecimal notation (which will be extended later), and return the
corresponding *scriptPubKey*. The functions `sh` (P2SH) and `wsh` (P2WSH)
take as input a script, and return the script describing P2SH and P2WSH
outputs with the input as embedded script. The names of the functions do
not contain "p2" for brevity.

### Multisig

Several pieces of software use multi-signature (multisig) scripts based
on Bitcoin's OP_CHECKMULTISIG opcode. To support these, we introduce the
`multi(k,key_1,key_2,...,key_n)` function. It represents a *k-of-n*
multisig policy, where any *k* out of the *n* provided public keys must
sign.

### BIP32 derived keys and chains

Most modern wallet software and hardware uses keys that are derived using
BIP32 ("HD keys"). We support these directly by permitting strings
consisting of an extended public key (commonly referred to as an *xpub*)
plus derivation path anywhere a public key is expected. The derivation
path consists of a sequence of 0 or more integers (in the range
*0..2<sup>31</sup>-1*) each optionally followed by `'` or `h`, and
separated by `/` characters. The string may optionally end with the
literal `/*` or `/*'` (or `/*h`) to refer to all unhardened or hardened
child keys instead.

Whenever a public key is described using a hardened derivation step, the
script cannot be computed without access to the corresponding private
key.

### Including private keys

Often it is useful to communicate a description of scripts along with the
necessary private keys. For this reason, anywhere a public key or xpub is
supported, a private key in WIF format or xprv may be provided instead.
This is useful when private keys are necessary for hardened derivation
steps, or for dumping wallet descriptors including private key material.

### Compatibility with old wallets

In order to easily represent the sets of scripts currently supported by
existing Bitcoin Core wallets, a convenience function `combo` is
provided, which takes as input a public key, and constructs the P2PK,
P2PKH, P2WPKH, and P2SH-P2WPH scripts for that key. In case the key is
uncompressed, it only constructs P2PK and P2PKH.
