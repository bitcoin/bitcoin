24.1 Release Notes
==================

Tortoisecoin Core version 24.1 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-24.1/>

This release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tortoisecoin/tortoisecoin/issues>

To receive security and update notifications, please subscribe to:

  <https://tortoisecoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Tortoisecoin-Qt` (on macOS)
or `tortoisecoind`/`tortoisecoin-qt` (on Linux).

Upgrading directly from a version of Tortoisecoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Tortoisecoin Core are generally supported.

Compatibility
==============

Tortoisecoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Tortoisecoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Tortoisecoin Core on
unsupported systems.

### P2P

- #26878 I2P network optimizations
- #26909 net: prevent peers.dat corruptions by only serializing once
- #27608 p2p: Avoid prematurely clearing download state for other peers
- #27610 Improve performance of p2p inv to send queues

### RPC and other APIs

- #26515 rpc: Require NodeStateStats object in getpeerinfo
- #27279 doc: fix/improve warning helps in {create,load,unload,restore}wallet
- #27468 rest: avoid segfault for invalid URI

### Build System

- #26944 depends: fix systemtap download URL
- #27462 depends: fix compiling bdb with clang-16 on aarch64

### Wallet

- #26595 wallet: be able to specify a wallet name and passphrase to migratewallet
- #26675 wallet: For feebump, ignore abandoned descendant spends
- #26679 wallet: Skip rescanning if wallet is more recent than tip
- #26761 wallet: fully migrate address book entries for watchonly/solvable wallets
- #27053 wallet: reuse change dest when re-creating TX with avoidpartialspends
- #27080 wallet: Zero out wallet master key upon locking so it doesn't persist in memory
- #27473 wallet: Properly handle "unknown" Address Type

### GUI changes

- gui#687 Load PSBTs using istreambuf_iterator rather than istream_iterator
- gui#704 Correctly limit overview transaction list

### Miscellaneous

- #26880 ci: replace Intel macOS CI job
- #26924 refactor: Add missing includes to fix gcc-13 compile error

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrew Chow
- Anthony Towns
- Hennadii Stepanov
- John Moffett
- Jon Atack
- Marco Falke
- Martin Zumsande
- Matthew Zipkin
- Michael Ford
- pablomartin4btc
- Sebastian Falbesoner
- Suhas Daftuar
- Thomas Nguyen
- Vasil Dimov

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).