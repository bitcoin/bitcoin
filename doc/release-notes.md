(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

Bitcoin Core version 0.14.x is now available from:

  <https://bitcoin.org/bin/bitcoin-core-0.14.x/>

This is a new minor version release, including various bugfixes and
performance improvements, as well as updated translations.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

Compatibility
==============

Bitcoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later.

Microsoft ended support for Windows XP on [April 8th, 2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support),
No attempt is made to prevent installing or running the software on Windows XP, you
can still do so at your own risk but be aware that there are known instabilities and issues.
Please do not report issues about Windows XP to the issue tracker.

Bitcoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

Example item
-----------------------------------------------

RPC changes
-----------

The first positional argument of `createrawtransaction` was renamed.
This interface change breaks compatibility with 0.14.0, when the named
arguments functionality, introduced in 0.14.0, is used.

0.14.x Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

[to be filled in at release]

Credits
=======

Thanks to everyone who directly contributed to this release:

[to be filled in at release]

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).

