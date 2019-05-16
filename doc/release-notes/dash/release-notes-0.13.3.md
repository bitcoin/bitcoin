Dash Core version 0.13.3.0
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

As spork15 has been activated on mainnet, there is no need for `masternode start`
anymore. Upgrading a masternode now only involves replacing binaries and restarting
the node.

Downgrade warning
-----------------

### Downgrade to a version < 0.13.0.0

Downgrading to a version smaller than 0.13 is not supported anymore as DIP2/DIP3 has activated
on mainnet and testnet.

### Downgrade to a version < 0.13.3.0

Downgrading to previous 0.13 releases is fully supported but is not recommended unless you have some serious issues with 0.13.3.0.

Notable changes
===============

Number of false-positives from anti virus software should be reduced
--------------------------------------------------------------------
We have removed all mining code from Windows and Mac binaries, which should avoid most of the false-positive alerts
from anti virus software. Linux builds are not affected. The mining code found in `dash-qt` and `dashd` are only meant
for regression/integration tests and devnets, so there is no harm in removing this code from non-linux builds.

Fixed an issue with invalid merkle blocks causing SPV nodes to ban other nodes
------------------------------------------------------------------------------
A fix that was introduces in the last minor version caused creation of invalid merkle blocks, which in turn cause SPV
nodes to ban 0.13.2 nodes. This can be observed on mobile clients which have troubles maintaining connections. This
release fixes this issue and should allow SPV/mobile clients to sync with upgraded nodes.

Hardened spork15 value to 1047200
---------------------------------
We've hardened the spork15 block height to 1047200, which makes sure that syncing from scratch will always work, no
matter if spork15 is received in-time or not.

Bug fixes/Other improvements
----------------------------
There are few bug fixes in this release:
- Fixed an issue with transaction sometimes not being fully zapped when `-zapwallettxes` is used
- Fixed an issue with the `protx revoke` RPC and REASON_CHANGE_OF_KEYS

 0.13.3.0 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.13.2.0...dashpay:v0.13.3.0).

### Backports

- [`575cafc01`](https://github.com/dashpay/dash/commit/575cafc01) Do not skip pushing of vMatch and vHashes in CMerkleBlock (#2826)
- [`8c58799d8`](https://github.com/dashpay/dash/commit/8c58799d8) There can be no two votes which differ by the outcome only (#2819)
- [`66c2f3953`](https://github.com/dashpay/dash/commit/66c2f3953) Fix vote ratecheck (#2813)
- [`b52d0ad19`](https://github.com/dashpay/dash/commit/b52d0ad19) Fix revoke reason check for ProUpRevTx (#2787)
- [`35914e084`](https://github.com/dashpay/dash/commit/35914e084) Skip mempool.dat when wallet is starting in "zap" mode (#2782)
- [`46d875100`](https://github.com/dashpay/dash/commit/46d875100) Disable in-wallet miner for win/macos Travis/Gitian builds (#2778)

### Other

- [`25038ff36`](https://github.com/dashpay/dash/commit/25038ff36) Bump version to 0.13.3.0
- [`53b2162e2`](https://github.com/dashpay/dash/commit/53b2162e2) Harden spork15 value to 1047200 when on mainnet (#2830)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- gladcow
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

