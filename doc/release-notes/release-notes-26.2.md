26.2 Release Notes
==================

Tortoisecoin Core version 26.2 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-26.2/>

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
using the Linux kernel, macOS 11.0+, and Windows 7 and newer.  Tortoisecoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Tortoisecoin Core on
unsupported systems.

Notable changes
===============

### Script

- #29853: sign: don't assume we are parsing a sane TapMiniscript

### P2P and network changes

- #29691: Change Luke Dashjr seed to dashjr-list-of-p2p-nodes.us
- #30085: p2p: detect addnode cjdns peers in GetAddedNodeInfo()

### RPC

- #29869: rpc, bugfix: Enforce maximum value for setmocktime
- #28554: bugfix: throw an error if an invalid parameter is passed to getnetworkhashps RPC
- #30094: rpc: move UniValue in blockToJSON
- #29870: rpc: Reword SighashFromStr error message

### Build

- #29747: depends: fix mingw-w64 Qt DEBUG=1 build
- #29985: depends: Fix build of Qt for 32-bit platforms with recent glibc
- #30151: depends: Fetch miniupnpc sources from an alternative website
- #30283: upnp: fix build with miniupnpc 2.2.8

### Misc

- #29776: ThreadSanitizer: Fix #29767
- #29856: ci: Bump s390x to ubuntu:24.04
- #29764: doc: Suggest installing dev packages for debian/ubuntu qt5 build
- #30149: contrib: Renew Windows code signing certificate

Credits
=======

Thanks to everyone who directly contributed to this release:

- Antoine Poinsot
- Ava Chow
- Cory Fields
- dergoegge
- fanquake
- glozow
- Hennadii Stepanov
- Jameson Lopp
- jonatack
- laanwj
- Luke Dashjr
- MarcoFalke
- nanlour
- willcl-ark

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).
