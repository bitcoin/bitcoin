23.1 Release Notes
==================

Tortoisecoin Core version 23.1 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-23.1/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tortoisecoin/tortoisecoin/issues>

To receive security and update notifications, please subscribe to:

  <https://tortoisecoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Tortoisecoin-Qt` (on macOS)
or `tortoisecoind`/`tortoisecoin-qt` (on Linux).

Upgrading directly from a version of Tortoisecoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Tortoisecoin Core are generally supported.

Compatibility
==============

Tortoisecoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Tortoisecoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Tortoisecoin Core on
unsupported systems.

### P2P

- #25314 p2p: always set nTime for self-advertisements

### RPC and other APIs

- #25220 rpc: fix incorrect warning for address type p2sh-segwit in createmultisig
- #25237 rpc: Capture UniValue by ref for rpcdoccheck
- #25983 Prevent data race for pathHandlers
- #26275 Fix crash on deriveaddresses when index is 2147483647 (2^31-1)

### Build system

- #25201 windeploy: Renewed windows code signing certificate
- #25788 guix: patch NSIS to remove .reloc sections from installer stubs
- #25861 guix: use --build={arch}-guix-linux-gnu in cross toolchain
- #25985 Revert "build: Use Homebrew's sqlite package if it is available"

### GUI

- #24668 build, qt: bump Qt5 version to 5.15.3
- gui#631 Disallow encryption of watchonly wallets
- gui#680 Fixes MacOS 13 segfault by preventing certain notifications

### Tests

- #24454 tests: Fix calculation of external input weights

### Miscellaneous

- #26321 Adjust .tx/config for new Transifex CLI

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrew Chow
- brunoerg
- Hennadii Stepanov
- John Moffett
- MacroFake
- Martin Zumsande
- Michael Ford
- muxator
- Pavol Rusnak
- Sebastian Falbesoner
- W. J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).
