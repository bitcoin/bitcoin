v30.4 Release Notes
===================

Bitcoin Core version v30.4 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-30.4/>

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
or `bin/` and `libexec/` (on Linux).

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

- BIP 9 bits 5 to 28 inclusive are now ignored for soft fork signaling, as per
  BIP 323. We won't warn about unknown deployments when receiving blocks that
  set any of those bits in their version. (#34779)

### Versionbits

- #34779 BIP 323: reserve version bits 5-28 as extra nonce space

### P2P

- #35691 chainparams: delete my DNS seed
- #35766 p2p: Assume v2transport for addresses from seeds

### Net

- #36665 sock: align exception monitoring between poll() and select() backends
- #36667 net: enforce timeout interval for paused send peers

### Util

- #36669 refactor: store positions as a 64 bits integer in flatfiles

### Build

- #36218 build: avoid pipe2 on Darwin (for now)

### Fuzz

- #35679 fuzz: Remove unused DeserializeFromFuzzingInput params overload

### Doc

- #36377 doc: add 461 (Deterministic ECDSA signatures with low-R grinding) to bips.md
- #36441 doc: fix How to Upgrade section for bitcoin wrapper and libexec

Credits
=======

Thanks to everyone who directly contributed to this release:

- ajtowns
- darosior
- fanquake
- Hennadii Stepanov
- Martin Zumsande
- Sebastian Falbesoner
- sipa
- spidey279

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
