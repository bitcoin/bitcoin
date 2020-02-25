Dash Core version 0.14.0.5
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing various bugfixes and improvements.
It is highly recommended to upgrade to this release as it contains a critical
fix for a possible DoS vector.

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

Fix for a DoS vector
--------------------

This release fixes a serious DoS vector which allows to cause memory exhaustion until the point of
out-of-memory related crashes. We highly recommend upgrading all nodes. Thanks to Bitcoin ABC
developers for finding and reporting this issue to us.

Better handling of non-locked transactions in mined blocks
----------------------------------------------------------

We observed multiple cases of ChainLocks failing on mainnet. We tracked this down to a situation where
PrivateSend mixing transactions were first rejected by parts of the network (0.14.0.4 nodes) while other parts
(<=0.14.0.3) accepted the transaction into the mempool. This caused InstantSend locking to fail for these
transactions, while non-upgraded miners still included the transactions into blocks after 10 minutes.
This caused blocks to not get ChainLocked for at least 10 minutes. This release improves an already existent
fallback mechanism (retroactive InstantSend locking) to also work for transaction which are already partially
known in the network. This should cause ChainLocks to succeed in such situations.

0.14.0.5 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.14.0.4...dashpay:v0.14.0.5).

- [`20d4a27778`](https://github.com/dashpay/dash/commit/dc07a0c5e1) Make sure mempool txes are properly processed by CChainLocksHandler despite node restarts (#3230)
- [`dc07a0c5e1`](https://github.com/dashpay/dash/commit/dc07a0c5e1) [v0.14.0.x] Bump version and prepare release notes (#3228)
- [`401da32090`](https://github.com/dashpay/dash/commit/401da32090) More fixes in llmq-is-retroactive tests
- [`33721eaa11`](https://github.com/dashpay/dash/commit/33721eaa11) Make llmq-is-retroactive test compatible with 0.14.0.x
- [`85bd162a3e`](https://github.com/dashpay/dash/commit/85bd162a3e) Make wait_for_xxx methods compatible with 0.14.0.x
- [`22cfddaf12`](https://github.com/dashpay/dash/commit/22cfddaf12) Allow re-signing of IS locks when performing retroactive signing (#3219)
- [`a8b8891a1d`](https://github.com/dashpay/dash/commit/a8b8891a1d) Add wait_for_xxx methods as found in develop
- [`8dae12cc60`](https://github.com/dashpay/dash/commit/8dae12cc60) More/better logging for InstantSend
- [`fdd19cf667`](https://github.com/dashpay/dash/commit/fdd19cf667) Tests: Fix the way nodes are connected to each other in setup_network/start_masternodes (#3221)
- [`41f0e9d028`](https://github.com/dashpay/dash/commit/41f0e9d028) More fixes related to extra_args
- [`5213118601`](https://github.com/dashpay/dash/commit/5213118601) Tests: Allow specifying different cmd-line params for each masternode (#3222)
- [`2fef21fd80`](https://github.com/dashpay/dash/commit/2fef21fd80) Don't join thread in CQuorum::~CQuorum when called from within the thread (#3223)
- [`e69c6c3207`](https://github.com/dashpay/dash/commit/e69c6c3207) Merge #12392: Fix ignoring tx data requests when fPauseSend is set on a peer (#3225)

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

- [v0.14.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.4.md) released November/22/2019
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

