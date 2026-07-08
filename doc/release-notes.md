Bitcoin Core version 29.4 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-29.4/>

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

Bitcoin Core is supported and tested on operating systems using the
Linux Kernel 3.17+, macOS 13+, and Windows 10+. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

This release fixes an issue where the chainstate database would repeatedly
rewrite large portions of itself, causing excessive disk reads and writes
during normal operation.

### Validation

- #35209 validation: correct lifetime of precomputed tx data
- #35465 coins: compact chainstate regularly

### Leveldb

- #61(bitcoin-core/leveldb): Disable seek compaction

### Net

- #34093 netif: fix compilation warning in QueryDefaultGatewayImpl()

### Wallet

- #35228 wallet: use outpoint when estimating input size

### Build

- #34228 depends: Unset SOURCE_DATE_EPOCH in gen_id script
- #34848 cmake: Migrate away from deprecated SQLite3 target

### Test

- #34918 fuzz: [refactor] Remove unused g_setup pointers

### Doc

- #34510 doc: fix broken bpftrace installation link
- #34561 wallet: rpc: manpage: fix example missing `fee_rate` argument
- #34671 doc: Update Guix install for Debian/Ubuntu
- #35283 doc: mention -DWITH_ZMQ=ON in BSD build guides

### CI

- #35202 ci: restore sockets in i686, no IPC job
- #35378 ci: switch runners from cirrus to warpbuild
- #35408 ci: 35378 followups

### Misc

- #35175 multi_index: fix compilation failure with boost >= 1.91

Credits
=======

Thanks to everyone who directly contributed to this release:

- andrewtoth
- Cory Fields
- Daniel Pfeifer
- darosior
- fanquake
- Hennadii Stepanov
- jayvaliya
- junbyjun1238
- Lőrinc
- MarcoFalke
- SomberNight
- ToRyVand
- willcl-ark

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
