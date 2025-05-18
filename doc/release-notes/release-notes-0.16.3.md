Tortoisecoin Core version 0.16.3 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-0.16.3/>

This is a new minor version release, with various bugfixes.

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

The first time you run version 0.15.0 or newer, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0 or higher. Upgrading
directly from 0.7.x and earlier without re-downloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

Wallets created in 0.16 and later are not compatible with versions prior to 0.16
and will not work if you try to use newly created wallets in older versions. Existing
wallets that were created with older versions are not affected by this.

Compatibility
==============

Tortoisecoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

Tortoisecoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

Denial-of-Service vulnerability
-------------------------------

A denial-of-service vulnerability (CVE-2018-17144) exploitable by miners has
been discovered in Tortoisecoin Core versions 0.14.0 up to 0.16.2. It is recommended
to upgrade any of the vulnerable versions to 0.16.3 as soon as possible.

0.16.3 change log
------------------

### Consensus
- #14249 `696b936` Fix crash bug with duplicate inputs within a transaction (TheBlueMatt, sdaftuar)

### RPC and other APIs
- #13547 `212ef1f` Make `signrawtransaction*` give an error when amount is needed but missing (ajtowns)

### Miscellaneous
- #13655 `1cdbea7` tortoisecoinconsensus: invalid flags error should be set to `tortoisecoinconsensus_err` (afk11)

### Documentation
- #13844 `11b9dbb` correct the help output for -prune (hebasto)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Anthony Towns
- Hennadii Stepanov
- Matt Corallo
- Suhas Daftuar
- Thomas Kerin
- Wladimir J. van der Laan

And to those that reported security issues:

- (anonymous reporter)

