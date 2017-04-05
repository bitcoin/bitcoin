Bitcoin Core version 0.14.1 is now available from:

  <https://bitcoin.org/bin/bitcoin-core-0.14.1/>

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

RPC changes
-----------

The first positional argument of `createrawtransaction` was renamed.
This interface change breaks compatibility with 0.14.0, when the named
arguments functionality, introduced in 0.14.0, is used.

0.14.1 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and other APIs
- #10084 `142fbb2` Rename first named arg of createrawtransaction (MarcoFalke)
- #10139 `f15268d` Remove auth cookie on shutdown (practicalswift)
- #10146 `2fea10a` Better error handling for submitblock (gmaxwell)
- #10144 `d947afc` Prioritisetransaction wasn't always updating ancestor fee (sdaftuar)

### Block and transaction handling
- #10126 `0b5e162` Compensate for memory peak at flush time (sipa)
- #9912 `fc3d7db` Optimize GetWitnessHash() for non-segwit transactions (sdaftuar)
- #10133 `ab864d3` Clean up calculations of pcoinsTip memory usage (morcos)

### P2P protocol and network code
- #9953/#10013 `d2548a4` Fix shutdown hang with >= 8 -addnodes set (TheBlueMatt)

### Build system
- #9973 `e9611d1` depends: fix zlib build on osx (theuni)

### GUI
- #10060 `ddc2dd1` Ensure an item exists on the rpcconsole stack before adding (achow101)

### Mining
- #9955/#10006 `569596c` Don't require segwit in getblocktemplate for segwit signalling or mining (sdaftuar)
- #9959/#10127 `b5c3440` Prevent slowdown in CreateNewBlock on large mempools (sdaftuar)

### Miscellaneous
- #10094 `37bf0d5` 0.14: Clear release notes (MarcoFalke)
- #10037 `4d8e660` Trivial: Fix typo in help getrawtransaction RPC (keystrike)
- #10120 `e4c9a90` util: Work around (virtual) memory exhaustion on 32-bit w/ glibc (laanwj)
- #10130 `ecc5232` bitcoin-tx input verification (awemany, jnewbery)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alex Morcos
- Andrew Chow
- Awemany
- Cory Fields
- Gregory Maxwell
- James Evans
- John Newbery
- MarcoFalke
- Matt Corallo
- Pieter Wuille
- practicalswift
- Suhas Daftuar
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).

