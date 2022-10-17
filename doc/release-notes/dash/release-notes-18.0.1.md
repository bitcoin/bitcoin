Dash Core version v18.0.1
=========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes
and other improvements.

This release is mandatory for all nodes.

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
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

When upgrading from a version prior to 18.0.1, the
first startup of Dash Core will run a migration process which can take anywhere
from a few minutes to thirty minutes to finish. After the migration, a
downgrade to an older version is only possible with a reindex
(or reindex-chainstate).

Downgrade warning
-----------------

### Downgrade to a version < v18.0.1

Downgrading to a version older than v18.0.1 is not supported due to changes in
the indexes database folder. If you need to use an older version, you must
either reindex or re-sync the whole chain.

### Downgrade of masternodes to < v18.0.1

Starting with the 0.16 release, masternodes verify the protocol version of other
masternodes. This results in PoSe punishment/banning for outdated masternodes,
so downgrading even prior to the activation of the introduced hard-fork changes
is not recommended.

Notable changes
===============

Quorum rotation
--------------
InstantSend quorums will now use a new quorum type and a new algorithm for
establishing quorums. The upcoming DIP-0024 will provide comprehensive details.

Quorum rotation is activated via a BIP9 style hard fork that will begin
signalling on August 15, 2022 using bit 7. New quorums will start forming in
1152-1440 block range after the activation. Any nodes that do not upgrade by
that time will diverge from the rest of the network.

Deterministic InstantSend
-------------------------
Deterministically verifying InstantSend locks at any point in the future has
been added to support Dash Platform. This update introduces versioning to
InstantSend messages and adds quorum information to them. While the previous
design was sufficient for core chain payments, the platform chain will benefit
from this enhanced verification capability. Details about deterministic
InstantSend are provided in [DIP-0022](https://github.com/dashpay/dips/blob/master/dip-0022.md).

Deterministic InstantSend will be activated with the DIP0024 hard fork.

Governance
----------
Several improvements have been made to Dash’s DAO governance system.
The governance proposal fee has been reduced from 5 Dash to 1 Dash following
a vote by masternode owners to do so. For improved security and flexibility,
proposal payouts to pay-to-script-hash (P2SH) addresses are now supported.

These changes will be activated with the DIP0024 hard fork.

Governance proposals can now be viewed in GUI Governance tab (must be enabled
in Preferences first).

Initial Enhanced Hard Fork support
----------------------------------
The masternode hard fork signal special transaction has been added as the first
step in enabling an improved hard fork mechanism. This enhancement enables
future hard forks to be activated quickly and safely without any
“race conditions” if miners and masternodes update at significantly different
speeds. Effectively there will be a masternode signal on chain in addition to
the miner one to ensure smooth transitions. Details of the enhanced hard fork
system are provided in [DIP-0023](https://github.com/dashpay/dips/blob/master/dip-0023.md).

Network improvements
--------------------
We implemented and backported implementations of several improvement proposals.
You can read more about implemented changes in the following documents:
- [`DIP-0025`](https://gist.github.com/thephez/6c4c2a7747298e8b3e528c0c4e98a68c): Compressed headers.
- [`BIP 155`](https://github.com/bitcoin/bips/blob/master/bip-0155.mediawiki): The 'addrv2' and 'sendaddrv2' messages which enable relay of Tor V3 addresses (and other networks).
- [`BIP 158`](https://github.com/bitcoin/bips/blob/master/bip-0158.mediawiki): Compact Block Filters for Light Clients.

KeePass support removed
-----------------------
Please make sure to move your coins to a wallet with a regular passphrase.

Wallet changes
--------------
We continued backporting wallet functionality updates. Most notable changes
are:
- Added support for empty, encrypted-on-creation and watch-only wallets.
- Wallets can now be created, opened and closed via a GUI menu.
- No more `salvagewallet` option in cmd-line and Repair tab in GUI. Check the
`salvage` command in the `dash-wallet` tool.

Indexes
-------
The transaction index is moved into `indexes/` folder. The migration of the old
data is done on the first run and does not require reindexing. Note that the data
in the old path is removed which means that this change is not backwards
compatible and you'll have to reindex the whole blockchain if you decide to
downgrade to a pre-v18.0.1 version.

Remote Procedure Call (RPC) Changes
-----------------------------------
Most changes here were introduced through Bitcoin backports mostly related to
the deprecation of wallet accounts in DashCore v0.17 and introduction of PSBT
format.

The new RPCs are:
- `combinepsbt`
- `converttopsbt`
- `createpsbt`
- `decodepsbt`
- `deriveaddresses`
- `finalizepsbt`
- `getblockfilter`
- `getdescriptorinfo`
- `getnodeaddresses`
- `getrpcinfo`
- `joinpsbts`
- `listwalletdir`
- `quorum rotationinfo`
- `scantxoutset`
- `submitheader`
- `testmempoolaccept`
- `utxoupdatepsbt`
- `walletcreatefundedpsbt`
- `walletprocesspsbt`

The removed RPCs are:
- `estimatefee`
- `getinfo`
- `getreceivedbyaccount`
- `keepass`
- `listaccounts`
- `listreceivedbyaccount`
- `move`
- `resendwallettransactions`
- `sendfrom`
- `signrawtransaction`

Changes in existing RPCs introduced through bitcoin backports:
- The `testnet` field in `dash-cli -getinfo` has been renamed to `chain` and
now returns the current network name as defined in BIP70 (main, test, regtest).
- Added `window_final_block_height` in `getchaintxstats`
- Added `feerate_percentiles` object with feerates at the 10th, 25th, 50th,
75th, and 90th percentile weight unit instead of `medianfeerate` in
`getblockstats`
- In `getmempoolancestors`, `getmempooldescendants`, `getmempoolentry` and
`getrawmempool` RPCs, to be consistent with the returned value and other RPCs
such as `getrawtransaction`, `vsize` has been added and `size` is now
deprecated. `size` will only be returned if `dashd` is started with
`-deprecatedrpc=size`.
- Added `loaded` in mempool related RPCs indicates whether the mempool is fully
loaded or not
- Added `localservicesnames` in `getnetworkinfo` list the services the node
offers to the network, in human-readable form (in addition to an already
existing `localservices` hex string)
- Added `hwm` in `getzmqnotifications`
- `createwallet` can create blank, encrypted or watch-only wallets now.
- Added `private_keys_enabled` in `getwalletinfo`
- Added `solvable`, `desc`, `ischange` and `hdmasterfingerprint` in `getaddressinfo`
- Added `desc` in `listunspent`

Dash-specific changes in existing RPCs:
- Added `quorumIndex` in `quorum getinfo` and `quorum memberof`
- In rpc `masternodelist` with parameters `full`, `info` and `json` the PoS penalty score of the MN will be returned. For `json` parameter, the field `pospenaltyscore` was added.

Please check `help <command>` for more detailed information on specific RPCs.

Command-line options
--------------------
Most changes here were introduced through Bitcoin backports.

New cmd-line options:
- `asmap`
- `avoidpartialspends`
- `blockfilterindex`
- `blocksonly`
- `llmqinstantsenddip0024`
- `llmqtestinstantsendparams`
- `maxuploadtarget`
- `natpmp`
- `peerblockfilters`
- `powtargetspacing`
- `stdinwalletpassphrase`
- `zmqpubhashchainlock`
- `zmqpubrawchainlock`

The option to set the PUB socket's outbound message high water mark
(SNDHWM) may be set individually for each notification:
- `-zmqpubhashtxhwm=n`
- `-zmqpubhashblockhwm=n`
- `-zmqpubhashchainlockhwm=n`
- `-zmqpubhashtxlockhwm=n`
- `-zmqpubhashgovernancevotehwm=n`
- `-zmqpubhashgovernanceobjecthwm=n`
- `-zmqpubhashinstantsenddoublespendhwm=n`
- `-zmqpubhashrecoveredsighwm=n`
- `-zmqpubrawblockhwm=n`
- `-zmqpubrawtxhwm=n`
- `-zmqpubrawchainlockhwm=n`
- `-zmqpubrawchainlocksighwm=n`
- `-zmqpubrawtxlockhwm=n`
- `-zmqpubrawtxlocksighwm=n`
- `-zmqpubrawgovernancevotehwm=n`
- `-zmqpubrawgovernanceobjecthwm=n`
- `-zmqpubrawinstantsenddoublespendhwm=n`
- `-zmqpubrawrecoveredsighwm=n`

Removed cmd-line options:
- `keepass`
- `keepassport`
- `keepasskey`
- `keepassid`
- `keepassname`
- `salvagewallet`

Changes in existing cmd-line options:

Please check `Help -> Command-line options` in Qt wallet or `dashd --help` for
more information.

Backports from Bitcoin Core
---------------------------
This release introduces over 1000 updates from Bitcoin v0.18/v0.19/v0.20 as well as numerous updates from Bitcoin v0.21 and more recent versions. This includes multi-wallet support in the GUI, support for partially signed transactions (PSBT), Tor version 3 support, and a number of other updates that will benefit Dash users. Bitcoin changes that do not align with Dash’s product needs, such as SegWit and RBF, are excluded from our backporting. For additional detail on what’s included in Bitcoin, please refer to their release notes – v0.18, v0.19, v0.20.

Miscellaneous
-------------
A lot of refactoring, code cleanups and other small fixes were done in this release.

v18.0.1 Change log
==================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.17.0.3...dashpay:v18.0.1).

Credits
=======

Thanks to everyone who directly contributed to this release:

- AJ ONeal (coolaj86)
- Christian Fifi Culp (christiancfifi)
- dustinface (xdustinface)
- gabriel-bjg
- Holger Schinzel (schinzelh)
- humbleDasher
- Kittywhiskers Van Gogh (kittywhiskers)
- Konstantin Akimov (knst)
- ktechmidas
- linuxsh2
- Munkybooty
- Nathan Marley (nmarley)
- Odysseas Gabrielides (ogabrielides)
- PastaPastaPasta
- pravblockc
- rkarthik2k21
- Stefan (5tefan)
- strophy
- TheLazieR Yip (thelazier)
- thephez
- UdjinM6
- Vijay (vijaydasmp)
- Vlad K (dzutto)

As well as everyone that submitted issues, reviewed pull requests, helped debug the release candidates, and write DIPs that were implemented in this release. Notable mentions include:

- Samuel Westrich (quantumexplorer)
- Virgile Bartolo
- xkcd

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

- [v0.17.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.3.md) released June/07/2021
- [v0.17.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.2.md) released May/19/2021
- [v0.16.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.1.md) released November/17/2020
- [v0.16.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.0.md) released November/14/2020
- [v0.16.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.0.1.md) released September/30/2020
- [v0.15.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.15.0.0.md) released Febrary/18/2020
- [v0.14.0.5](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.5.md) released December/08/2019
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
