*After branching off for a major version release of Syscoin Core, use this
template to create the initial release notes draft.*

*The release notes draft is a temporary file that can be added to by anyone. See
[/doc/developer-notes.md#release-notes](/doc/developer-notes.md#release-notes)
for the process.*

*Create the draft, named* "*version* Release Notes Draft"
*(e.g. "4.3.0 Release Notes Draft"), as a collaborative wiki in:*

https://github.com/syscoin-core/syscoin-devwiki/wiki/

*Before the final release, move the notes back to this git repository.*

*version* Release Notes Draft
===============================

Syscoin Core version *version* is now available from:

  <https://syscoincore.org/bin/syscoin-core-*version*/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>

To receive security and update notifications, please subscribe to:

  <https://syscoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux).

Upgrading directly from a version of Syscoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Syscoin Core are generally supported.

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.14+, and Windows 7 and newer.  Syscoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Syscoin Core on
unsupported systems.

Notable changes
===============

P2P and network changes
-----------------------

- A bitcoind node will no longer rumour addresses to inbound peers by default.
  They will become eligible for address gossip after sending an ADDR, ADDRV2,
  or GETADDR message. (#21528)

Updated RPCs
------------

New RPCs
--------

Build System
------------

Files
-----

* On startup, the list of banned hosts and networks (via `setban` RPC) in
  `banlist.dat` is ignored and only `banlist.json` is considered. Bitcoin Core
  version 22.x is the only version that can read `banlist.dat` and also write
  it to `banlist.json`. If `banlist.json` already exists, version 22.x will not
  try to translate the `banlist.dat` into json. After an upgrade, `listbanned`
  can be used to double check the parsed entries. (#22570)

New settings
------------

Updated settings
----------------

Tools and Utilities
-------------------

- Update `-getinfo` to return data in a user-friendly format that also reduces vertical space. (#21832)

Wallet
------

GUI changes
-----------

Low-level changes
=================

RPC
---

- `getblockchaininfo` now returns a new `time` field, that provides the chain tip time. (#22407)

Tests
-----

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/syscoin/syscoin/).
