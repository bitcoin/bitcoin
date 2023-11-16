# Dash Core version v20.0.0

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

## Downgrade warning

### Downgrade to a version < v19.2.0

Downgrading to a version older than v19.2.0 is not supported due to changes
in the evodb database. If you need to use an older version, you must either
reindex or re-sync the whole chain.

# Notable changes

## ChainLock-based random beacon

To improve long-living masternode quorum (LLMQ) selection, the `v20` hard fork
will enable a new random beacon using ChainLock signatures as described in
[DIP-0029][DIP-0029]. ChainLock signatures will be stored in the Coinbase
transaction to be used as a source of randomness during quorum member
calculations. This change increases the decentralization in the source of
randomness and prevents masternodes or miners from being able to manipulate the
LLMQ process.

## Treasury expansion

Currently, 10% of block rewards are set aside for the Dash DAO treasury which
funds development and other network efforts. Once the `v20` hard fork goes into
effect on mainnet, the treasury system allotment will increase to 20% of block
rewards to align with the proposal approved in September. Miner and masternode
rewards will change to 20% and 60% respectively upon activation of the change,
previous reallocation periods are ignored.

## Asset lock and unlock transactions

The asset lock process provides a mechanism for locking and unlocking Dash in a
credit pool. This credit pool mirrors the available credit supply in Dash
Platform. When a certain amount of Dash is locked, a proportional number of
credits are issued to the identity specified by the transaction issuer. The
identity then uses the credits to pay fees for operations done on Dash
Platform. Masternode identities receive the fees and can convert them back to
Dash by withdrawing them from Dash Platform.

Note: The asset lock feature is activated by the `v20` hard fork while the asset
unlock feature will be activated later by another hard fork.

## Coinbase Transaction Changes

Once v20 activates, Coinbase transactions in all mined blocks must be of
version 3.

Version 3 of the Coinbase transaction will include the following new fields:
- `bestCLSignature` (`BLSSignature`) will hold the best Chainlock signature
  known at the moment
- `bestCLHeightDiff` (`uint32`) is equal to the diff in heights from the mined
  block height
- `creditPoolBalance` (`int64`) is equal to all the duffs locked in all previous
  blocks minus all the duffs unlocked in all previous blocks plus all the block
  subsidy paid to evonodes on Platform.

Although miners are forced to include a version 3 Coinbase transaction, the
actual real-time best Chainlock signature isn't required as long as blocks
contain a `bestCLSignature` equal to (or newer than) the previous block.

Note: Until the first `bestCLSignature` is included in a Coinbase transaction
once v20 activates, null `bestCLSignature` and `bestCLHeightDiff` values are
perfectly valid.

## Testnet Breaking Changes

### Enhanced hard fork

Full implementation of the [DIP-0023][DIP-0023] enhanced hard fork mechanism has
been completed. This improved activation mechanism includes signaling from both
masternodes and miners while also ensuring sufficient upgrade time is available
to partners. This new system is enabled on testnet and devnets by the `v20`
activation and a newly introduced `SPORK_24_TEST_EHF` spork.

### Fixed subsidy base after `v20`

Starting from `v20` activation, the base block subsidy on testnet and devnets
will no longer be calculated based on previous block difficulty. This change
mimics mainnet behaviour. It also simplifies calculations for Platform which
would not need to constantly get block's difficulty in order to calculate
platform reward.

## Sentinel deprecation

Sentinel functionality has been integrated directly into v20.0.0, so it will no
longer be necessary for masternodes to run the standalone Sentinel application.
During the superblock maturity window (1662 blocks), payee masternodes will try
to create, sign and submit a superblock candidate. Several RPC commands have
been updated to prevent conflicts between v20.0.0 and existing Sentinel
installs. It is recommended to remove or disable Sentinel after updating
masternodes to v20.0.0.

## P2P and network changes

### Removal of reject network messages from Dash Core (BIP61)

This feature has been disabled by default since Dash Core version 19.0.0.
Nodes on the network can not generally be trusted to send valid ("reject")
messages, so this should only ever be used when connected to a trusted node.

Since Dash Core version 20.0.0 there are extra changes:

The removal of BIP61 `REJECT` message support also has the following minor RPC
and logging implications:

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P `REJECT`
  code when a transaction is not accepted to the mempool. They still return the
  verbal reject reason.

- Log messages that previously reported the `REJECT` code when a transaction was
  not accepted to the mempool now no longer report the `REJECT` code. The reason
  for rejection is still reported.

### Transaction rebroadcast

The mempool now tracks whether transactions submitted via the wallet or RPCs
have been successfully broadcast. Every 10-15 minutes, the node will try to
announce unbroadcast transactions until a peer requests it via a `getdata`
message or the transaction is removed from the mempool for other reasons.
The node will not track the broadcast status of transactions submitted to the
node using P2P relay. This version reduces the initial broadcast guarantees
for wallet transactions submitted via P2P to a node running the wallet.

To improve wallet privacy, the frequency of wallet rebroadcast attempts is
reduced from approximately once every 15 minutes to once every 1-3 hours.
To maintain a similar level of guarantee for initial broadcast of wallet
transactions, the mempool tracks these transactions as a part of the newly
introduced unbroadcast set.

### `MNLISTDIFF` P2P message

Starting with protocol version `70230`, the following fields are added to the
`MNLISTDIFF` message after `newQuorums`.

| Field | Type | Size | Description |
|-|-|-|-|
| quorumsCLSigsCount | compactSize uint | 1-9 | Number of quorumsCLSigs elements |
| quorumsCLSigs | quorumsCLSigsObject[] | variable | ChainLock signature used to calculate members per quorum indexes (in newQuorums) |

The content of `quorumsCLSigsObject`:

| Field | Type | Size | Description |
|-|-|-|-|
| signature | BLSSig | 96 | ChainLock signature |
| indexSetCount | compactSize uint | 1-9 | Number of quorum indexes using the same `signature` for their member calculation |
| indexSet | uint16_t[] | variable | Quorum indexes corresponding in `newQuorums` using `signature` for their member calculation |

Note: The `quorumsCLSigs` field in both RPC and P2P will only be populated after
the v20 activation.

### Other

- It is possible to run Dash Core as an I2P service and connect to such
  services. Please read [docs][i2p] for more information.
- Removed support for Tor v2, `LEGACYTXLOCKREQUEST` (`"ix"`) message and
`NODE_GETUTXO` (bit 1) node service bit.

## GUI

Add more granular CoinJoin options to let users control the number of CoinJoin
sessions and also the min/max number of denominations for wallets to create
while mixing.

## Remote Procedure Call (RPC) Changes

### The new RPCs are:

- `addpeeraddress` Add the address of a potential peer to the address manager.
  This RPC is for testing only.

- The `getindexinfo` RPC returns the actively running indices of the node,
  including their current sync status and height. It also accepts an
  `index_name` to specify returning only the status of that index.

- `protx listdiff` RPC returns a full deterministic masternode list diff
  between two heights.

### The removed RPCs are:

- `gobject vote-conf` is no longer available.

### Changes in existing RPCs introduced through bitcoin backports:

- The `fundrawtransaction`, `sendmany`, `sendtoaddress`, and
  `walletcreatefundedpsbt` RPC commands have been updated to include two new
  fee estimation methods, `DASH/kB` and `duff/B`. The target is the fee
  expressed explicitly in the given form. In addition, the `estimate_mode`
  parameter is now case insensitive for all of the above RPC commands.

- Soft fork reporting in the `getblockchaininfo` return object has been
  updated. For full details, see the RPC help text. In summary:
  - The `bip9_softforks` sub-object is no longer returned
  - The `softforks` sub-object now returns an object keyed by soft fork name,
    rather than an array
  - Each softfork object in the `softforks` object contains a `type` value
    which is either `buried` (for soft fork deployments where the activation
    height is hard-coded into the client implementation), or `bip9`(for soft
    fork deployments where activation is controlled by BIP 9 signaling).

- `getblocktemplate` no longer returns a `rules` array containing `CSV`
  (the BIP 9 deployments that are currently in active state).

- The `gettransaction` RPC now accepts a third (boolean) argument, `verbose`.
  If set to `true`, a new `decoded` field will be added to the response
  containing the decoded transaction. This field is equivalent to RPC
  `decoderawtransaction`, or RPC `getrawtransaction` when `verbose` is passed.

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P
  `REJECT` code when a transaction is not accepted to the mempool. See the
  Section [_Removal of reject network messages from Dash Core (BIP61)_](#removal-of-reject-network-messages-from-dash-core-bip61) above for
  details on the removal of BIP61 `REJECT` message support.

- A new descriptor type, `sortedmulti(...)`, has been added to support multisig
  scripts where the public keys are sorted lexicographically in the resulting
  script.

- The RPC `gettransaction`, `listtransactions` and `listsinceblock` responses
  now also includes the height of the block that contains the wallet
  transaction, if any.

- RPC `getaddressinfo` changes:
  - the `label` field has been deprecated in favor of the `labels` field and
    will be removed in 0.21. It can be re-enabled in the interim by launching
    with `-deprecatedrpc=label`.
  - the `labels` behavior of returning an array of JSON objects containing name
    and purpose key/value pairs has been deprecated in favor of an array of
    label names and will be removed in 0.21. The previous behavior can be
    re-enabled in the interim by launching with `-deprecatedrpc=labelspurpose`.

- Error codes have been updated to be more accurate for the following error
  cases:
  - `signmessage` now returns `RPC_INVALID_ADDRESS_OR_KEY (-5)` if the
    passed address is invalid. Previously returned `RPC_TYPE_ERROR (-3)`.
  - `verifymessage` now returns `RPC_INVALID_ADDRESS_OR_KEY (-5)` if the
    passed address is invalid. Previously returned `RPC_TYPE_ERROR (-3)`.
  - `verifymessage` now returns `RPC_TYPE_ERROR (-3)` if the passed signature
    is malformed. Previously returned `RPC_INVALID_ADDRESS_OR_KEY (-5)`.

- The `settxfee` RPC will fail if the fee is set higher than the `-maxtxfee`
  command line setting. The wallet will already fail to create transactions
  with fees higher than `-maxtxfee`.

- `getmempoolinfo` now returns an additional `unbroadcastcount` field. The
  mempool tracks locally submitted transactions until their initial broadcast
  is acknowledged by a peer. This field returns the count of transactions
  waiting for acknowledgement.

- Mempool RPCs such as `getmempoolentry` and `getrawmempool` with
  `verbose=true` now return an additional `unbroadcast` field. This indicates
  whether initial broadcast of the transaction has been acknowledged by a
  peer. `getmempoolancestors` and `getmempooldescendants` are also updated.

- `logging` and `debug` rpc support new logging categories `validations` and
  `i2p`

- `gettxoutsetinfo` RPC has 2 new parameters and lots of new fields in
  results. Please check `help gettxoutsetinfo` for more information.

- `gettxchainlocks` Returns the block height each transaction was mined at and
  whether it is chainlocked or not.

### Dash-specific changes in existing RPCs:

- `getblockchaininfo` RPC returns the field `activation_height` for each
  softfork in `locked_in` status indicating the expected activation height.

- `getblockchaininfo` RPC returns the field `ehf` and `ehf_height` for each
  softfork. `ehf_height` indicates the minimum height when miner's signals can
  be accepted for the deployment with `ehf` set to `true`.

- `getgovernanceinfo` RPC returns two new fields:
  - `governancebudget` (the governance budget for the next superblock)
  - `fundingthreshold` (the number of absolute yes votes required for a proposal
    to be passing)

- `gobject submit` will no longer accept submission of triggers.

- `protx diff` RPC returns a new field `quorumsCLSigs`. This field is a list
  containing: a ChainLock signature and the list of corresponding quorum
  indexes in `newQuorums`.

- `protx list`: New type `evo` filters only HPMNs.

- `*_hpmn` RPCs were renamed:
  - `protx register_hpmn` to `protx register_evo`
  - `protx register_fund_hpmn` to `protx register_fund_evo`
  - `protx register_prepare_hpmn` to `protx register_prepare_evo`
  - `protx update_service_hpmn` to `protx update_service_evo`

  Please note that `*_hpmn` RPCs are not fully removed; instead they are
  deprecated so you can still enable them by using the `-deprecatedrpc=hpmn`
  option.

- `masternode count` returns the total number of evonodes under field `evo`
  instead of `hpmn`.

- `masternodelist`: New mode `evo` filters only HPMNs.

- Description type string `HighPerformance` is renamed to `Evo`: affects most
  RPCs return MNs details.

Please check `help <command>` for more detailed information on specific RPCs.

## Command-line options

### New cmd-line options:

- `-generate` Generate blocks immediately, equivalent to RPC generatenewaddress
  followed by RPC generatetoaddress. Optional positional integer arguments are
  number of blocks to generate (default: 1) and maximum iterations to try
  (default: 1000000), equivalent to RPC generatetoaddress nblocks and maxtries
  arguments.

- `-netinfo` Get network peer connection information from the remote server.
  An optional integer argument from 0 to 4 can be passed for different peers
  listings (default: 0). Pass `help` for detailed help documentation.

- `-coinstatsindex` Maintain coinstats index used by the gettxoutset RPC
  (default: false)

- `-fastprune` Use smaller block files and lower minimum prune height for
  testing purposes

- `-i2psam`=<ip:port> I2P SAM proxy to reach I2P peers and accept I2P
  connections (default: none)

- `-i2pacceptincoming` If set and `-i2psam` is also set then incoming I2P
  connections are accepted via the SAM proxy. If this is not set but `-i2psam`
  is set then only outgoing connections will be made to the I2P network. Ignored
  if `-i2psam` is not set. Listening for incoming I2P connections is done
  through the SAM proxy, not by binding to a local address and port (default: 1)

- `-llmqmnhf` Override the default LLMQ type used for EHF. (default:
  llmq_devnet, devnet-only)

- `-llmqtestinstantsend` Override the default LLMQ type used for InstantSend.
  Used mainly to test Platform. (default: llmq_test_instantsend, regtest-only)

- `-llmqtestinstantsenddip0024` Override the default LLMQ type used for
  InstantSendDIP0024. Used mainly to test Platform. (default:
  llmq_test_dip0024, regtest-only)

- `-mnemonicbits` User defined mnemonic security for HD wallet in bits (BIP39).
  Only has effect during wallet creation/first start (allowed values: 128, 160,
  192, 224, 256; default: 128)

- `-chainlocknotify=<cmd>` Execute command when the best chainlock changes
  `%s` in cmd is replaced by chainlocked block hash.

### Removed cmd-line options:

- `-dropmessagestest`

- `-upgradewallet` replaced by `upgradewallet` rpc

### Changes in existing cmd-line options:

- `-fallbackfee` was 0 (disabled) by default for the main chain, but 1000 by
  default for the test chains. Now it is 0 by default for all chains. Testnet
  and regtest users will have to add fallbackfee=1000 to their configuration if
  they weren't setting it and they want it to keep working like before.

- The `dash-cli -getinfo` command now displays the wallet name and balance for
  each of the loaded wallets when more than one is loaded (e.g. in multiwallet
  mode) and a wallet is not specified with `-rpcwallet`.

- A `download` permission has been extracted from the `noban` permission. For
  compatibility, `noban` implies the `download` permission, but this may change
  in future releases. Refer to the help of the affected settings `-whitebind`
  and `-whitelist` for more details.

- `-vbparams` accepts `useehf`.

- `-instantsendnotify=<cmd>` `%w` is replaced by wallet name.

Please check `Help -> Command-line options` in Qt wallet or `dashd --help` for
more information.

## JSON-RPC

All JSON-RPC methods accept a new [named parameter][json-rpc] called `args` that can
contain positional parameter values. This is a convenience to allow some
parameter values to be passed by name without having to name every value. The
python test framework and `dash-cli` tool both take advantage of this, so
for example:

```sh
dash-cli -named createwallet wallet_name=mywallet load_on_startup=1
```

Can now be shortened to:

```sh
dash-cli -named createwallet mywallet load_on_startup=1
```

## Switch from Gitian to Guix

Starting from v20.0.0 deterministic builds are going to be produced using the
Guix build system. See [contrib/guix/README.md][guix-readme]. The previously
used
Gitian build system is no longer supported and is not guaranteed to produce any
meaningful results.

## Backports from Bitcoin Core

This release introduces many updates from Bitcoin  v0.20-v22.0 as well as
numerous updates from Bitcoin v23.0-v25.0. Bitcoin changes that do not align
with Dash’s product needs, such as SegWit and RBF, are excluded from our
backporting. For additional detail on what’s included in Bitcoin, please refer
to their release notes.

# v20.0.0 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Ivan Shumkov (shumkov)
- Kittywhiskers Van Gogh (kittywhiskers)
- Konstantin Akimov (knst)
- Munkybooty
- Odysseas Gabrielides (ogabrielides)
- PanderMusubi (Pander)
- PastaPastaPasta
- strophy
- thephez
- UdjinM6
- Vijay Das Manikpuri (vijaydasmp)

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

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

- [v19.3.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.3.0.md) released July/31/2023
- [v19.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.2.0.md) released June/19/2023
- [v19.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.1.0.md) released May/22/2023
- [v19.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.0.0.md) released Apr/14/2023
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

[DIP-0023]: https://github.com/dashpay/dips/blob/master/dip-0023.md
[DIP-0029]: https://github.com/dashpay/dips/blob/master/dip-0029.md

[i2p]: https://github.com/dashpay/dash/blob/v20.x/doc/i2p.md
[guix-readme]: https://github.com/dashpay/dash/blob/v20.x/contrib/guix/README.md
[json-rpc]: https://github.com/dashpay/dash/blob/v20.x/doc/JSON-RPC-interface.md#parameter-passing
[set-of-changes]: https://github.com/dashpay/dash/compare/v19.3.0...dashpay:v20.0.0
