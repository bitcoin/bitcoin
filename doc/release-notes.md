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

### Net

- #34093 netif: fix compilation warning in QueryDefaultGatewayImpl()
- #34549 net: reduce log level for PCP/NAT-PMP NOT_AUTHORIZED failures

### PSBT

- #34272 psbt: Fix PSBTInputSignedAndVerified bounds assert
- #34219 psbt: validate pubkeys in MuSig2 pubnonce/partial sig deserialization

### Miniscript

- #34434 miniscript: correct and_v() properties

### Build

- #34281 build: Temporarily remove confusing and brittle -fdebug-prefix-map
- #34554 build: avoid exporting secp256k1 symbols
- #34627 guix: use a temporary file over sponge, drop moreutils
- #34713 depends: Allow building Qt packages after interruption
- #34754 depends: Qt fixes for GCC 16 compatibility

### Test

- #34185 test: fix feature_pruning when built without wallet
- #34282 qa: Fix Windows logging bug
- #34390 test: allow overriding tar in get_previous_releases.py
- #34409 test: use ModuleNotFoundError in interface_ipc.py
- #34445 fuzz: Use AFL_SHM_ID for naming test directories
- #34608 test: Fix broken --valgrind handling after bitcoin wrapper
- #34690 test: Add missing timeout_factor to zmq socket

### Util

- #34597 util: Fix UB in SetStdinEcho when ENOTTY

### Doc

- #34252 doc: add 433 (Pay to Anchor) to bips.md
- #34413 doc: Remove outdated -fdebug-prefix-map section in dev notes
- #34510 doc: fix broken bpftrace installation link
- #34561 wallet: rpc: manpage: fix example missing `fee_rate` argument
- #34671 doc: Update Guix install for Debian/Ubuntu
- #34702 doc: Fix fee field in getblock RPC result
- #34706 doc: Improve dependencies.md IPC documentation
- #34789 doc: update build guides pre v31

### CI

- #32513 ci: remove 3rd party js from windows dll gha job
- #34344 ci: update GitHub Actions versions
- #34453 ci: Always print low ccache hit rate notice
- #34461 ci: Print verbose build error message in test-each-commit

Credits
=======

Thanks to everyone who directly contributed to this release:

- ANAVHEOBA
- brunoerg
- darosior
- fanquake
- Hennadii Stepanov
- jayvaliya
- Lőrinc
- m3dwards
- marcofleon
- MarcoFalke
- mzumsande
- nervana21
- Padraic Slattery
- ryanofsky
- Sebastian Falbesoner
- SomberNight
- tboy1337
- theuni
- ToRyVand
- willcl-ark

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
