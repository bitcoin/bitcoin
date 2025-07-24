Bitcoin Core version 29.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-29.x/>

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

Bitcoin Core is supported and tested on operating systems using the
Linux Kernel 3.17+, macOS 13+, and Windows 10+. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Mempool Policy

- The maximum number of potentially executed legacy signature operations in a
  single standard transaction is now limited to 2500. Signature operations in all
  previous output scripts, in all input scripts, as well as all P2SH redeem
  scripts (if there are any) are counted toward the limit. The new limit is
  assumed to not affect any known typically formed standard transactions. The
  change was done to prepare for a possible BIP54 deployment in the future.

- #32521 policy: make pathological transactions packed with legacy sigops non-standard

### Updated Settings

- The `-maxmempool` and `-dbcache` startup parameters are now capped on
  32-bit systems to 500MB and 1GiB respectively.

- #32530 node: cap -maxmempool and -dbcache values for 32-bit

### Wallet

- #31757 wallet: fix crash on double block disconnection
- #32553 wallet: Fix logging of wallet version

### P2P

- #32826 p2p: add more bad ports

### Test

- #32069 test: fix intermittent failure in wallet_reorgsrestore.py
- #32286 test: Handle empty string returned by CLI as None in RPC tests
- #32312 test: Fix feature_pruning test after nTime typo fix
- #32336 test: Suppress upstream -Wduplicate-decl-specifier in bpfcc
- #32463 test: fix an incorrect feature_fee_estimation.py subtest
- #32483 test: fix two intermittent failures in wallet_basic.py
- #32630 test: fix sync function in rpc_psbt.py
- #32765 test: Fix list index out of range error in feature_bip68_sequence.py
- #32742 test: fix catchup loop in outbound eviction functional test
- #32823 test: Fix wait_for_getheaders() call in test_outbound_eviction_blocks_relay_only()
- #32833 test: Add msgtype to msg_generic slots
- #32841 feature_taproot: sample tx version border values more
- #32850 test: check P2SH sigop count for coinbase tx
- #32859 test: correctly detect nonstd TRUC tx vsize in feature_taproot
- #33001 test: Do not pass tests on unhandled exceptions

### Util

- #32248 Remove support for RNDR/RNDRRS for aarch64

### Build

- #32356 cmake: Respect user-provided configuration-specific flags
- #32437 crypto: disable ASan for sha256_sse4 with Clang
- #32469 cmake: Allow WITH_DBUS on all Unix-like systems
- #32439 guix: accomodate migration to codeberg
- #32551 cmake: Add missed SSE41_CXXFLAGS
- #32568 depends: use "mkdir -p" when installing xproto
- #32678 guix: warn and abort when SOURCE_DATE_EPOCH is set
- #32690 depends: fix SHA256SUM command on OpenBSD (use GNU mode output)
- #32716 depends: Override host compilers for FreeBSD and OpenBSD
- #32760 depends: capnp 1.2.0
- #32798 build: add root dir to CMAKE_PREFIX_PATH in toolchain
- #32805 cmake: Use HINTS instead of PATHS in find_* commands
- #32814 cmake: Explicitly specify Boost_ROOT for Homebrew's package
- #32837 depends: fix libevent _WIN32_WINNT usage
- #32943 depends: Force CMAKE_EXPORT_NO_PACKAGE_REGISTRY=TRUE
- #32954 cmake: Drop no longer necessary "cmakeMinimumRequired" object

### Gui

- #864 Crash fix, disconnect numBlocksChanged() signal during shutdown
- #868 Replace stray tfm::format to cerr with qWarning

### Doc

- #32333 doc: Add missing top-level description to pruneblockchain RPC
- #32353 doc: Fix fuzz test_runner.py path
- #32389 doc: Fix test_bitcoin path
- #32607 rpc: Note in fundrawtransaction doc, fee rate is for package
- #32679 doc: update tor docs to use bitcoind binary from path
- #32693 depends: fix cmake compatibility error for freetype
- #32696 doc: make -DWITH_ZMQ=ON explicit on build-unix.md
- #32708 rpc, doc: update listdescriptors RCP help
- #32711 doc: add missing packages for BSDs (cmake, gmake, curl) to depends/README.md
- #32719 doc, windows: CompanyName "Bitcoin" => "Bitcoin Core project"
- #32776 doc: taproot became always active in v24.0
- #32777 doc: fix Transifex 404s
- #32846 doc: clarify that the "-j N" goes after the "--build build" part
- #32858 doc: Add workaround for vcpkg issue with paths with embedded spaces

### CI

- #32184 ci: Add workaround for vcpkg's libevent package

### Misc

- #32187 refactor: Remove spurious virtual from final ~CZMQNotificationInterface
- #32454 tracing: fix invalid argument in mempool_monitor
- #32771 contrib: tracing: Fix read of pmsg_type in p2p_monitor.py

Credits
=======

Thanks to everyone who directly contributed to this release:

- achow101
- Antoine Poinsot
- benthecarman
- bigspider
- Brandon Odiwuor
- brunoerg
- davidgumberg
- dergoegge
- enirox001
- fanquake
- furszy
- instagibbs
- Hennadii Stepanov
- hodlinator
- ismaelsadeeq
- jb55
- jlopp
- josibake
- laanwj
- luisschwab
- MarcoFalke
- Martin Zumsande
- monlovesmango
- nervana21
- pablomartin4btc
- rkrux
- ryanofsky
- Sjors
- theStack
- willcl-ark
- zaidmstrr

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
