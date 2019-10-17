[![Status](https://travis-ci.org/rust-bitcoin/bitcoin_hashes.png?branch=master)](https://travis-ci.org/rust-bitcoin/bitcoin_hashes)

# Bitcoin Hashes Library

This is a simple, no-dependency library which implements the hash functions
needed by Bitcoin. These are SHA1, SHA256, SHA256d, and RIPEMD160. As an
ancilliary thing, it exposes hexadecimal serialization and deserialization,
since these are needed to display hashes anway.

Ok, there is one dependency -- `byteorder` -- which is used to manage endianness.

[Documentation](https://docs.rs/bitcoin_hashes/)

## Contributions

Contributions are welcome, including additional hash function implementations.

