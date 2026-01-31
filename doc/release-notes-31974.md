Notable changes
===============

Testnet3 removed
-----
Support for Testnet4 as specified in [BIP94](https://github.com/bitcoin/bips/blob/master/bip-0094.mediawiki)
was added in v28.0, which also deprecated Testnet3. This release removes
Testnet3 support entirely.

Testnet3 may continue to exist indefinitely. It is still supported by v29.x
releases. It may become more difficult over time to bootstrap a new node,
as the DNS seeds and hardcoded seed nodes may go offline. If that happens
it may help to manually connect to a known peer using the `addnode` RPC.
