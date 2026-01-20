Bitcoin Core version 28.2 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-28.2/>

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

Bitcoin Core is supported and extensively tested on operating systems
using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer. Bitcoin
Core should also work on most other UNIX-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Build

- #31407 guix: Notarize MacOS app bundle and codesign all MacOS and Windows binaries
- #31500 depends: Fix compiling libevent package on NetBSD
- #31627 depends: Fix spacing issue
- #32070 build: use make < 3.82 syntax for define directive
- #32439 guix: accomodate migration to codeberg
- #32568 depends: use "mkdir -p" when installing xproto
- #32693 depends: fix cmake compatibility error for freetype

### Test

- #32286 test: Handle empty string returned by CLI as None in RPC tests
- #32336 test: Suppress upstream -Wduplicate-decl-specifier in bpfcc

### Tracing

- #31623 tracing: Rename the MIN macro to TRACEPOINT_TEST_MIN in log_raw_p2p_msgs

### Doc

- #32003 doc: remove note about macOS self-signing

### Misc

- #31611 doc: upgrade license to 2025
- #32187 refactor: Remove spurious virtual from final ~CZMQNotificationInterface

Credits
=======

Thanks to everyone who directly contributed to this release:
- 0xB10C
- achow101
- Brandon Odiwuor
- fanquake
- Hennadii Stepanov
- josibake
- kehiy
- MarcoFalke
- Sjors Provoost

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
