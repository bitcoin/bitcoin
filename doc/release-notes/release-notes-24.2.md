24.2 Release Notes
==================

Tortoisecoin Core version 24.2 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-24.2/>

This release includes various bug fixes and performance
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

### Fees

- #27622 Fee estimation: avoid serving stale fee estimate

### RPC and other APIs

- #27727 rpc: Fix invalid bech32 address handling

### Build System

- #28097 depends: xcb-proto 1.15.2
- #28543 build, macos: Fix qt package build with new Xcode 15 linker
- #28571 depends: fix unusable memory_resource in macos qt build

### CI

- #27777 ci: Prune dangling images on RESTART_CI_DOCKER_BEFORE_RUN
- #27834 ci: Nuke Android APK task, Use credits for tsan
- #27844 ci: Use podman stop over podman kill
- #27886 ci: Switch to amd64 container in "ARM" task

### Miscellaneous
- #28452 Do not use std::vector = {} to release memory

Credits
=======

Thanks to everyone who directly contributed to this release:

- Abubakar Sadiq Ismail
- Hennadii Stepanov
- Marco Falke
- Michael Ford
- Pieter Wuille

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).
