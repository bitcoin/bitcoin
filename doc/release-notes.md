Dash Core version 0.14.0.4
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

When upgrading from a version prior to 0.14.0.3, the
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

Fix respends of freshly received InstantSend transactions
---------------------------------------------------------

A bug in Dash Core caused respends to not work before a received InstantSend transaction was confirmed in at least
one block. This is fixed in this release, so that InstantSend locked mempool transactions can be
respent immediately in Dash Core (other wallets were not affected).

Deprecation of SPORK_16_INSTANTSEND_AUTOLOCKS
---------------------------------------------

With the activation of SPORK_20_INSTANTSEND_LLMQ_BASED a few month ago, all transactions started to be locked via
InstantSend, which already partly deprecated SPORK_16_INSTANTSEND_AUTOLOCKS. This release removes the last use
of SPORK_16_INSTANTSEND_AUTOLOCKS, which caused InstantSend to stop working when the mempool got too large.

Improve orphan transaction limit handling
-----------------------------------------

Instead of limiting orphan transaction by number of transaction, we limit orphans by total size in bytes
now. This allows to have thousands of orphan transactions before hitting the limit.

Discrepancies in orphan sets between nodes and handling of those was one of the major limiting factors in
the stress tests performed by an unknown entity on mainnet.

Improve re-requesting for already known transactions
----------------------------------------------------

Previously, Dash would re-request old transactions even though they were already known locally. This
happened when the outputs were respent very shortly after confirmation of the transaction. This lead to
wrongly handling these transactions as orphans, filling up the orphan set and hitting limits very fast.
This release fixes this for nodes which have txindex enabled, which is the case for all masternodes. Normal
nodes (without txindex) can ignore the issue as they are not involved in active InstantSend locking.

Another issue fixed in this release is the re-requesting of transactions after an InstantSend lock invalidated
a conflicting transaction.

Multiple improvements to PrivateSend
------------------------------------

Multiple improvements to PrivateSend are introduced in this release, leading to faster mixing and more
reasonable selection of UTXOs when sending PrivateSend funds.

Fix for CVE-2017-18350
----------------------

Bitcoin silently implemented a hidden fix for [CVE-2017-18350](https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2019-November/017453.html).
in Bitcoin v0.15.1. This release of Dash Core includes a backport of this fix.


0.14.0.4 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.14.0.3...dashpay:v0.14.0.4).

- [`5f98ed7a5`](https://github.com/dashpay/dash/commit/5f98ed7a5) [v0.14.0.x] Bump version to 0.14.0.4 and draft release notes (#3203)
- [`c0dda38fe`](https://github.com/dashpay/dash/commit/c0dda38fe) Circumvent BIP69 sorting in fundrawtransaction.py test (#3100)
- [`64ae6365f`](https://github.com/dashpay/dash/commit/64ae6365f) Fix compile issues
- [`36473015b`](https://github.com/dashpay/dash/commit/36473015b) Merge #11397: net: Improve and document SOCKS code
- [`66e298728`](https://github.com/dashpay/dash/commit/66e298728) Slightly optimize ApproximateBestSubset and its usage for PS txes (#3184)
- [`16b6b6f7c`](https://github.com/dashpay/dash/commit/16b6b6f7c) Update activemn if protx info changed (#3176)
- [`ce6687130`](https://github.com/dashpay/dash/commit/ce6687130) Actually update spent index on DisconnectBlock (#3167)
- [`9b49bfda8`](https://github.com/dashpay/dash/commit/9b49bfda8) Only track last seen time instead of first and last seen time (#3165)
- [`ad720eef1`](https://github.com/dashpay/dash/commit/ad720eef1) Merge #17118: build: depends macOS: point --sysroot to SDK (#3161)
- [`909d6a4ba`](https://github.com/dashpay/dash/commit/909d6a4ba) Simulate BlockConnected/BlockDisconnected for PS caches
- [`db7f471c7`](https://github.com/dashpay/dash/commit/db7f471c7) Few fixes related to SelectCoinsGroupedByAddresses (#3144)
- [`1acd4742c`](https://github.com/dashpay/dash/commit/1acd4742c) Various fixes for mixing queues (#3138)
- [`0031d6b04`](https://github.com/dashpay/dash/commit/0031d6b04) Also consider txindex for transactions in AlreadyHave() (#3126)
- [`c4be5ac4d`](https://github.com/dashpay/dash/commit/c4be5ac4d) Ignore recent rejects filter for locked txes (#3124)
- [`f2d401aa8`](https://github.com/dashpay/dash/commit/f2d401aa8) Make orphan TX map limiting dependent on total TX size instead of TX count (#3121)
- [`87ff566a0`](https://github.com/dashpay/dash/commit/87ff566a0) Update/modernize macOS plist (#3074)
- [`2141d5f9d`](https://github.com/dashpay/dash/commit/2141d5f9d) Fix bip69 vs change position issue (#3063)
- [`75fddde67`](https://github.com/dashpay/dash/commit/75fddde67) Partially revert 3061 (#3150)
- [`c74f2cd8b`](https://github.com/dashpay/dash/commit/c74f2cd8b) Fix SelectCoinsMinConf to allow instant respends (#3061)
- [`2e7ec2369`](https://github.com/dashpay/dash/commit/2e7ec2369) [0.14.0.x] Remove check for mempool size in CInstantSendManager::CheckCanLock (#3119)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- Nathan Marley (nmarley)
- PastaPastaPasta
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

- [v0.14.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.3.md) released August/15/2019
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

