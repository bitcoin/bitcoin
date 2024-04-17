Bitcoin Core version 27.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-27.x/>

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
using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Miniscript

- #29853 sign: don't assume we are parsing a sane TapMiniscript

### RPC

- #29869 rpc, bugfix: Enforce maximum value for setmocktime
- #29870 rpc: Reword SighashFromStr error message

### Index

- #29776 Fix #29767, set m_synced = true after Commit()

### Test

- #29892 test: Fix failing univalue float test

### Build

- #29747 depends: fix mingw-w64 Qt DEBUG=1 build
- #29859 build: Fix false positive CHECK_ATOMIC test
- #29985 depends: Fix build of Qt for 32-bit platforms with recent glibc

### Doc

- #29934 doc: add LLVM instruction for macOS < 13

### CI

- #29856 ci: Bump s390x to ubuntu:24.04

### Misc

- #29691 Change Luke Dashjr seed to dashjr-list-of-p2p-nodes.us

Credits
=======

Thanks to everyone who directly contributed to this release:

- Antoine Poinsot
- dergoegge
- fanquake
- Hennadii Stepanov
- laanwj
- Luke Dashjr
- MarcoFalke
- nanlour
- Sjors Provoost

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
