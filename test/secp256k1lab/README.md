secp256k1lab
============

![Dependencies: None](https://img.shields.io/badge/dependencies-none-success)

An INSECURE implementation of the secp256k1 elliptic curve and related cryptographic schemes written in Python, intended for prototyping, experimentation and education.

Features:
* Low-level secp256k1 field and group arithmetic.
* Schnorr signing/verification and key generation according to [BIP-340](https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki).
* ECDH key exchange.

WARNING: The code in this library is slow and trivially vulnerable to side channel attacks.
