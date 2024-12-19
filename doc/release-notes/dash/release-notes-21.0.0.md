# Dash Core version v21.0.0

This is a new major version release, bringing new features, various bugfixes
and other improvements.

This release is **mandatory** for all masternodes.
This release is optional but recommended for all other nodes.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v21.0.0

Downgrading to a version older than v21.0.0 may not be supported due to changes
if you are using descriptor wallets.

### Downgrade to a version < v19.2.0

Downgrading to a version older than v19.2.0 is not supported due to changes
in the evodb database. If you need to use an older version, you must either
reindex or re-sync the whole chain.

# Notable changes

Masternode Reward Location Reallocation (MN_RR) Hard Fork
---------------------------------------------------------
The MN_RR hard fork, first included in Dash Core v20, will be activated after v21 is adopted by masternodes. This
hard fork enables the major feature included in this release: Masternode Reward Location Reallocation. The activation
will also initiate the launch of the Dash Platform Genesis Chain.

As the MN_RR hard fork is an [Enhanced Hard Fork](https://github.com/dashpay/dips/blob/master/dip-0023.md),
activation is dependent on both masternodes and miners. However, as this hard fork was first implemented in v20, only
masternodes are required to upgrade for the hard fork to activate. Once 85% of masternodes have upgraded to v21,
an EHF message will be signed by the `LLMQ_400_85` quorum, and mined into a block. This signature serves as proof that 85% of
active masternodes have upgraded.

Once the EHF signed message is mined and a cycle ends, the hard fork status will move from `defined` to `started`. At this point,
miners on v20 and v21 will begin to signal for activation of MN_RR. As such, nearly 100% of miners will
likely be signalling. If a sufficient number of mined blocks signal support for activation (starting at 80% and gradually decreasing to 60%)
by the end of a cycle, the hard fork will change status from `started` to `locked_in`. At this point, hard fork activation
will occur at the end of the next cycle.

Now that the hard fork is active, the new rules will be in effect.

Masternode Reward Location Reallocation
---------------------------------------

Once the MN_RR hard fork activates, some coinbase funds will be moved into the Credit Pool (i.e., to Platform) each time a block is mined.
Evonodes will then receive a single reward per payment cycle on the Core chain - not rewards from four sequential blocks
as in v19/v20. The remainder of evonode payments will be distributed by Platform from the credit pool. This is to
incentivize evonodes to upgrade to Platform because only nodes running Platform can receive these reward payments.


Mainnet Spork Hardening
-----------------------

This version hardens all sporks on mainnet. Sporks remain in effect on all devnets and testnet; however, on mainnet,
the value of all sporks are hard coded to 0, or 1 for the `SPORK_21_QUORUM_ALL_CONNECTED` spork. These hardened values match the
active values historically used on mainnet, so there is no change in the network's functionality.

Default Branch Changed
----------------------

The Dash Core repository (`github.com/dashpay/dash`) will now use `develop` as the default branch. New clones
of the repository will no longer default to the `master` branch. If you want to continue using a stable version of
Dash Core, you should manually checkout the `master` branch.

# Additional Changes

Wallet
------

### Avoid Partial Spends

The wallet will now avoid partial spends (APS) by default if this does not result
in a difference in fees compared to the non-APS variant. The allowed fee threshold
can be adjusted using the new `-maxapsfee` configuration option. (dash#5930)

### Experimental Descriptor Wallets

Please note that Descriptor Wallets are still experimental and not all expected functionality
is available. Additionally there may be some bugs and current functions may change in the future.
Bugs and missing functionality can be reported to the [issue tracker](https://github.com/dashpay/dash/issues).

v21 introduces a new experimental type of wallet - Descriptor Wallets. Descriptor Wallets store
scriptPubKey information using descriptors. This is in contrast to the Legacy Wallet
structure where keys are used to generate scriptPubKeys and addresses. Because of this
shift to being script-based instead of key-based, many of the confusing things that Legacy
Wallets do are not possible with Descriptor Wallets. Descriptor Wallets use a definition
of "mine" for scripts which is simpler and more intuitive than that used by Legacy Wallets.
Descriptor Wallets also uses different semantics for watch-only things and imports.

As Descriptor Wallets are a new type of wallet, their introduction does not affect existing wallets.
Users who already have a Dash Core wallet can continue to use it as they did before without
any change in behavior. Newly created Legacy Wallets (which is the default type of wallet) will
behave as they did in previous versions of Dash Core.

The differences between Descriptor Wallets and Legacy Wallets are largely limited to non user facing
things. They are intended to behave similarly except for the import/export and watchonly functionality
as described below.

#### Creating Descriptor Wallets

Descriptor Wallets are not created by default. They must be explicitly created using the
`createwallet` RPC or via the GUI. A `descriptors` option has been added to `createwallet`.
Setting `descriptors` to `true` will create a Descriptor Wallet instead of a Legacy Wallet.

In the GUI, a checkbox has been added to the Create Wallet Dialog to indicate that a
Descriptor Wallet should be created.

A Legacy Wallet will be created if those options have not been set.


#### `IsMine` Semantics

`IsMine` refers to the function used to determine whether a script belongs to the wallet.
This is used to determine whether an output belongs to the wallet. `IsMine` in Legacy Wallets
returns true if the wallet would be able to sign an input that spends an output with that script.
Since keys can be involved in a variety of different scripts, this definition for `IsMine` can
lead to many unexpected scripts being considered part of the wallet.

With Descriptor Wallets, descriptors explicitly specify the set of scripts that are owned by
the wallet. Since descriptors are deterministic and easily enumerable, users will know exactly
what scripts the wallet will consider to belong to it. Additionally the implementation of `IsMine`
in Descriptor Wallets is far simpler than for Legacy Wallets. Notably, in Legacy Wallets, `IsMine`
allowed for users to take one type of address (e.g. P2PKH), mutate it into another address type
and the wallet would still detect outputs sending to the new address type
even without that address being requested from the wallet. Descriptor Wallets do not
allow for this and will only watch for the addresses that were explicitly requested from the wallet.

These changes to `IsMine` will make it easier to understand what scripts the wallet will
actually be watching for in outputs. However, for the vast majority of users, this change is
transparent and will not have noticeable effects.

#### Imports and Exports

In Legacy Wallets, raw scripts and keys could be imported to the wallet. Those imported scripts
and keys are treated separately from the keys generated by the wallet. This complicates the `IsMine`
logic as it has to distinguish between spendable and watchonly.

Descriptor Wallets handle importing scripts and keys differently. Only complete descriptors can be
imported. These descriptors are then added to the wallet as if it were a descriptor generated by
the wallet itself. This simplifies the `IsMine` logic so that it no longer has to distinguish
between spendable and watchonly. As such, the watchonly model for Descriptor Wallets is also
different and described in more detail in the next section.

To import into a Descriptor Wallet, a new `importdescriptors` RPC has been added that uses a syntax
similar to that of `importmulti`.

As Legacy Wallets and Descriptor Wallets use different mechanisms for storing and importing scripts and keys,
the existing import RPCs have been disabled for descriptor wallets.
New export RPCs for Descriptor Wallets have not yet been added.

The following RPCs are disabled for Descriptor Wallets:

* importprivkey
* importpubkey
* importaddress
* importwallet
* importelectrumwallet
* dumpprivkey
* dumpwallet
* dumphdinfo
* importmulti
* addmultisigaddress

#### Watchonly Wallets

A Legacy Wallet contains both private keys and scripts that were being watched.
Those watched scripts would not contribute to your normal balance. In order to see the watchonly
balance and to use watchonly things in transactions, an `include_watchonly` option was added
to many RPCs that would allow users to do that. However, it is easy to forget to include this option.

Descriptor Wallets move to a per-wallet watchonly model. An entire wallet is considered to be
watchonly depending on whether or not it was created with private keys disabled. This eliminates the need
to distinguish between things that are watchonly and things that are not within a wallet.

This change does have a caveat. If a Descriptor Wallet with private keys *enabled* has
a multiple key descriptor without all of the private keys (e.g. `multi(...)` with only one private key),
then the wallet will fail to sign and broadcast transactions. Such wallets would need to use the PSBT
workflow but the typical GUI Send, `sendtoaddress`, etc. workflows would still be available, just
non-functional.

This issue is worse if the wallet contains both single key (e.g. `pkh(...)`) descriptors and such
multiple key descriptors. In this case some transactions could be signed and broadcast while others fail. This is
due to some transactions containing only single key inputs while others contain both single
key and multiple key inputs, depending on which are available and how the coin selection algorithm
selects inputs. However, this is not a supported use case; multisigs
should be in their own wallets which do not already have descriptors. Although users cannot export
descriptors with private keys for now as explained earlier.

Configuration
-------------

### New cmd-line options
- `-networkactive=` Enable all P2P network activity (default: 1). Can be changed by the `setnetworkactive` RPC command.
- A new configuration flag `-maxapsfee` has been added, which sets the max allowed
  avoid partial spends (APS) fee. It defaults to 0 (i.e. fee is the same with
  and without APS). Setting it to -1 will disable APS, unless `-avoidpartialspends`
  is set. (dash#5930)

### Updated cmd-line options
- The `-debug=db` logging category, which was deprecated in v0.18 and replaced by
  `-debug=walletdb` to distinguish it from `coindb`, has been removed. (#6033)
- The `-banscore` configuration option, which modified the default threshold for
  disconnecting and discouraging misbehaving peers, has been removed as part of
  changes in this release to the handling of misbehaving peers. (dash#5511)
- If `-proxy=` is given together with `-noonion` then the provided proxy will
  not be set as a proxy for reaching the Tor network. So it will not be
  possible to open manual connections to the Tor network, for example, with the
  `addnode` RPC. To mimic the old behavior use `-proxy=` together with
  `-onlynet=` listing all relevant networks except `onion`.

Remote Procedure Calls (RPCs)
-----------------------------

### New RPCs
- A new `listdescriptors` RPC is available to inspect the contents of descriptor-enabled wallets.
  The RPC returns public versions of all imported descriptors, including their timestamp and flags.
  For ranged descriptors, it also returns the range boundaries and the next index to generate addresses from. (dash#5911)
- A new `send` RPC with similar syntax to `walletcreatefundedpsbt`, including
  support for coin selection and a custom fee rate. The `send` RPC is experimental
  and may change in subsequent releases. Using it is encouraged once it's no
  longer experimental: `sendmany` and `sendtoaddress` may be deprecated in a future release.
- A new `quorum platformsign` RPC is added for Platform needs. This composite command limits Platform to only request signatures from the Platform quorum type. It is equivalent to `quorum sign <platform type>`.

### RPC changes
- `createwallet` has an updated argument list: `createwallet "wallet_name" ( disable_private_keys blank "passphrase" avoid_reuse descriptors load_on_startup )`
  `load_on_startup` used to be argument 5 but now is number 6.
- `createwallet` requires specifying the `load_on_startup` flag when creating descriptor wallets due to breaking changes in v21.
- To make `sendtoaddress` more consistent with `sendmany`, the following
  `sendtoaddress` error codes were changed from `-4` to `-6`:
    - Insufficient funds
    - Fee estimation failed
    - Transaction has too long of a mempool chain
- Backwards compatibility has been dropped for two `getaddressinfo` RPC
  deprecations, as notified in the 19.1.0 and 19.2.0 release notes.
  The deprecated `label` field has been removed as well as the deprecated `labels` behavior of
  returning a JSON object containing `name` and `purpose` key-value pairs. Since
  20.1, the `labels` field returns a JSON array of label names. (dash#5823)
- `getnetworkinfo` now returns fields `connections_in`, `connections_out`,
  `connections_mn_in`, `connections_mn_out`, `connections_mn`
  that provide the number of inbound and outbound peer
  connections. These new fields are in addition to the existing `connections`
  field, which returns the total number of peer connections. Old fields
  `inboundconnections`, `outboundconnections`, `inboundmnconnections`,
  `outboundmnconnections` and `mnconnections` are removed (dash#5823)
- `getpeerinfo` no longer returns the `banscore` field unless the configuration
  option `-deprecatedrpc=banscore` is used. The `banscore` field will be fully
  removed in the next major release. (dash#5511)
- The `getpeerinfo` RPC no longer returns the `addnode` field by default. This
  field will be fully removed in the next major release.  It can be accessed
  with the configuration option `-deprecatedrpc=getpeerinfo_addnode`. However,
  it is recommended to instead use the `connection_type` field (it will return
  `manual` when addnode is true). (#6033)
- The `sendtoaddress` and `sendmany` RPCs accept an optional `verbose=True`
  argument to also return the fee reason about the sent tx. (#6033)
- The `getpeerinfo` RPC returns two new boolean fields, `bip152_hb_to` and
  `bip152_hb_from`, that respectively indicate whether we selected a peer to be
  in compact blocks high-bandwidth mode or whether a peer selected us as a
  compact blocks high-bandwidth peer. High-bandwidth peers send new block
  announcements via a `cmpctblock` message rather than the usual inv/headers
  announcements. See BIP 152 for more details.
- `upgradewallet` now returns an object for future extensibility.
- The following RPCs:  `gettxout`, `getrawtransaction`, `decoderawtransaction`,
  `decodescript`, `gettransaction`, and REST endpoints: `/rest/tx`,
  `/rest/getutxos`, `/rest/block` deprecated the following fields (which are no
  longer returned in the responses by default): `addresses`, `reqSigs`.
  The `-deprecatedrpc=addresses` flag must be passed for these fields to be
  included in the RPC response. Note that these fields are attributes of
  the `scriptPubKey` object returned in the RPC response. However, in the response
  of `decodescript` these fields are top-level attributes, and included again as attributes
  of the `scriptPubKey` object.
- The `listbanned` RPC now returns two new numeric fields: `ban_duration` and `time_remaining`.
  Respectively, these new fields indicate the duration of a ban and the time remaining until a ban expires,
  both in seconds. Additionally, the `ban_created` field is repositioned to come before `banned_until`. (#5976)
- The `walletcreatefundedpsbt` RPC call will now fail with
  `Insufficient funds` when inputs are manually selected but are not enough to cover
  the outputs and fee. Additional inputs can automatically be added through the
  new `add_inputs` option.
- The `fundrawtransaction` RPC now supports an `add_inputs` option that, when `false`,
  prevents adding more inputs even when necessary. In these cases the RPC fails.
- The `fundrawtransaction`, `send` and `walletcreatefundedpsbt` RPCs now support an `include_unsafe` option
  that, when `true`, allows using unsafe inputs to fund the transaction.
  Note that the resulting transaction may become invalid if one of the unsafe inputs disappears.
  If that happens, the transaction must be funded with different inputs and republished.
- The `getnodeaddresses` RPC now returns a `network` field indicating the
  network type (ipv4, ipv6, onion, or i2p) for each address.
- `getnodeaddresses` now also accepts a `network` argument (ipv4, ipv6, onion,
  or i2p) to return only addresses of the specified network.
- A new `sethdseed` RPC allows users to initialize their blank HD wallets with an HD seed. **A new backup must be made
  when a new HD seed is set.** This command cannot replace an existing HD seed if one is already set. `sethdseed` uses
  WIF private key as a seed. If you have a mnemonic, use the `upgradetohd` RPC.
- The following RPCs, `protx list`, `protx listdiff`, `protx info` will no longer report `collateralAddress` if the
  transaction index has been disabled (`txindex=0`).

### Improved support of composite commands

Dash Core's composite commands such as `quorum list` or `bls generate` now are compatible with a whitelist feature.

For example, the whitelist `getblockcount,quorumlist` will allow calling commands `getblockcount` and `quorum list`, but not `quorum sign`.

Note, that adding simply `quorum` in the whitelist will allow use of all `quorum...` commands, such as `quorum`, `quorum list`, `quorum sign`, etc.


### JSON-RPC Server Changes

The JSON-RPC server now rejects requests where a parameter is specified multiple times with the same name, instead of
silently overwriting earlier parameter values with later ones. (dash#6050)


P2P and network changes
-----------------------

- The protocol version has been bumped to 70232 even though it should be identical in behavior to 70231. This was done
  to help easily differentiate between v20 and v21 peers.
- With I2P connections, a new, transient address is used for each outbound
  connection if `-i2pacceptincoming=0`.
- A dashd node will no longer broadcast addresses to inbound peers by default.
  They will become eligible for address gossip after sending an `ADDR`, `ADDRV2`,
  or `GETADDR` message.


dash-tx Changes
---------------
- When creating a hex-encoded Dash transaction using the `dash-tx` utility
  with the `-json` option set, the following fields: `addresses`, `reqSigs` are no longer
  returned in the tx output of the response.

dash-cli Changes
----------------

- A new `-rpcwaittimeout` argument to `dash-cli` sets the timeout
  in seconds to use with `-rpcwait`. If the timeout expires,
  `dash-cli` will report a failure. (dash#5923)
- The `connections` field of `dash-cli -getinfo` is expanded to return a JSON
  object with `in`, `out` and `total` numbers of peer connections and `mn_in`,
  `mn_out` and `mn_total` numbers of verified mn connections. It previously
  returned a single integer value for the total number of peer connections. (dash#5823)
- Update `-getinfo` to return data in a user-friendly format that also reduces vertical space.
- A new CLI `-addrinfo` command returns the number of addresses known to the
  node per network type (including Tor v2 versus v3) and total. This can be
  useful to see if the node knows enough addresses in a network to use options
  like `-onlynet=<network>` or to upgrade to current and future Tor releases
  that support Tor v3 addresses only.  (#5904)
- CLI `-addrinfo` now returns a single field for the number of `onion` addresses
  known to the node instead of separate `torv2` and `torv3` fields, as support
  for Tor V2 addresses was removed from Dash Core in 18.0.

dash-wallet changes
-------------------
This release introduces several improvements and new features to the `dash-wallet` tool, making it more versatile and
user-friendly for managing Dash wallets.

- Wallets created with the `dash-wallet` tool will now utilize the `FEATURE_LATEST` version of wallet which is the HD
  (Hierarchical Deterministic) wallets with HD chain inside.
- new command line argument `-usehd` allows creation of non-Hierarchical Deterministic (non-HD) wallets with the
  `wallettool` for compatibility reasons since default version is bumped to HD version
- new command line argument `-descriptors` enables _experimental_ support of Descriptor wallets. It lets users
  create descriptor wallets directly from the command line. This change aligns the command-line interface with
  the `createwallet` RPC, promoting the use of descriptor wallets which offer a more flexible ways to manage
  wallet addresses and keys.
- new commands `dump` and `createfromdump` have been added, enhancing the wallet's storage migration capabilities. The
  `dump` command allows for exporting every key-value pair from the wallet as comma-separated hex values, facilitating a
  storage agnostic dump. Meanwhile, the `createfromdump` command enables the creation of a new wallet file using the
  records specified in a dump file. These commands are similar to BDB's `db_dump` and `db_load` tools and are useful
  for manual wallet file construction for testing or migration purposes.


# v21.0.0 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Alessandro Rezzi
- Kittywhiskers Van Gogh
- Konstantin Akimov
- PastaPastaPasta
- thephez
- UdjinM6
- Vijay

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These release are considered obsolete. Old release notes can be found here:

- [v20.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.1.md) released April/3/2024
- [v20.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.0.md) released March/5/2024
- [v20.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.4.md) released Jan/13/2024
- [v20.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.3.md) released December/26/2023
- [v20.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.2.md) released December/06/2023
- [v20.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.1.md) released November/18/2023
- [v20.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.0.md) released November/15/2023
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

[set-of-changes]: https://github.com/dashpay/dash/compare/v20.1.1...dashpay:v21.0.0
