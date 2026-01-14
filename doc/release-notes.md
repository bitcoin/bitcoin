v30.x Release Notes
===================

Bitcoin Core version v30.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-30.x/>

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

- #34358 wallet: fix removeprunedfunds bug with conflicting transactions

### PSBT

- #34272 psbt: Fix PSBTInputSignedAndVerified bounds assert

### Build

- #34281 build: Temporarily remove confusing and brittle -fdebug-prefix-map

### Test

- #34185 test: fix feature_pruning when built without wallet
- #34282 qa: Fix Windows logging bug
- #34390 test: allow overriding tar in get_previous_releases.py

### Doc

- #34252 doc: add 433 (Pay to Anchor) to bips.md
- #34413 doc: Remove outdated -fdebug-prefix-map section in dev notes

### CI

- #32513 ci: remove 3rd party js from windows dll gha job
- #34344 ci: update GitHub Actions versions

Credits
=======

Thanks to everyone who directly contributed to this release:

- brunoerg
- fanquake
- Hennadii Stepanov
- Lőrinc
- m3dwards
- MarcoFalke
- mzumsande
- Padraic Slattery
- Sebastian Falbesoner

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
