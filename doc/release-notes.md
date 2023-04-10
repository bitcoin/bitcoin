# Dash Core version v19.0.0

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes
and other improvements.

This release is mandatory for all nodes.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>


# Upgrading and downgrading

## How to Upgrade

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

## Downgrade warning

### Downgrade to a version < v19.0.0
Downgrading to a version older than v19.0.0 is not supported due to changes in the evodb database. If you need to use an older version, you must either reindex or re-sync the whole chain.

# Notable changes

## High-Performance Masternodes

In preparation for the release of Dash Platform to mainnet, a new masternode type has been added. High-performance masternodes will be responsible for hosting Dash Platform services (once they are on mainnet) in addition to the existing responsibilities like ChainLocks and InstantSend.

Activation of the DashCore v19.0 hard fork will enable registration of the new 4000 DASH collateral masternodes. Until Dash Platform is released to mainnet, high-performance masternodes will provide the same services as regular masternodes with one small exception. Regular masternodes will no longer participate in the Platform-specific LLMQ after the hard fork since they will not be responsible for hosting Dash Platform.

Note: In DashCore v19.0 the relative rewards and voting power are equivalent between regular and high-performance masternodes. Masternodes effectively receive one payout and one governance vote per 1000 DASH collateral. So, there is no difference in reward amount for running four regular masternodes or one high-performance masternode. In v19.0, high-performance masternodes simply receive payments in four consecutive blocks when they are selected for payout. Some frequently asked questions may be found at https://www.dash.org/hpmn-faq/.

## BLS Scheme Upgrade

Once the v19 hard fork is activated, all remaining messages containing BLS public keys or signatures will serialise them using the new basic BLS scheme.
The motivation behind this change is the need to be aligned with IETF standards.

List of affected messages:
`dsq`, `dstx`, `mnauth`, `govobj`, `govobjvote`, `qrinfo`, `qsigshare`, `qsigrec`, `isdlock`, `clsig`, and all DKG messages (`qfcommit`, `qcontrib`, `qcomplaint`, `qjustify`, `qpcommit`).

### `qfcommit`

Once the v19 hard fork is activated, `quorumPublicKey` will be serialised using the basic BLS scheme.
To support syncing of older blocks containing the transactions using the legacy BLS scheme, the `version` field indicates which scheme to use for serialisation of `quorumPublicKey`.

| Version | Version Description                                    | Includes `quorumIndex` field |
|---------|--------------------------------------------------------|------------------------------|
| 1       | Non-rotated qfcommit serialised using legacy BLS scheme | No                           |
| 2       | Rotated qfcommit serialised using legacy BLS scheme     | Yes                          |
| 3       | Non-rotated qfcommit serialised using basic BLS scheme  | No                           |
| 4       | Rotated qfcommit serialised using basic BLS scheme      | Yes                          |

### `MNLISTDIFF` P2P message

Starting with protocol version 70225, the following field is added to the `MNLISTDIFF` message between `cbTx` and `deletedQuorumsCount`.

| Field               | Type | Size | Description                       |
|---------------------| ---- | ---- |-----------------------------------|
| version             | uint16_t | 2 | Version of the `MNLISTDIFF` reply |

The `version` field indicates which BLS scheme is used to serialise the `pubKeyOperator` field for all SML entries of `mnList`.

| Version | Version Description                                       |
|---------|-----------------------------------------------------------|
| 1       | Serialisation of `pubKeyOperator` using legacy BLS scheme |
| 2       | Serialisation of `pubKeyOperator` using basic BLS scheme  |

### `ProTx` txs family

`proregtx` and `proupregtx` will support a new `version` value:

| Version | Version Description                                       |
|---------|-----------------------------------------------------------|
| 1       | Serialisation of `pubKeyOperator` using legacy BLS scheme |
| 2       | Serialisation of `pubKeyOperator` using basic BLS scheme  |

`proupservtx` and `prouprevtx` will support a new `version` value:

| Version | Version Description                            |
|---------|------------------------------------------------|
| 1       | Serialisation of `sig` using legacy BLS scheme |
| 2       | Serialisation of `sig` using basic BLS scheme  |

### `MNHFTx`

`MNHFTx` will support a new `version` value:

| Version | Version Description                            |
|---------|------------------------------------------------|
| 1       | Serialisation of `sig` using legacy BLS scheme |
| 2       | Serialisation of `sig` using basic BLS scheme  |

## Wallet

## Automatic wallet creation removed

Dash Core will no longer automatically create new wallets on startup. It will
load existing wallets specified by -wallet options on the command line or in
dash.conf or settings.json files. And by default it will also load a
top-level unnamed ("") wallet. However, if specified wallets don't exist,
Dash Core will now just log warnings instead of creating new wallets with
new keys and addresses like previous releases did.

New wallets can be created through the GUI (which has a more prominent create
wallet option), through the dash-wallet create command or the createwallet RPC.

## P2P and Network Changes

## Removal of reject network messages from Dash Core (BIP61)

The command line option to enable BIP61 (-enablebip61) has been removed.

Nodes on the network can not generally be trusted to send valid ("reject")
messages, so this should only ever be used when connected to a trusted node.
Please use the recommended alternatives if you rely on this deprecated feature:

- Testing or debugging of implementations of the Dash P2P network protocol
should be done by inspecting the log messages that are produced by a recent
version of Dash Core. Dash Core logs debug messages
(-debug=<category>) to a stream (-printtoconsole) or to a file
(-debuglogfile=<debug.log>).

- Testing the validity of a block can be achieved by specific RPCs:
    - submitblock
    - getblocktemplate with 'mode' set to 'proposal' for blocks with
    - potentially invalid POW
    - Testing the validity of a transaction can be achieved by specific RPCs:
      - sendrawtransaction
      - testmempoolaccept

## CoinJoin update

A minor update in several CoinJoin-related network messages improves support
for mixing from SPV clients. These changes make it easier for SPV clients to
participate in the CoinJoin process by using masternode information they can
readily obtain and verify via [DIP-0004](https://github.com/dashpay/dips/blob/master/dip-0004.md).

## Remote Procedure Call (RPC) Changes

### The new RPCs are:
- In order to support BLS public keys encoded in the legacy BLS scheme, `protx register_legacy`, `protx register_fund_legacy`, and `protx register_prepare_legacy` were added.
- `cleardiscouraged` clears all the already discouraged peers.
- The following RPCs were added: `protx register_hpmn`, `protx register_fund_hpmn`, `protx register_prepare_hpmn` and `protx update_service_hpmn`.
  These HPMN RPCs correspond to the standard masternode RPCs but have the following additional mandatory arguments: `platformNodeID`, `platformP2PPort` and `platformHTTPPort`.
- `upgradewallet`

### The removed RPCs are:

None

### Changes in existing RPCs introduced through bitcoin backports:

- The `utxoupdatepsbt` RPC method has been updated to take a descriptors
argument. When provided, input and output scripts and keys will be filled in
when known. See the RPC help text for full details.


### Dash-specific changes in existing RPCs:

- `masternodelist`: New mode `recent` was added in order to hide banned masternodes for more than one `SuperblockCycle`. If the mode `recent` is used, then the reply mode is JSON (can be additionally filtered)
- `quorum info`: The new `previousConsecutiveDKGFailures` field will be returned for rotated LLMQs. This field will hold the number of previous consecutive DKG failures for the corresponding quorumIndex before the currently active one. Note: If no previous commitments were found then 0 will be returned for `previousConsecutiveDKGFailures`.
- `bls generate` and `bls fromsecret`: The new `scheme` field will be returned indicating which scheme was used to serialise the public key. Valid returned values are `legacy` and`basic`.
- `bls generate` and `bls fromsecret`: Both RPCs accept an incoming optional boolean argument `legacy` that enforces the use of legacy BLS scheme for the serialisation of the reply even if v19 is active.
- `masternode status`: now returns the type of the masternode.
- `masternode count`: now returns a detailed summary of total and enabled masternodes per type.
- `gobject getcurrentvotes`: reply is enriched by adding the vote weight at the end of each line. Possible values are 1 or 4. Example: "7cb20c883c6093b8489f795b3ec0aad0d9c2c2821610ae9ed938baaf42fec66d": "277e6345359071410ab691c21a3a16f8f46c9229c2f8ec8f028c9a95c0f1c0e7-1:1670019339:yes:funding:4"
- Once the v19 hard fork is activated, `protx register`, `protx register_fund`, and `protx register_prepare` RPCs will decode BLS operator public keys using the new basic BLS scheme.

Please check `help <command>` for more detailed information on specific RPCs.

## Command-line options

A number of command-line option changes were made related to testing and
removal of BIP61 support.

New cmd-line options:
- `llmqplatform` (devnet only)
- `unsafesqlitesync`

Removed cmd-line options:
- `enablebip61`
- `upgradewallet`

Changes in existing cmd-line options:
- `llmqinstantsend` and `llmqinstantsenddip0024` can be used in regtest now
- Passing an invalid `-rpcauth` argument now cause dashd to fail to start.

Please check `Help -> Command-line options` in Qt wallet or `dashd --help` for
more information.

## Backports from Bitcoin Core

This release introduces many updates from Bitcoin v0.18-v0.21 as well as numerous updates from Bitcoin v22 and more recent versions. Bitcoin changes that do not align with Dash’s product needs, such as SegWit and RBF, are excluded from our backporting. For additional detail on what’s included in Bitcoin, please refer to their release notes.

# v19.0.0 Change log

See detailed [set of changes](https://github.com/dashpay/dash/compare/v18.2.2...dashpay:v19.0.0).

# Credits

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh (kittywhiskers)
- Konstantin Akimov (knst)
- Odysseas Gabrielides (ogabrielides)
- Oleg Girko (OlegGirko)
- PastaPastaPasta
- thephez
- UdjinM6
- Vijay Das Manikpuri (vijaydasmp)

As well as everyone that submitted issues, reviewed pull requests, helped debug the release candidates, and write DIPs that were implemented in this release.

# Older releases

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

- [v18.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.2.md) released Mar/21/2023
- [v18.2.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.1.md) released Jan/17/2023
- [v18.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.0.md) released Jan/01/2023
- [v18.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.1.md) released January/08/2023
- [v18.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.0.md) released October/09/2022
- [v18.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.2.md) released October/09/2022
- [v18.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.1.md) released August/17/2022
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
