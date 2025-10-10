Bitcoin Knots version 29.2.knots20251010 is now available from:

  <https://bitcoinknots.org/files/29.x/29.2.knots20251010/>

This release includes various bug fixes and a new Dockerfile.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoinknots/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoinknots.org/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from very old versions of Bitcoin Core or Knots is possible,
but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Knots are generally supported.

Compatibility
==============

Bitcoin Knots is supported on operating systems using the Linux kernel, macOS
13+, and Windows 10+. It is not recommended to use Bitcoin Knots on
unsupported systems.

Known Bugs
==========

In various locations, including the GUI's transaction details dialog and the
`"vsize"` result in many RPC results, transaction virtual sizes may not account
for an unusually high number of sigops (ie, as determined by the
`-bytespersigop` policy) or datacarrier penalties (ie, `-datacarriercost`).
This could result in reporting a lower virtual size than is actually used for
mempool or mining purposes.

Due to disruption of the shared Bitcoin Transifex repository, this release
still does not include updated translations, and Bitcoin Knots may be unable
to do so until/unless that is resolved.

Notable changes
===============

A new Dockerfile has been added to the source code release, under the
`contrib/docker` directory. Please read [the documentation](https://github.com/bitcoinknots/bitcoin/blob/29.x-knots/contrib/docker/README.md) for details.

### Consensus

- #33334 node: optimize CBlockIndexWorkComparator

### P2P and network changes

- #7219 Discontinue advertising NODE_REPLACE_BY_FEE service bit
- #15421 Bugfix: torcontrol: Use ephemeral config file rather than stdin
- #32646 p2p: Add witness mutation check inside FillBlock
- #33296 net: check for empty header before calling FillBlock
- #33311 net: Quiet down logging when router doesn't support natpmp/pcp
- #33338 net: Add interrupt to pcp retry loop
- #33395 net: do not apply whitelist permissions to onion inbounds
- #33464 p2p: Use network-dependent timers for inbound inv scheduling
- knots#187 add Léo Haf DNS seed
- Bugfix: torcontrol: Map bind-any to loopback address
- Bugfix: net: Treat connections to the first normal bind as Tor when appropriate

### GUI

- gui#886 Avoid pathological QT text/markdown behavior...
- knots#203 add migratewallet rpc in historyFilter
- icon: Render macOS icns as a macOS-style icon

### Wallet

- knots#205 Bugfix: Wallet: Migration: Adapt sanity checks for walletimplicitsegwit=0

### RPC

- #31785 Have createNewBlock() wait for tip, make rpc handle shutdown during long poll and wait methods
- #33446 rpc: fix getblock(header) returns target for tip
- #33475 bugfix: miner: fix addPackageTxs unsigned integer overflow
- #33484 doc: rpc: fix case typo in finalizepsbt help (final_scriptwitness)
- knots#190 Add zsh completion script generation support
- Interpret ignore_rejects=truc to ignore all TRUC policies

### Block and transaction handling

- #31144 \[IBD\] multi-byte block obfuscation
- #31845 Bugfix: Correctly handle pruneduringinit=0 by treating it as manual-prune until sync completes

### Index

- #33410 coinstats: avoid unnecessary Coin copy in ApplyHash

### Test

- #33433 Bugfix: QA: rpc_bind: Skip nonloopback test if no such address is found

### Mempool

- #33504 mempool: Do not enforce TRUC checks on reorg

### RPC

- #33446 rpc: fix getblock(header) returns target for tip

### CI

- #32989 ci: Migrate CI to GitHub Actions
- #32999 ci: Use APT_LLVM_V in msan task
- #33099 ci: allow for any libc++ intrumentation & use it for TSAN
- #33258 ci: use LLVM 21
- #33303 ci: Checkout latest merged pulls
- #33319 ci: reduce runner sizes on various jobs
- #33364 ci: always use tag for LLVM checkout
- #33425 ci: remove Clang build from msan fuzz job

### Doc

- #33484 doc: rpc: fix case typo in `finalizepsbt` help

### Misc

- #33310 trace: Workaround GCC bug compiling with old systemtap
- #33332 common: Make arith_uint256 trivially copyable
- #33340 Fix benchmark CSV output
- #33422 build: Remove lingering Windows registry & shortcuts
- #33482 contrib: fix macOS deployment with no translations
- #33494 depends: Update URL for qrencode package source tarball
- #33504 Mempool: Do not enforce TRUC checks on reorg
- #33511 init: Fix Ctrl-C shutdown hangs during wait calls
- #33580 depends: Use $(package)_file_name when downloading from the fallback
- knots#171 Add Dockerfile
- knots#192 depends: fetch miniupnpc sources from github releases
- guix: Rename win64*-unsigned to win64*-pgpverifiable

Credits
=======

Thanks to everyone who directly contributed to this release:

- /dev/fd0
- Amisha Chhajed
- Ava Chow
- Claudio Raimondi
- David Gumberg
- Eugene Siegel
- Fabian Jahr
- fanquake
- glozow
- Greg Sanders
- Hennadii Stepanov
- Hodlinator
- ismaelsadeeq
- laanwj
- Léo Haf
- Lőrinc
- Luke Dashjr
- Marcel Stampfer
- MarcoFalke
- Martin Zumsande
- Max Edwards
- Ryan Ofsky
- sashass1315
- Sebastian Falbesoner
- Sjors Provoost
- TheCharlatan
- Trevor Arjeski
- Vasil Dimov
- Will Clark
