v31.x Release Notes
===================

Bitcoin Core version 31.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-31.x/>

This release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS) or
`bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated. Old wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on the following operating systems or
newer: Linux Kernel 3.17, macOS 14, and Windows 10 (version 1903). Bitcoin Core
should also work on most other Unix-like systems but is not as frequently tested
on them. It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

- The private-broadcast queue (transactions submitted via `sendrawtransaction`
when `-privatebroadcast` is enabled and not yet echoed back from the network)
is now capped at 10,000 entries. When full, new submissions are rejected. It is
up to the caller to inspect the queue via `getprivatebroadcastinfo` and free
up space when stuck via `abortprivatebroadcast`. (#35406)

### P2P

- #34873 net: fix premature stale flagging of unpicked private broadcast txs
- #35406 private broadcast: limit outstanding txs to count of 10,000
- #35678 private broadcast: define and use new RPC_LIMIT_EXCEEDED error code
- #35691 chainparams: delete my DNS seed
- #35766 p2p: Assume v2transport for addresses from seeds

### Fuzz

- #35679 fuzz: Remove unused DeserializeFromFuzzingInput params overload

### Build

- #35769 depends, zeromq: Apply upstream patch

Credits
=======

Thanks to everyone who directly contributed to this release:

- Greg Sanders
- hebasto
- Martin Zumsande
- Mccalabrese
- sipa
- stickies-v

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
