Dash Core version 0.14.0.3
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing various bugfixes and improvements.

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

Due to the changes in the "evodb" database format introduced in this release, the
first startup of Dash Core will run a migration process which can take a few minutes
to finish. After the migration, a downgrade to an older version is only possible with
a reindex (or reindex-chainstate).

Downgrade warning
-----------------

### Downgrade to a version < 0.14.0.3

Downgrading to a version smaller than 0.14.0.3 is not supported anymore due to changes
in the "evodb" database format. If you need to use an older version, you have to perform
a reindex or re-sync the whole chain.

Notable changes
===============

Database space usage improvements
--------------------------------
Version 0.13.0.0 introduced a new database (evodb) which is found in the datadir of Dash Core. It turned
out that this database grows quite fast when a lot of changes inside the deterministic masternode list happen,
which is for example the case when a lot PoSe punishing/banning is happening. Such a situation happened
immediately after the activation LLMQ DKGs, causing the database to grow a lot. This release introduces
a new format in which information in "evodb" is stored, which causes it grow substantially slower.  

Version 0.14.0.0 also introduced a new database (llmq) which is also found in the datadir of Dash Core.
This database stores all LLMQ signatures for 7 days. After 7 days, a cleanup task removes old signatures.
The idea was that the "llmq" database would grow in the beginning and then stay at an approximately constant
size. The recent stress test on mainnet has however shown that the database grows too much and causes a risk
of out-of-space situations. This release will from now also remove signatures when the corresponding InstantSend
lock is fully confirmed on-chain (superseded by a ChainLock). This should remove >95% of all signatures from
the database. After the upgrade, no space saving will be observed however as this logic is only applied to new
signatures, which means that it will take 7 days until the whole "llmq" database gets to its minimum size.

DKG and LLMQ signing failures fixed
-----------------------------------
Recent stress tests have shown that masternodes start to ban each other under high load and specific situations.
This release fixes this and thus makes it a highly recommended upgrade for masternodes.

MacOS: macOS: disable AppNap during sync and mixing
---------------------------------------------------
AppNap is disabled now when Dash Core is syncing/reindexing or mixing.

Signed binaries for Windows
---------------------------
This release is the first one to include signed binaries for Windows.

New RPC command: quorum memberof <proTxHash>
--------------------------------------------
This RPC allows you to verify which quorums a masternode is supposed to be a member of. It will also show
if the masternode succesfully participated in the DKG process.

More information about number of InstantSend locks
--------------------------------------------------
The debug console will now show how many InstantSend locks Dash Core knows about. Please note that this number
does not necessarily equal the number of mempool transactions.

The "getmempoolinfo" RPC also has a new field now which shows the same information.

0.14.0.3 Change log
===================

TODO

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.14.0.2...dashpay:v0.14.0.3).


Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- Nathan Marley (nmarley)
- PastaPastaPasta
- strophy
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

- [v0.14.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.2.md) released July/4/2019
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

