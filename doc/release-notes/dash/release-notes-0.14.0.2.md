Dash Core version 0.14.0.2
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing various bugfixes.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading from
version 0.13 should not require any additional actions.

Downgrade warning
-----------------

### Downgrade to a version < 0.14.0.0

Downgrading to a version smaller than 0.14 is not supported anymore as DIP8 has
activated on mainnet and testnet.

### Downgrade to versions 0.14.0.0 - 0.14.0.1

Downgrading to older 0.14 releases is fully supported but is not
recommended unless you have some serious issues with version 0.14.0.2.

Notable changes
===============

Performance improvements
------------------------
Slow startup times were observed in older versions. This was due to sub-optimal handling of old
deterministic masternode lists which caused the loading of too many lists into memory. This should be
fixed now.

Fixed excessive memory use
--------------------------
Multiple issues were found which caused excessive use of memory in some situations, especially when
a full reindex was performed, causing the node to crash even when enough RAM was available. This should
be fixed now.

Fixed out-of-sync masternode list UI
------------------------------------
The masternode tab, which shows the masternode list, was not always up-to-date as it missed some internal
updates. This should be fixed now.

0.14.0.2 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.14.0.1...dashpay:v0.14.0.2).

- [`d2ff63e8d`](https://github.com/dashpay/dash/commit/d2ff63e8d) Use std::unique_ptr for mnList in CSimplifiedMNList (#3014)
- [`321bbf5af`](https://github.com/dashpay/dash/commit/321bbf5af) Fix excessive memory use when flushing chainstate and EvoDB (#3008)
- [`0410259dd`](https://github.com/dashpay/dash/commit/0410259dd) Fix 2 common Travis failures which happen when Travis has network issues (#3003)
- [`8d763c144`](https://github.com/dashpay/dash/commit/8d763c144) Only load signingActiveQuorumCount + 1 quorums into cache (#3002)
- [`2dc1b06ec`](https://github.com/dashpay/dash/commit/2dc1b06ec) Remove skipped denom from the list on tx commit (#2997)
- [`dff2c851d`](https://github.com/dashpay/dash/commit/dff2c851d) Update manpages for 0.14.0.2 (#2999)
- [`46c4f5844`](https://github.com/dashpay/dash/commit/46c4f5844) Use Travis stages instead of custom timeouts (#2948)
- [`49c37b82a`](https://github.com/dashpay/dash/commit/49c37b82a) Back off for 1m when connecting to quorum masternodes (#2975)
- [`c1f756fd9`](https://github.com/dashpay/dash/commit/c1f756fd9) Multiple speed optimizations for deterministic MN list handling (#2972)
- [`11699f540`](https://github.com/dashpay/dash/commit/11699f540) Process/keep messages/connections from PoSe-banned MNs (#2967)
- [`c5415e746`](https://github.com/dashpay/dash/commit/c5415e746) Fix UI masternode list (#2966)
- [`fb6f0e04d`](https://github.com/dashpay/dash/commit/fb6f0e04d) Bump version to 0.14.0.2 and copy release notes (#2991)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- UdjinM6

As well as everyone that submitted issues and reviewed pull requests.

Older releases
==============

Dash was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Dash Core tree 0.11.x was a fork of Bitcoin Core tree 0.9,
Darkcoin was rebranded to Dash.

Dash Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Dash Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old release notes can be found here:

- [v0.14.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.1.md) released May/31/2019
- [v0.14.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.md) released May/22/2019
- [v0.13.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.3.md) released Apr/04/2019
- [v0.13.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.2.md) released Mar/15/2019
- [v0.13.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.1.md) released Feb/9/2019
- [v0.13.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.0.md) released Jan/14/2019
- [v0.12.3.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.4.md) released Dec/14/2018
- [v0.12.3.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.3.md) released Sep/19/2018
- [v0.12.3.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.2.md) released Jul/09/2018
- [v0.12.3.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.1.md) released Jul/03/2018
- [v0.12.2.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Aug/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

