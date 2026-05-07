v31.x Release Notes
===================

Bitcoin Core version 31.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-31.x/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS) or
`bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated. Old wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on the following operating systems or
newer: Linux Kernel 3.17, macOS 14, and Windows 10 (version 1903). Bitcoin Core
should also work on most other Unix-like systems but is not as frequently tested
on them. It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

### Validation

- #35209 validation: correct lifetime of precomputed tx data

### P2P

- #35032 net_processing: don't modify addrman for private broadcast connections

### Wallet

- #35227 wallet: check the final BDB page LSN during migration

### Build

- #34228 depends: Unset SOURCE_DATE_EPOCH in gen_id script

### Test

- #34425 test: Fix all races after a socket is closed gracefully
- #34863 test: Clean shutdown in Socks5Server
- #34991 test: fix feature_index_prune.py bug when using --usecli
- #35080 test: Add missing self.options.timeout_factor scale in tool_bitcoin_chainstate.py
- #35218 test: fix P2SH script in coins cache fuzz target

### Fuzz

- #35289 fuzz: Fix timeout in txorphan

### Docs

- #35283 doc: mention -DWITH_ZMQ=ON in BSD build guides

### CI

- #35202 ci: restore sockets in i686, no IPC job
- #35230 ci: Move --usecli --extended from i386 task to alpine task

### Misc

- #35044 contrib: Fix NameError in signet miner gbt()
- #35175 multi_index: fix compilation failure with boost >= 1.91

Credits
=======

Thanks to everyone who directly contributed to this release:

- Cory Fields
- darosior
- Greg Sanders
- Hennadii Stepanov
- junbyjun1238
- Lőrinc
- MarcoFalke
- marcofleon
- optout21
- Pol Espinasa
- Torkel Rogstad
- Vasil Dimov

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
