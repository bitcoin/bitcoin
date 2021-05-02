0.20.1 Release Notes
====================

XBit Core version 0.20.1 is now available from:

  <https://xbitcore.org/bin/xbit-core-0.20.1/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/xbit/xbit/issues>

To receive security and update notifications, please subscribe to:

  <https://xbitcore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/XBit-Qt` (on Mac)
or `xbitd`/`xbit-qt` (on Linux).

Upgrading directly from a version of XBit Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of XBit Core are generally supported.

Compatibility
==============

XBit Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  XBit
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use XBit Core on
unsupported systems.

From XBit Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, XBit Core does not yet change appearance
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
  `XBIT_GENBUILD_NO_GIT=1 make`.

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
[#18325](https://github.com/xbit/xbit/issues/18325)).

PSBT changes
------------

PSBTs will contain both the non-witness utxo and the witness utxo for segwit
inputs in order to restore compatibility with wallet software that are now
requiring the full previous transaction for segwit inputs. The witness utxo
is still provided to maintain compatibility with software which relied on its
existence to determine whether an input was segwit.

0.20.1 change log
=================

### Mining
- #19019 Fix GBT: Restore "!segwit" and "csv" to "rules" key (luke-jr)

### P2P protocol and network code
- #19219 Replace automatic bans with discouragement filter (sipa)

### Wallet
- #19300 Handle concurrent wallet loading (promag)
- #18982 Minimal fix to restore conflicted transaction notifications (ryanofsky)

### RPC and other APIs
- #19524 Increment input value sum only once per UTXO in decodepsbt (fanquake)
- #19517 psbt: Increment input value sum only once per UTXO in decodepsbt (achow101)
- #19215 psbt: Include and allow both non_witness_utxo and witness_utxo for segwit inputs (achow101)

### GUI
- #19097 Add missing QPainterPath include (achow101)
- #19059 update Qt base translations for macOS release (fanquake)

### Build system
- #19152 improve build OS configure output (skmcontrib)
- #19536 qt, build: Fix QFileDialog for static builds (hebasto)

### Tests and QA
- #19444 Remove cached directories and associated script blocks from appveyor config (sipsorcery)
- #18640 appveyor: Remove clcache (MarcoFalke)

### Miscellaneous
- #19194 util: Don't reference errno when pthread fails (miztake)
- #18700 Fix locking on WSL using flock instead of fcntl (meshcollider)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Andrew Chow
- fanquake
- Hennadii Stepanov
- João Barbosa
- Luke Dashjr
- MarcoFalke
- MIZUTA Takeshi
- Pieter Wuille
- Russell Yanofsky
- sachinkm77
- Samuel Dobson
- Wladimir J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/xbit/xbit/).
