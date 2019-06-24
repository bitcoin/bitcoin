Dash Core version 0.14.0.1
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

### Downgrade to a version < 0.13.0.0

Downgrading to a version smaller than 0.13 is not supported anymore as DIP2/DIP3 has
activated on mainnet and testnet.

### Downgrade to versions 0.13.0.0 - 0.13.3.0

Downgrading to 0.13 releases is fully supported until DIP0008 activation but is not
recommended unless you have some serious issues with version 0.14.

Notable changes
===============

Fixed governance votes pruning for invalid masternodes 
------------------------------------------------------
A community member reported a possible attack that involves DoSing masternodes to force the network
to prune all governance votes from this masternodes. This could be used to manipulate vote outcomes.

This vulnerability is currently not possible to execute as LLMQ DKGs and PoSe have not activated yet on
mainnet. This version includes a fix that requires to have at least 51% masternodes to upgrade to
0.14.0.1, after which superblock trigger voting will automatically fix the discrepancies between
old and new nodes. This also means that we will postpone activation of LLMQ DKGs and thus PoSe until
at least 51% of masternodes have upgraded to 0.14.0.1.

Fixed a rare memory/db leak in LLMQ based InstantSend
-----------------------------------------------------
We fixed a rare memory/db leak in LLMQ based InstantSend leak which would only occur when reorganizations
would happen.

0.14.0.1 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.14.0.0...dashpay:v0.14.0.1).

- [`a2baa93ec`](https://github.com/dashpay/dash/commit/a2baa93ec) Only require valid collaterals for votes and triggers (#2947) (#2957)
- [`b293e6dde`](https://github.com/dashpay/dash/commit/b293e6dde) Fix off-by-one error in InstantSend mining info removal when disconnecting blocks (#2951)
- [`276b6e3a8`](https://github.com/dashpay/dash/commit/276b6e3a8) bump version to 0.14.0.1 and prepare release notes (#2952)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- demodun6
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

