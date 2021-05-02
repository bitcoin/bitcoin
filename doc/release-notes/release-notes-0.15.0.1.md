XBit Core version *0.15.0.1* is now available from:

  <https://xbit.org/bin/xbit-core-0.15.0.1/>

and

  <https://xbitcore.org/bin/xbit-core-0.15.0.1/>

This is a minor bug fix for 0.15.0.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/xbit/xbit/issues>

To receive security and update notifications, please subscribe to:

  <https://xbitcore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the 
installer (on Windows) or just copy over `/Applications/XBit-Qt` (on Mac)
or `xbitd`/`xbit-qt` (on Linux).

The first time you run version 0.15.0 or higher, your chainstate database will
be converted to a new format, which will take anywhere from a few minutes to
half an hour, depending on the speed of your machine.

The file format of `fee_estimates.dat` changed in version 0.15.0. Hence, a
downgrade from version 0.15.0 or upgrade to version 0.15.0 will cause all fee
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

XBit Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

XBit Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

GUI startup crash issue
-------------------------

After upgrade to 0.15.0, some clients would crash at startup because a custom
fee setting was configured that no longer exists in the GUI. This is a minimal
patch to avoid this issue from occuring.

0.15.0.1 Change log
====================

-  #11332 `46c8d23` Fix possible crash with invalid nCustomFeeRadio in QSettings (achow101, TheBlueMatt)

Also the manpages were updated, as this was forgotten for 0.15.0.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrew Chow
- Matt Corallo
- Jonas Schnelli
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/xbit/).
