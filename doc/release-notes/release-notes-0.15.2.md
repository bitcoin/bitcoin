Tortoisecoin Core version *0.15.2* is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-0.15.2/>

This is a new minor version release, including various bugfixes and
performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tortoisecoin/tortoisecoin/issues>

To receive security and update notifications, please subscribe to:

  <https://tortoisecoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the 
installer (on Windows) or just copy over `/Applications/Tortoisecoin-Qt` (on Mac)
or `tortoisecoind`/`tortoisecoin-qt` (on Linux).

The first time you run version 0.15.0 or higher, your chainstate database will
be converted to a new format, which will take anywhere from a few minutes to
half an hour, depending on the speed of your machine.

The file format of `fee_estimates.dat` changed in version 0.15.0. Hence, a
downgrade from version 0.15 or upgrade to version 0.15 will cause all fee
estimates to be discarded.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0. Upgrading
directly from 0.7.x and earlier without redownloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

The chainstate database for this release is not compatible with previous
releases, so if you run 0.15 and then decide to switch back to any
older version, you will need to run the old release with the `-reindex-chainstate`
option to rebuild the chainstate data structures in the old format.

If your node has pruning enabled, this will entail re-downloading and
processing the entire blockchain.

Compatibility
==============

Tortoisecoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

Tortoisecoin Core should also work on most other Unix-like systems but is not
frequently tested on them.


Notable changes
===============

Denial-of-Service vulnerability CVE-2018-17144
-------------------------------

A denial-of-service vulnerability exploitable by miners has been discovered in
Tortoisecoin Core versions 0.14.0 up to 0.16.2. It is recommended to upgrade any of
the vulnerable versions to 0.15.2 or 0.16.3 as soon as possible.

0.15.2 Change log
=================

### Build system

- #11995 `9bb1a16` depends: Fix Qt build with XCode 9.2(fanquake)
- #12946 `93b9a61` depends: Fix Qt build with XCode 9.3(fanquake)
- #13544 `9fd3e00` depends: Update Qt download url (fanquake)
- #11847 `cb7ef31` Make boost::multi_index comparators const (sdaftuar)

### Consensus
- #14247 `4b8a3f5` Fix crash bug with duplicate inputs within a transaction (TheBlueMatt, sdaftuar)
 
### RPC
- #11676 `7af2457` contrib/init: Update openrc-run filename (Luke Dashjr)
- #11277 `7026845` Fix uninitialized URI in batch RPC requests (Russell Yanofsky)
 
### Wallet
- #11289 `3f1db56` Wrap dumpwallet warning and note scripts aren't dumped (MeshCollider)
- #11289 `42ea47d` Add wallet backup text to import*, add* and dumpwallet RPCs (MeshCollider)
- #11590 `6372a75` [Wallet] always show help-line of wallet encryption calls (Jonas Schnelli)

### tortoisecoin-tx

- #11554 `a69cc07` Sanity-check script sizes in tortoisecoin-tx (TheBlueMatt)

### Tests
- #11277 `3a6cdd4` Add test for multiwallet batch RPC calls (Russell Yanofsky)
- #11647 `1c8c7f8` Add missing batch rpc calls to python coverage logs (Russell Yanofsky)
- #11277 `1036c43` Add missing multiwallet rpc calls to python coverage logs (Russell Yanofsky)
- #11277 `305f768` Limit AuthServiceProxyWrapper.\_\_getattr\_\_ wrapping (Russell Yanofsky)
- #11277 `2eea279` Make AuthServiceProxy.\_batch method usable (Russell Yanofsky)

Credits
=======

Thanks to everyone who directly contributed to this release:

- fanquake
- Jonas Schnelli
- Luke Dashjr
- Matt Corallo
- MeshCollider
- Russell Yanofsky
- Suhas Daftuar
- Wladimir J. van der Laan

And to those that reported security issues:

- awemany (for CVE-2018-17144, previously credited as "anonymous reporter")

