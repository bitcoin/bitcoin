Bitcoin Core version 29.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-29.x/>

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

Bitcoin Core is supported and tested on operating systems using the
Linux Kernel 3.17+, macOS 13+, and Windows 10+. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Wallet

- #32553 wallet: Fix logging of wallet version

### Test

- #32286 test: Handle empty string returned by CLI as None in RPC tests
- #32312 test: Fix feature_pruning test after nTime typo fix
- #32336 test: Suppress upstream -Wduplicate-decl-specifier in bpfcc
- #32483 test: fix two intermittent failures in wallet_basic.py

### Util

- #32248 Remove support for RNDR/RNDRRS for aarch64

### Build

- #32356 cmake: Respect user-provided configuration-specific flags
- #32437 crypto: disable ASan for sha256_sse4 with Clang
- #32469 cmake: Allow WITH_DBUS on all Unix-like systems

### Gui

- #864 Crash fix, disconnect numBlocksChanged() signal during shutdown
- #868 Replace stray tfm::format to cerr with qWarning

### Doc

- #32353 doc: Fix fuzz test_runner.py path
- #32389 doc: Fix test_bitcoin path

### CI

- #32184 ci: Add workaround for vcpkg's libevent package

### Misc

- #32187 refactor: Remove spurious virtual from final ~CZMQNotificationInterface
- #32454 tracing: fix invalid argument in mempool_monitor

Credits
=======

Thanks to everyone who directly contributed to this release:

- achow101
- Brandon Odiwuor
- enirox001
- fanquake
- furszy
- Hennadii Stepanov
- jb55
- laanwj
- MarcoFalke
- Martin Zumsande
- monlovesmango

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
