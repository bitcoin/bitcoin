libsecp256k1
============

[![Build Status](https://travis-ci.org/bitcoin/secp256k1.svg?branch=master)](https://travis-ci.org/bitcoin/secp256k1)

Optimized C library for EC operations on curve secp256k1.

This library is experimental, so use at your own risk.

Features:
* Low-level field and group operations on secp256k1.
* ECDSA signing/verification and key generation.
* Adding/multiplying private/public keys.
* Serialization/parsing of private keys, public keys, signatures.
* Very efficient implementation.

Implementation details
----------------------

* General
  * Avoid dynamic memory usage almost everywhere.
* Field operations
  * Optimized implementation of arithmetic modulo the curve's field size (2^256 - 0x1000003D1).
    * Using 5 52-bit limbs (including hand-optimized assembly for x86_64, by Diederik Huys).
    * Using 10 26-bit limbs.
    * Using GMP.
  * Field inverses and square roots using a sliding window over blocks of 1s (by Peter Dettman).
* Group operations
  * Point addition formula specifically simplified for the curve equation (y^2 = x^3 + 7).
  * Use addition between points in Jacobian and affine coordinates where possible.
* Point multiplication for verification (a*P + b*G).
  * Use wNAF notation for point multiplicands.
  * Use a much larger window for multiples of G, using precomputed multiples.
  * Use Shamir's trick to do the multiplication with the public key and the generator simultaneously.
  * Optionally use secp256k1's efficiently-computable endomorphism to split the multiplicands into 4 half-sized ones first.
* Point multiplication for signing
  * Use a precomputed table of multiples of powers of 16 multiplied with the generator, so general multiplication becomes a series of additions.
  * Slice the precomputed table in memory per byte, so memory access to the table becomes uniform.
  * Not fully constant-time, but the precomputed tables add and eventually subtract points for which no known scalar (private key) is known, blinding non-constant time effects even from an attacker with control over the private key used.

Build steps
-----------

libsecp256k1 is built using autotools:

    $ ./autogen.sh
    $ ./configure
    $ make
    $ sudo make install  # optional
