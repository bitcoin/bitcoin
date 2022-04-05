*The release notes draft is a temporary file that can be added to by anyone. See
[/doc/developer-notes.md#release-notes](/doc/developer-notes.md#release-notes)
for the process.*

# Dash Core version *version*
===============================

This is a new minor version release, bringing various bugfixes and performance improvements.
This release is **optional** for all nodes, although recommended.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Dash-Qt` (on Mac) or
`dashd`/`dash-qt` (on Linux).

## Downgrade warning

### Downgrade to a version < *version*

Downgrading to a version older than *version* may not be supported, and will
likely require a reindex.

# Release Notes

Notable changes
===============

P2P and network changes
-----------------------

Updated RPCs
------------


Changes to wallet related RPCs can be found in the Wallet section below.

New RPCs
--------

Build System
------------

Updated settings
----------------


Changes to GUI or wallet related settings can be found in the GUI or Wallet section below.

New settings
------------

Tools and Utilities
-------------------

Wallet
------

GUI changes
-----------

Low-level changes
=================

RPC
---

Tests
-----

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

-
-
-

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

-
-
-

[set-of-changes]: https://github.com/dashpay/dash/compare/*version*...dashpay:*version*
