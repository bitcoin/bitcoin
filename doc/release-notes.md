25.2rc1 Release Notes
==================

Bitcoin Core version 25.2rc1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-25.2/test.rc1>

This release includes various bug fixes and performance
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
using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Gui

- gui#774 Fix crash on selecting "Mask values" in transaction view

### RPC

- #29003 rpc: fix getrawtransaction segfault

### Wallet

- #29176 wallet: Fix use-after-free in WalletBatch::EraseRecords

Credits
=======

Thanks to everyone who directly contributed to this release:

- Martin Zumsande
- Sebastian Falbesoner
- MarcoFalke

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
