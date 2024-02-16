26.1rc1 Release Notes
==================

Bitcoin Core version 26.1rc1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-26.1/test.rc1/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 11.0+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Wallet

- #28994 wallet: skip BnB when SFFO is enabled
- #28920 wallet: birth time update during tx scanning
- #29176 wallet: Fix use-after-free in WalletBatch::EraseRecords

### RPC

- #29003 rpc: fix getrawtransaction segfault

### Logs

- #29227 log mempool loading progress

### P2P and network changes

- #29200 net: create I2P sessions using both ECIES-X25519 and ElGamal encryption

### Build

- #29127 Use hardened runtime on macOS release builds.
- #29195 build: Fix -Xclang -internal-isystem option

### CI

- #28992 ci: Use Ubuntu 24.04 Noble for asan,tsan,tidy,fuzz
- #29080 ci: Set HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK to avoid unrelated failures

### Miscellaneous

- #28391 refactor: Simplify CTxMempool/BlockAssembler fields, remove some external mapTx access
- #29179 test: wallet rescan with reorged parent + IsFromMe child in mempool
- #28791 snapshots: don't core dump when running -checkblockindex after loadtxoutset

Credits
=======

Thanks to everyone who directly contributed to this release:

- fanquake
- furszy
- glozow
- Hennadii Stepanov
- Jon Atack
- MarcoFalke
- Mark Friedenbach
- Martin Zumsande
- Murch
- stickies-v

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
