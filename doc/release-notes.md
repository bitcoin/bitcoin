Bitcoin Core version 28.1rc1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-28.1/test.rc1>

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

Running Bitcoin Core binaries on macOS requires self signing.
```
cd /path/to/bitcoin-28.x/bin
xattr -d com.apple.quarantine bitcoin-cli bitcoin-qt bitcoin-tx bitcoin-util bitcoin-wallet bitcoind test_bitcoin
codesign -s - bitcoin-cli bitcoin-qt bitcoin-tx bitcoin-util bitcoin-wallet bitcoind test_bitcoin
```

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer. Bitcoin
Core should also work on most other UNIX-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### P2P

- #30568 addrman: change internal id counting to int64_t

### Key

- #31166 key: clear out secret data in DecodeExtKey

### Build

- #31013 depends: For mingw cross compile use `-gcc-posix` to prevent library conflict

### Test

- #31016 test: add missing sync to feature_fee_estimation.py

### Doc

- #31007 doc: add testnet4 section header for config file

### CI

- #30961 ci: add LLVM_SYMBOLIZER_PATH to Valgrind fuzz job

### Misc

- #31267 refactor: Drop deprecated space in `operator""_mst`

Credits
=======

- fanquake
- laanwj
- MarcoFalke
- Martin Zumsande
- Marnix
- Sebastian Falbesoner

Thanks to everyone who directly contributed to this release:

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
