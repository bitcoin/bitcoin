Description
------------

This directory contains data-driven tests for various aspects of Bitcoin.

License
--------

The data files in this directory are distributed under the MIT software
license, see the accompanying file COPYING or
https://www.opensource.org/licenses/mit-license.php.

`bip322_vectors.json` contains selected, unmodified BIP322 test cases from
bitcoin/bips commit a34f044bb8d5cd684d7a742d2f6483c14b719046 (BIP322 2.1.0).
These upstream cases are licensed CC0-1.0, as specified in BIP322. The fixture
records source URLs and SHA256 hashes, original section indices, and an explicit
scope map. It includes every published simple signature and basic hash case,
all basic errors, and the generated simple errors. Full signatures and proofs
of funds, including their generated error cases, are classified as unsupported.
No test downloads or regenerates these external expected values.

