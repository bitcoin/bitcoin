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

### Build

- #33580 depends: Use `$(package)_file_name` when downloading from the fallback

### IPC

- #33517 multiprocess: Fix high overhead from message logging
- #33519 Update libmultiprocess subtree in 30.x branch
- #33566 miner: fix empty mempool case for waitNext()

### Test

- #33612 test: change log rate limit version gate

### Doc

- #33630 doc: correct topology requirements in submitpackage helptext

### Misc

- #33508 ci: fix buildx gha cache authentication on forks
- #33558 ci: Use native platform for win-cross task
- #33581 ci: Properly include $FILE_ENV in DEPENDS_HASH

Credits
=======

Thanks to everyone who directly contributed to this release:

- Ava Chow
- Cory Fields
- Eugene Siegel
- glozow
- MarcoFalke
- Ryan Ofsky
- Sjors Provoost
- willcl-ark

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
