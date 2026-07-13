v30.x Release Notes
===================

Bitcoin Core version v30.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-30.x/>

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

- BIP 9 bits 5 to 28 inclusive are now ignored for soft fork signaling, as per
  BIP 323. We won't warn about unknown deployments when receiving blocks that
  set any of those bits in their version. (#34779)

### Versionbits

- #34779 BIP 323: reserve version bits 5-28 as extra nonce space

### P2P

- #35691 chainparams: delete my DNS seed
- #35766 p2p: Assume v2transport for addresses from seeds

### Fuzz

- #35679 fuzz: Remove unused DeserializeFromFuzzingInput params overload

Credits
=======

Thanks to everyone who directly contributed to this release:

- ajtowns
- darosior
- Hennadii Stepanov
- Martin Zumsande
- sipa

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
