v30.x Release Notes
===================

Bitcoin Core version v30.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-30.x/>

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
or `bin/` and `libexec/` (on Linux).

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

### Net

- #36818 seeds: drop 21.ninja as a makeseeds source

### Build

- #35765 depends: keep local source archives in WORK_PATH
- #36889 cmake: Switch include_directories -> target_include_directories
- #36927 build: remove *dev* errors from dev-mode

### Doc

- #36957 doc: add 389 (multipath) and 394 (rawtr) to bips.md

### Misc

- #36869 refactor, memusage: Make fields of STL helper structs public

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xB10C
- fanquake
- hebasto
- jeanpablo
- Ryan Ofsky

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
