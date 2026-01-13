# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

#### Added
 - Added new methods `Scalar.from_int_nonzero_checked` and `Scalar.from_bytes_nonzero_checked`
   that ensure a constructed scalar is in the range `0 < s < N` (i.e. is non-zero and within the
   group order) and throw a `ValueError` otherwise. This is e.g. useful for ensuring that newly
   generated secret keys or nonces are valid without having to do the non-zero check manually.
   The already existing methods `Scalar.from_int_checked` and `Scalar.from_bytes_checked` error
   on overflow, but not on zero, i.e. they only ensure `0 <= s < N`.

 - Added a new method `GE.from_bytes_compressed_with_infinity` to parse a compressed
   public key (33 bytes) to a group element, where the all-zeros bytestring maps to the
   point at infinity. This is the counterpart to the already existing serialization
   method `GE.to_bytes_compressed_with_infinity`.

## [1.0.0] - 2025-03-31

Initial release.
