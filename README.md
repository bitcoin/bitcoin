BTCSQ
=====

This repository contains BTCSQ - Bitcoin Squared implementation. A Bitcoin fork which implements [Proof-Of-Proof](./https://wiki.veriblock.org/index.php/VeriBlock_and_Proof-of-Proof_FAQ) protocol by VeriBlock.

Most of the functionality is implemented in a common SDK - https://github.com/VeriBlock/alt-integration-cpp.
Version of this SDK can be found in [depends/packages/veriblock-pop-cpp.mk](depends/packages/veriblock-pop-cpp.mk).
Small BTCSQ-specific part is implemented in [./src/vbk](./src/vbk) folder.

Fork created at Bitcoin [8830cb58abc888144a1edb9b2fba427716cc45d8](https://github.com/bitcoin/bitcoin/commit/8830cb58abc888144a1edb9b2fba427716cc45d8).

Mainnet genesis block contains coins from previous testnet/mainnet network runs squashed into single coinbase transaction.

Have fun!
