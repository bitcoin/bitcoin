Bitcoin Core version 28.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-28.x/test.rc1/>

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

### Test

- #32765 test: Fix list index out of range error in feature_bip68_sequence.py

### Build

- #32678 guix: warn and abort when SOURCE_DATE_EPOCH is set

### Doc

- #32776 doc: taproot became always active in v24.0
- #32777 doc: fix Transifex 404s


Credits
=======

Thanks to everyone who directly contributed to this release:
- fanquake
- Sjors Provoost
- willcl-ark
- zaidmstrr

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
