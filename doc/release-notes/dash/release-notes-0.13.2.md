Dash Core version 0.13.2.0
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
updating from v0.13.0.0 or v0.13.1.0 do not require any additional actions (no need to issue
`masternode start` command).

Downgrade warning
-----------------

### Downgrade to a version < 0.13.0.0

Downgrading to a version smaller than 0.13 is not supported anymore as DIP2/DIP3 has activated
on mainnet and testnet.

### Downgrade to 0.13.0.0 or 0.13.1.0

Downgrading to 0.13.0.0 is fully supported but is not recommended unless you have some serious issues with 0.13.2.0.

Notable changes
===============

Providing "masternodeblsprivkey" is now mandatory when the node is launched as a masternode ("masternode=1")
------------------------------------------------------------------------
In previous versions, "masternodeblsprivkey" was not mandatory as these versions had to function with and without DIP3
activation. Now that DIP3 has activated on mainnet and testnet, we can make "masternodeblsprivkey" mandatory when
configuring and running a masternode. Please note that your masternode will fail to start when "masternodeblsprivkey"
is not specified. This also means that 0.13.2.0 will only work with masternodes which have already registered their
DIP3 masternode. This enforcement was added to catch misconfigurations of masternodes which would otherwise stay
unnoticed until spork 15 activation and thus surprise and hurt masternode owners.

Fix for consistency issues after sudden stopping of node
--------------------------------------------------------
Previous versions resulted in inconsistency between the chainstate and evodb when the node crashed or otherwise suddenly
stopped (e.g. power failure). This should be fixed in 0.13.2.0. 

Fix for litemode nodes to not reject specific DIP3 transactions
---------------------------------------------------------------
Previous versions might cause litemode nodes to reject the mainnet chain after spork 15 activation. This is due to a
consensus rule being less strict in one specific case when spork 15 is active. Litemode nodes can not know about the
change in consensus rules as they have no knowledge about sporks. In 0.13.2.0, when litemode is enabled, we default to the
behaviour of activated spork15 in this specific case, which fixes the issue. The restriction will be completely removed
in the next major release.

Fix incorrect behavior for "protx diff" and the P2P message "GETMNLISTDIFF"
---------------------------------------------------------------------------
Both were responding with errors when "0" was used as base block hash. DIP4 defines "0" to be equivalent with the
genesis block, so that it's easy for peers to request the full masternode list.
This is mostly important for SPV nodes (e.g. mobile wallets) which need the masternode list. Right now, all nodes in
the network will respond with an error when "0" is provided in  "GETMNLISTDIFF". Until enough masternodes have upgraded
to 0.13.2.0, SPV nodes should use the full genesis hash to circumvent the error.

Exclusion of LLMQ quorum commitments from partial blocks
--------------------------------------------------------
SPV nodes are generally not interested in non-financial special transactions in blocks, so we're omitting them now when
sending partial/filtered blocks to SPV clients. This currently only filters LLMQ quorum commitments, which also caused
some SPV implementations to ban nodes as they were not able to process these. DIP3 transactions (ProRegTx, ProUpRegTx, ...)
are not affected and are still included in partial/filtered blocks as these might also move funds. 

RPC changes
-----------
`masternode list json` and `protx list` will now include the collateral address of masternodes.

Bug fixes/Other improvements
----------------------------
There are few bug fixes in this release:
- Fixed a crash on shutdown
- Fixed a misleading error message in the RPC "protx update_registrar"  
- Slightly speed up initial sync by not running DIP3 logic in old blocks
- Add build number (CLIENT_VERSION_BUILD) to MacOS bundle information 

 0.13.2.0 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.13.1.0...dashpay:v0.13.2.0).

### Backports

- [`548a48918`](https://github.com/dashpay/dash/commit/548a48918) Move IS block filtering into ConnectBlock (#2766)
- [`6374dce99`](https://github.com/dashpay/dash/commit/6374dce99) Fix error message for invalid voting addresses (#2747)
- [`25222b378`](https://github.com/dashpay/dash/commit/25222b378) Make -masternodeblsprivkey mandatory when -masternode is given (#2745)
- [`0364e033a`](https://github.com/dashpay/dash/commit/0364e033a) Implement 2-stage commit for CEvoDB to avoid inconsistencies after crashes (#2744)
- [`a11e2f9eb`](https://github.com/dashpay/dash/commit/a11e2f9eb) Add collateraladdress into masternode/protx list rpc output (#2740)
- [`43612a272`](https://github.com/dashpay/dash/commit/43612a272) Only include selected TX types into CMerkleBlock (#2737)
- [`f868fbc78`](https://github.com/dashpay/dash/commit/f868fbc78) Stop g_connman first before deleting it (#2734)
- [`9e233f391`](https://github.com/dashpay/dash/commit/9e233f391) Fix incorrect usage of begin() when genesis block is requested in "protx diff" (#2699)
- [`e75f971b9`](https://github.com/dashpay/dash/commit/e75f971b9) Do not process blocks in CDeterministicMNManager before dip3 activation (#2698)
- [`1cc47ebcd`](https://github.com/dashpay/dash/commit/1cc47ebcd) Backport #14701: build: Add CLIENT_VERSION_BUILD to CFBundleGetInfoString (#2687)

### Other

- [`2516a6e19`](https://github.com/dashpay/dash/commit/2516a6e19) Bump version to 0.13.2
- [`9dd16cdbe`](https://github.com/dashpay/dash/commit/9dd16cdbe) Bump minChainWork and AssumeValid to block #1033120 (#2750)
- [`18f087b27`](https://github.com/dashpay/dash/commit/18f087b27) Fix some typos in doc/guide-startmany.md (#2711)
- [`709ab6d3e`](https://github.com/dashpay/dash/commit/709ab6d3e) Minimal fix for litemode vs bad-protx-key-not-same issue (#2694)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block (codablock)
- Felix Yan (felixonmars)
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

