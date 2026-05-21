Bitcoin Core version 28.3 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-28.3/>

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
Core should also work on most other UNIX-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Mempool & Policy

 The minimum block feerate (`-blockmintxfee`) has been changed to 1 satoshi per kvB. It can still be changed using the
configuration option.

- The default minimum relay feerate (`-minrelaytxfee`) and incremental relay feerate (`-incrementalrelayfee`) have been
changed to 100 satoshis per kvB. They can still be changed using their respective configuration options, but it is
recommended to change both together if you decide to do so.
  - Other minimum feerates (e.g. the dust feerate, the minimum returned by the fee estimator, and all feerates used by
  the wallet) remain unchanged. The mempool minimum feerate still changes in response to high volume.
  - Note that unless these lower defaults are widely adopted across the network, transactions created with lower fee
  rates are not guaranteed to propagate or confirm. The wallet feerates remain unchanged; `-mintxfee` must be changed
  before attempting to create transactions with lower feerates using the wallet.

- #33106 policy: lower the default blockmintxfee, incrementalrelayfee, minrelaytxfee
- #33504 mempool: Do not enforce TRUC checks on reorg

### P2P

- #33395 net: do not apply whitelist permissions to onion inbounds

### Test

- #32765 test: Fix list index out of range error in feature_bip68_sequence.py
- #33001 test: Do not pass tests on unhandled exceptions
- #30125 test: improve BDB parser (handle internal/overflow pages, support all page sizes)
- #30948 test: Add missing sync_mempools() to fill_mempool()
- #30784 test: add BulkTransaction helper to unit test transaction utils

### Build

- #32678 guix: warn and abort when SOURCE_DATE_EPOCH is set
- #32943 depends: Force CMAKE_EXPORT_NO_PACKAGE_REGISTRY=TRUE
- #33073 guix: warn SOURCE_DATE_EPOCH set in guix-codesign
- #33563 build: fix depends Qt download link

### Doc

- #32776 doc: taproot became always active in v24.0
- #32777 doc: fix Transifex 404s
- #33070 doc/zmq: fix unix socket path example
- #33133 rpc: fix getpeerinfo ping duration unit docs
- #33236 doc: Remove wrong and redundant doxygen tag

### Misc

- #33340 Fix benchmark CSV output
- #33482 contrib: fix macOS deployment with no translations
- #33581 ci: Properly include $FILE_ENV in DEPENDS_HASH

Credits
=======

Thanks to everyone who directly contributed to this release:
- 0xB10C
- amisha
- Ava Chow
- fanquake
- glozow
- Hennadii Stepanov
- MarcoFalke
- Martin Zumsande
- romanz
- Sjors Provoost
- theStack
- Vasil Dimov
- willcl-ark
- zaidmstrr

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
