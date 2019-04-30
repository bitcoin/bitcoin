Dash Core version 0.13.1.0
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing various bugfixes and other improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation you will
have to reindex (start with -reindex-chainstate or -reindex) to make sure
your wallet has all the new data synced (only if you were using version < 0.13).

Note that there is no protocol bump in this version and thus active masternodes
updating from v0.13.0.0 do not require any additional actions (no need to issue
`masternode start` command).

Downgrade warning
-----------------

### Downgrade to a version < 0.13.0.0

Downgrading to a version smaller than 0.13 is only supported as long as DIP2/DIP3
has not been activated. Activation will happen when enough miners signal compatibility
through a BIP9 (bit 3) deployment.

### Downgrade to 0.13.0.0

Downgrading to 0.13.0.0 is fully supported but is not recommended unless you have some serious issues with 0.13.1.0.

Notable changes
===============

DIP0003 block signaling
-----------------------
Miners running v0.13.1.0 are going to signal DIP3 regardles of the readiness of the corresponding masternode.
With 70%+ masternodes already running on v0.13.0.0 we believe it's safe to slightly speed up the migration
this way. This is fully backwards compatible and no update is required for (non-mining) nodes running on v0.13.0.0.

GUI changes
-----------
Masternodes tab has a new checkbox that should filter masternode list by using keys stored in the wallet.
This should make it much easier for masternode owners to find their masternodes in the list.

RPC changes
-----------
There is a new RPC command `getspecialtxes` which returns an array of special transactions found in the specified
block with different level of verbosity. Also, various `protx` commands show extended help text for each parameter
now (instead of referencing `protx register`).

Bug fixes/Other improvements
----------------------------
There are few bug fixes in this release:
- Block size should be calculated correctly for blocks with quorum commitments when constructing new block;
- Special transactions should be checked for mempool acceptance at the right time (nodes could ban each other
in some rare cases otherwise);
- Signature verification and processing of special transactions is decoupled to avoid any potential issues.

 0.13.1.0 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.13.0.0...dashpay:v0.13.1.0).

### Backports

- [`da5a861c0`](https://github.com/dashpay/dash/commit/da5a861c0) Change the way invalid ProTxes are handled in `addUnchecked` and `existsProviderTxConflict` (#2691)
- [`6ada90c11`](https://github.com/dashpay/dash/commit/6ada90c11) Call existsProviderTxConflict after CheckSpecialTx (#2690)
- [`23eb70cb7`](https://github.com/dashpay/dash/commit/23eb70cb7) Add getspecialtxes rpc (#2668)
- [`023f8a01a`](https://github.com/dashpay/dash/commit/023f8a01a) Fix bench log for payee and special txes (#2678)
- [`8961a6acc`](https://github.com/dashpay/dash/commit/8961a6acc) Stop checking MN protocol version before signalling DIP3 (#2684)
- [`e18916386`](https://github.com/dashpay/dash/commit/e18916386) Add missing help text for `operatorPayoutAddress` (#2679)
- [`0d8cc0761`](https://github.com/dashpay/dash/commit/0d8cc0761) Invoke CheckSpecialTx after all normal TX checks have passed (#2673)
- [`592210daf`](https://github.com/dashpay/dash/commit/592210daf) Bump block stats when adding commitment tx into block (#2654)
- [`070ad103f`](https://github.com/dashpay/dash/commit/070ad103f) Wait for script checks to finish before messing with txes in Dash-specific way (#2652)
- [`3a3586d5a`](https://github.com/dashpay/dash/commit/3a3586d5a) Use helper function to produce help text for params of `protx` rpcs (#2649)
- [`332e0361c`](https://github.com/dashpay/dash/commit/332e0361c) Add checkbox to show only masternodes the wallet has keys for (#2627)

### Other

- [`bd0de4876`](https://github.com/dashpay/dash/commit/bd0de4876) Bump version to 0.13.1 (#2686)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block
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

