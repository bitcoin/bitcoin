XBit Core version *0.14.3* is now available from:

  <https://xbit.org/bin/xbit-core-0.14.3/>

This is a new minor version release, including various bugfixes and
performance improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/xbit/xbit/issues>

To receive security and update notifications, please subscribe to:

  <https://xbitcore.org/en/list/announcements/join/>

Compatibility
==============

XBit Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later.

Microsoft ended support for Windows XP on [April 8th, 2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support),
No attempt is made to prevent installing or running the software on Windows XP, you
can still do so at your own risk but be aware that there are known instabilities and issues.
Please do not report issues about Windows XP to the issue tracker.

XBit Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

Denial-of-Service vulnerability CVE-2018-17144
 -------------------------------

A denial-of-service vulnerability exploitable by miners has been discovered in
XBit Core versions 0.14.0 up to 0.16.2. It is recommended to upgrade any of
the vulnerable versions to 0.14.3, 0.15.2 or 0.16.3 as soon as possible.

Known Bugs
==========

Since 0.14.0 the approximate transaction fee shown in XBit-Qt when using coin
control and smart fee estimation does not reflect any change in target from the
smart fee slider. It will only present an approximate fee calculated using the
default target. The fee calculated using the correct target is still applied to
the transaction and shown in the final send confirmation dialog.

0.14.3 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### Consensus
- #14247 `52965fb` Fix crash bug with duplicate inputs within a transaction (TheBlueMatt, sdaftuar)
 
### RPC and other APIs

- #10445 `87a21d5` Fix: make CCoinsViewDbCursor::Seek work for missing keys (Pieter Wuille, Gregory Maxwell)
- #9853 Return correct error codes in setban(), fundrawtransaction(), removeprunedfunds(), bumpfee(), blockchain.cpp (John Newbery)


### P2P protocol and network code

- #10234 `d289b56` [net] listbanned RPC and QT should show correct banned subnets (John Newbery)

### Build system


### Miscellaneous

- #10451 `3612219` contrib/init/xbitd.openrcconf: Don't disable wallet by default (Luke Dashjr)
- #10250 `e23cef0` Fix some empty vector references (Pieter Wuille)
- #10196 `d28d583` PrioritiseTransaction updates the mempool tx counter (Suhas Daftuar)
- #9497 `e207342` Fix CCheckQueue IsIdle (potential) race condition and remove dangerous constructors. (Jeremy Rubin)

### GUI

- #9481 `7abe7bb` Give fallback fee a reasonable indent (Luke Dashjr)
- #9481 `3e4d7bf` Qt/Send: Figure a decent warning colour from theme (Luke Dashjr)
- #9481 `e207342` Show more significant warning if we fall back to the default fee (Jonas Schnelli)

### Wallet

- #10308 `28b8b8b` Securely erase potentially sensitive keys/values (tjps)
- #10265 `ff13f59` Make sure pindex is non-null before possibly referencing in LogPrintf call. (Karl-Johan Alm)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Cory Fields
- CryptAxe
- fanquake
- Jeremy Rubin
- John Newbery
- Jonas Schnelli
- Gregory Maxwell
- Karl-Johan Alm
- Luke Dashjr
- MarcoFalke
- Matt Corallo
- Mikerah
- Pieter Wuille
- practicalswift
- Suhas Daftuar
- Thomas Snider
- Tjps
- Wladimir J. van der Laan

And to those that reported security issues:

- awemany (for CVE-2018-17144, previously credited as "anonymous reporter")

