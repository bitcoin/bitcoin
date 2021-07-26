0.19.1 Release Notes
===============================

Bitcoin Core version 0.19.1 is now available from:

  <https://bitcoinrupeecore.org/bin/bitcoinrupee-core-0.19.1/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoinrupee/bitcoinrupee/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoinrupeecore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoinrupeed`/`bitcoinrupee-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the datadir needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended
to use Bitcoin Core on unsupported systems.

Bitcoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From Bitcoin Core 0.17.0 onwards, macOS versions earlier than 10.10 are no
longer supported, as Bitcoin Core is now built using Qt 5.9.x which requires
macOS 10.10+. Additionally, Bitcoin Core does not yet change appearance when
macOS "dark mode" is activated.

In addition to previously supported CPU platforms, this release's pre-compiled
distribution provides binaries for the RISC-V platform.

0.19.1 change log
=================

### Wallet
- #17643 Fix origfee return for bumpfee with feerate arg (instagibbs)
- #16963 Fix `unique_ptr` usage in boost::signals2 (promag)
- #17258 Fix issue with conflicted mempool tx in listsinceblock (adamjonas, mchrostowski)
- #17924 Bug: IsUsedDestination shouldn't use key id as script id for ScriptHash (instagibbs)
- #17621 IsUsedDestination should count any known single-key address (instagibbs)
- #17843 Reset reused transactions cache (fjahr)

### RPC and other APIs
- #17687 cli: Fix fatal leveldb error when specifying -blockfilterindex=basic twice (brakmic)
- #17728 require second argument only for scantxoutset start action (achow101)
- #17445 zmq: Fix due to invalid argument and multiple notifiers (promag)
- #17524 psbt: handle unspendable psbts (achow101)
- #17156 psbt: check that various indexes and amounts are within bounds (achow101)

### GUI
- #17427 Fix missing qRegisterMetaType for `size_t` (hebasto)
- #17695 disable File-\>CreateWallet during startup (fanquake)
- #17634 Fix comparison function signature (hebasto)
- #18062 Fix unintialized WalletView::progressDialog (promag)

### Tests and QA
- #17416 Appveyor improvement - text file for vcpkg package list (sipsorcery)
- #17488 fix "bitcoinrupeed already running" warnings on macOS (fanquake)
- #17980 add missing #include to fix compiler errors (kallewoof)

### Platform support
- #17736 Update msvc build for Visual Studio 2019 v16.4 (sipsorcery)
- #17364 Updates to appveyor config for VS2019 and Qt5.9.8 + msvc project fixes (sipsorcery)
- #17887 bug-fix macos: give free bytes to `F_PREALLOCATE` (kallewoof)

### Miscellaneous
- #17897 init: Stop indexes on shutdown after ChainStateFlushed callback (jimpo)
- #17450 util: Add missing headers to util/fees.cpp (hebasto)
- #17654 Unbreak build with Boost 1.72.0 (jbeich)
- #17857 scripts: Fix symbol-check & security-check argument passing (fanquake)
- #17762 Log to net category for exceptions in ProcessMessages (laanwj)
- #18100 Update univalue subtree (MarcoFalke)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Adam Jonas
- Andrew Chow
- Fabian Jahr
- fanquake
- Gregory Sanders
- Harris
- Hennadii Stepanov
- Jan Beich
- Jim Posen
- João Barbosa
- Karl-Johan Alm
- Luke Dashjr
- MarcoFalke
- Michael Chrostowski
- Russell Yanofsky
- Wladimir J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoinrupee/bitcoinrupee/).
