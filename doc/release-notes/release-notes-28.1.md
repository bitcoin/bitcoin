Bitcoin Core version 28.1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-28.1>

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

- When the `-port` configuration option is used, the default onion listening port will now
  be derived to be that port + 1 instead of being set to a fixed value (8334 on mainnet).
  This re-allows setups with multiple local nodes using different `-port` and not using `-bind`,
  which would lead to a startup failure in v28.0 due to a port collision.

  Note that a `HiddenServicePort` manually configured in `torrc` may need adjustment if used in
  connection with the `-port` option.
  For example, if you are using `-port=5555` with a non-standard value and not using `-bind=...=onion`,
  previously Bitcoin Core would listen for incoming Tor connections on `127.0.0.1:8334`.
  Now it would listen on `127.0.0.1:5556` (`-port` plus one). If you configured the hidden service manually
  in torrc now you have to change it from `HiddenServicePort 8333 127.0.0.1:8334` to `HiddenServicePort 8333
  127.0.0.1:5556`, or configure bitcoind with `-bind=127.0.0.1:8334=onion` to get the previous behavior.
  (#31223)
- #30568 addrman: change internal id counting to int64_t

### Key

- #31166 key: clear out secret data in DecodeExtKey

### Build

- #31013 depends: For mingw cross compile use `-gcc-posix` to prevent library conflict
- #31502 depends: Fix CXXFLAGS on NetBSD

### Test

- #31016 test: add missing sync to feature_fee_estimation.py
- #31448 fuzz: add cstdlib to FuzzedDataProvider
- #31419 test: fix MIN macro redefinition
- #31563 rpc: Extend scope of validation mutex in generateblock

### Doc

- #31007 doc: add testnet4 section header for config file

### CI

- #30961 ci: add LLVM_SYMBOLIZER_PATH to Valgrind fuzz job

### Misc

- #31267 refactor: Drop deprecated space in `operator""_mst`
- #31431 util: use explicit cast in MultiIntBitSet::Fill()

Credits
=======

Thanks to everyone who directly contributed to this release:

- fanquake
- Hennadii Stepanov
- laanwj
- MarcoFalke
- Martin Zumsande
- Marnix
- Sebastian Falbesoner

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
