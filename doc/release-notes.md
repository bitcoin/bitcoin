0.21.1 Release Notes
====================

Bitcoin Core version 0.21.1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.21.1/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance
when macOS "dark mode" is activated.

Notable changes
===============

RPC
---


0.21.1 change log
=================

### Consensus
- #21377 Speedy trial support for versionbits (ajtowns)
- #21686 Speedy trial activation parameters for Taproot (achow101)

### P2P protocol and network code
- #20852 allow CSubNet of non-IP networks (vasild)
- #21043 Avoid UBSan warning in ProcessMessage(…) (practicalswift)

### Wallet
- #21166 Introduce DeferredSignatureChecker and have SignatureExtractorClass subclass it (achow101)
- #21083 Avoid requesting fee rates multiple times during coin selection (achow101)

### RPC and other APIs
- #21201 Disallow sendtoaddress and sendmany when private keys disabled (achow101)

### Build system
- #21486 link against -lsocket if required for `*ifaddrs` (fanquake)
- #20983 Fix MSVC build after gui#176 (hebasto)

### Tests and QA
- #21380 Add fuzzing harness for versionbits (ajtowns)
- #20812 fuzz: Bump FuzzedDataProvider.h (MarcoFalke)
- #20740 fuzz: Update FuzzedDataProvider.h from upstream (LLVM) (practicalswift)
- #21446 Update vcpkg checkout commit (sipsorcery)
- #21397 fuzz: Bump FuzzedDataProvider.h (MarcoFalke)
- #21081 Fix the unreachable code at `feature_taproot` (brunoerg)
- #20562 Test that a fully signed tx given to signrawtx is unchanged (achow101)
- #21571 Make sure non-IP peers get discouraged and disconnected (vasild, MarcoFalke)
- #21489 fuzz: cleanups for versionbits fuzzer (ajtowns)

### Miscellaneous
- #20861 BIP 350: Implement Bech32m and use it for v1+ segwit addresses (sipa)

### Documentation
- #21384 add signet to bitcoin.conf documentation (jonatack)
- #21342 Remove outdated comment (hebasto)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Andrew Chow
- Anthony Towns
- Bruno Garcia
- Fabian Jahr
- fanquake
- Hennadii Stepanov
- Jon Atack
- Luke Dashjr
- MarcoFalke
- Pieter Wuille
- practicalswift
- randymcmillan
- Sjors Provoost
- Vasil Dimov
- W. J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
