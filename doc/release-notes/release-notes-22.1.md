22.1 Release Notes
==================

Tortoisecoin Core version 22.1 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-22.1/>

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
using the Linux kernel, macOS 10.14+, and Windows 7 and newer.  Tortoisecoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Tortoisecoin Core on
unsupported systems.

From Tortoisecoin Core 22.0 onwards, macOS versions earlier than 10.14 are no longer supported.

Notable changes
===============

Updated settings
----------------

- In previous releases, the meaning of the command line option
  `-persistmempool` (without a value provided) incorrectly disabled mempool
  persistence.  `-persistmempool` is now treated like other boolean options to
  mean `-persistmempool=1`. Passing `-persistmempool=0`, `-persistmempool=1`
  and `-nopersistmempool` is unaffected. (#23061)

### P2P

### RPC and other APIs

- #25237 rpc: Capture UniValue by ref for rpcdoccheck
- #25983 Prevent data race for pathHandlers
- #26275 Fix crash on deriveaddresses when index is 2147483647 (2^31-1)

### Wallet

- #22781 wallet: fix the behavior of IsHDEnabled
- #22949 fee: Round up fee calculation to avoid a lower than expected feerate
- #23333 wallet: fix segfault by avoiding invalid default-ctored external_spk_managers entry

### Build system

- #22820 build, qt: Fix typo in QtInputSupport check
- #23045 build: Restrict check for CRC32C intrinsic to aarch64
- #23148 build: Fix guix linker-loader path and add check_ELF_interpreter
- #23314 build: explicitly disable libsecp256k1 openssl based tests
- #23580 build: patch qt to explicitly define previously implicit header include
- #24215 guix: ignore additional failing certvalidator test
- #24256 build: Bump depends packages (zmq, libXau)
- #25201 windeploy: Renewed windows code signing certificate
- #25985 Revert "build: Use Homebrew's sqlite package if it is available"
- #26633 depends: update qt 5.12 url to archive location

### GUI

- #gui631 Disallow encryption of watchonly wallets
- #gui680 Fixes MacOS 13 segfault by preventing certain notifications
- #24498 qt: Avoid crash on startup if int specified in settings.json

### Tests

- #23716 test: replace hashlib.ripemd160 with an own implementation
- #24239 test: fix ceildiv division by using integers

### Utilities

- #22390 system: skip trying to set the locale on NetBSD
- #22895 don't call GetBlockPos in ReadBlockFromDisk without cs_main lock
- #24104 fs: Make compatible with boost 1.78

### Miscellaneous

- #23335 refactor: include a missing <limits> header in fs.cpp
- #23504 ci: Replace soon EOL hirsute with jammy
- #26321 Adjust .tx/config for new Transifex CLI

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrew Chow
- BlackcoinDev
- Carl Dong
- Hennadii Stepanov
- Joan Karadimov
- John Moffett
- Jon Atack
- Kittywhiskers Van Gogh
- Marco Falke
- Martin Zumsande
- Michael Ford
- muxator
- Pieter Wuille
- Ryan Ofsky
- Saibato
- Sebastian Falbesoner
- W. J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).
