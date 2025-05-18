0.20.2 Release Notes
====================

Tortoisecoin Core version 0.20.2 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-0.20.2/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tortoisecoin/tortoisecoin/issues>

To receive security and update notifications, please subscribe to:

  <https://tortoisecoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Tortoisecoin-Qt` (on Mac)
or `tortoisecoind`/`tortoisecoin-qt` (on Linux).

Upgrading directly from a version of Tortoisecoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Tortoisecoin Core are generally supported.

Compatibility
==============

Tortoisecoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Tortoisecoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Tortoisecoin Core on
unsupported systems.

From Tortoisecoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Tortoisecoin Core does not yet change appearance
when macOS "dark mode" is activated.

Known Bugs
==========

The process for generating the source code release ("tarball") has changed in an
effort to make it more complete, however, there are a few regressions in
this release:

- The generated `configure` script is currently missing, and you will need to
  install autotools and run `./autogen.sh` before you can run
  `./configure`. This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run
  `TORTOISECOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
===============

Changes regarding misbehaving peers
-----------------------------------

Peers that misbehave (e.g. send us invalid blocks) are now referred to as
discouraged nodes in log output, as they're not (and weren't) strictly banned:
incoming connections are still allowed from them, but they're preferred for
eviction.

Furthermore, a few additional changes are introduced to how discouraged
addresses are treated:

- Discouraging an address does not time out automatically after 24 hours
  (or the `-bantime` setting). Depending on traffic from other peers,
  discouragement may time out at an indeterminate time.

- Discouragement is not persisted over restarts.

- There is no method to list discouraged addresses. They are not returned by
  the `listbanned` RPC. That RPC also no longer reports the `ban_reason`
  field, as `"manually added"` is the only remaining option.

- Discouragement cannot be removed with the `setban remove` RPC command.
  If you need to remove a discouragement, you can remove all discouragements by
  stop-starting your node.

Notification changes
--------------------

`-walletnotify` notifications are now sent for wallet transactions that are
removed from the mempool because they conflict with a new block. These
notifications were sent previously before the v0.19 release, but had been
broken since that release (bug
[#18325](https://github.com/tortoisecoin/tortoisecoin/issues/18325)).

PSBT changes
------------

PSBTs will contain both the non-witness utxo and the witness utxo for segwit
inputs in order to restore compatibility with wallet software that are now
requiring the full previous transaction for segwit inputs. The witness utxo
is still provided to maintain compatibility with software which relied on its
existence to determine whether an input was segwit.

0.20.2 change log
=================

### P2P protocol and network code

- #19620 Add txids with non-standard inputs to reject filter (sdaftuar)
- #20146 Send post-verack handshake messages at most once (MarcoFalke)

### Wallet

- #19740 Simplify and fix CWallet::SignTransaction (achow101)

### RPC and other APIs

- #19836 Properly deserialize txs with witness before signing (MarcoFalke)
- #20731 Add missing description of vout in getrawtransaction help text (benthecarman)

### Build system

- #20142 build: set minimum required Boost to 1.48.0 (fanquake)
- #20298 use the new plistlib API (jonasschnelli)
- #20880 gitian: Use custom MacOS code signing tool (achow101)
- #22190 Use latest signapple commit (achow101)

### Tests and QA

- #19839 Set appveyor vm version to previous Visual Studio 2019 release. (sipsorcery)
- #19842 Update the vcpkg checkout commit ID in appveyor config. (sipsorcery)
- #20562 Test that a fully signed tx given to signrawtx is unchanged (achow101)

### Miscellaneous

- #19192 Extract net permissions doc (MarcoFalke)
- #19777 Correct description for getblockstats's txs field (shesek)
- #20080 Strip any trailing / in -datadir and -blocksdir paths (hebasto)
- #20082 fixes read buffer to use min rather than max (EthanHeilman)
- #20141 Avoid the use of abs64 in timedata (sipa)
- #20756 Add missing field (permissions) to the getpeerinfo help (amitiuttarwar)
- #20861 BIP 350: Implement Bech32m and use it for v1+ segwit addresses (sipa)
- #22124 Update translations after closing 0.20.x on Transifex (hebasto)
- #21471 fix bech32_encode calls in gen_key_io_test_vectors.py (sipa)
- #22837 mention bech32m/BIP350 in doc/descriptors.md (sipa)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Amiti Uttarwar
- Andrew Chow
- Ethan Heilman
- fanquake
- Hennadii Stepanov
- Jonas Schnelli
- MarcoFalke
- Nadav Ivgi
- Pieter Wuille
- Suhas Daftuar

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).
