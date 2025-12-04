# Dash Core version v23.0.0

This is a new major version release, bringing new features, various bugfixes and other improvements.
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

### Downgrade to a version < v23.0.0

Downgrading to a version older than v23.0.0 is not supported, and will
require a reindex.

# Notable changes

## EvoDB migration

This release introduces a new internal format for masternode state data to support extended addresses and make future updates of this data seamless. Nodes will automatically migrate to the new format. Downgrading to an earlier version would require a full reindex. #6813

## Breaking Change: Block Filter Index Format Update

The compact block filter index format has been updated to include Dash special transaction data, providing feature parity with bloom filters for SPV client support. This change is incompatible with existing blockfilter indexes. Existing blockfilter indexes will automatically be re-created with the new version.

- The blockfilter index now includes fields from Dash special transactions:
  - ProRegTx (masternode registration)
  - ProUpServTx (masternode service updates)
  - ProUpRegTx (masternode operator updates)
  - ProUpRevTx (masternode revocation)
  - AssetLockTx (platform credit outputs)
- A versioning system has been added to detect incompatible indexes on startup
- The index version is now 2 (previously unversioned)

### Benefits

- SPV clients can now detect and track Dash-specific transactions
- Feature parity between bloom filters and compact block filters
- Protection against serving incorrect filter data to light clients

## Other notable changes

* Performance of block validation in Dash Core has been significantly improved, index from
  scratch and reindex are up to 20% faster compared to v22.x.
* To help prevent fingerprinting transactions created by the Dash Core wallet, change output
  amounts are now randomized. (#6685)
* Dash Core will no longer migrate EvoDb databases generated in v19 and v20, users upgrading
  from these versions are recommended to run `-reindex` to rebuild all databases and indexes. (#6579)

## Updated REST APIs

- The `/headers/` and `/blockfilterheaders/` endpoints have been updated to use
  a query parameter instead of a path parameter to specify the result count. The
  count parameter is now optional and defaults to 5 for both endpoints. The old
  endpoints are still functional, and have no documented behaviour change. (#6784)

  For `/headers`, use
  `GET /rest/headers/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>` instead of
  `GET /rest/headers/<COUNT>/<BLOCK-HASH>.<bin|hex|json>` (deprecated)

  For `/blockfilterheaders/`, use
  `GET /rest/blockfilterheaders/<FILTERTYPE>/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>` instead of
  `GET /rest/blockfilterheaders/<FILTERTYPE>/<COUNT>/<BLOCK-HASH>.<bin|hex|json>` (deprecated)

## P2P and network changes

- `MIN_PEER_PROTO_VERSION` has been bumped to `70221`. (#6877)
- `PROTO_VERSION` has been bumped to `70238` with the introduction of the `platformban` p2p message.
  This message allows evonodes to initiate PoSe masternode banning in Dash Core for evonodes which are
  not providing adequate service on Dash Platform.
- `cycleHash` field in `isdlock` message will now represent a DKG cycle starting block of the signing quorum instead of a DKG cycle starting block corresponding to the current chain height. While this is fully backwards compatible with older versions of Dash Core, other implementations might not be expecting this, so the P2P protocol version was bumped to 70237. (#6608)
- UNIX domain sockets can now be used for proxy connections. Set `-onion` or `-proxy` to the local socket path with the prefix `unix:` (e.g. `-onion=unix:/home/me/torsocket`). UNIX socket paths are now accepted for `-zmqpubrawblock` and `-zmqpubrawtx` with the format `-zmqpubrawtx=unix:/path/to/file`. (#6634)

## Updated RPCs


* `protx revoke` will now use the legacy scheme version for legacy masternodes instead of defaulting to the
   highest `ProUpRevTx` version. (#6729)

* The RPCs `protx register_legacy`, `protx register_fund_legacy`, `protx register_prepare_legacy` and
  `protx update_registrar_legacy` have been deprecated in Dash Core v23 and may be removed in a future version.
  They can be re-enabled with the runtime argument `-deprecatedrpc=legacy_mn`.

* The argument `legacy` in `bls generate` has been deprecated in Dash Core v23 and may be ignored in a future version.
  It can be re-enabled with the runtime argument `-deprecatedrpc=legacy_mn`. (#6723)

* A new optional field `submit` has been introduced to the `protx revoke`, `protx update_registrar`, `protx update_service` RPCs. It behaves identically to `submit` in `protx register` or `protx register_fund`. (#6720)

* The `instantsendtoaddress` RPC was deprecated in Dash Core v0.15 and is now removed. (#6686)

* `quorum rotationinfo` will now expect the third param to be a JSON array. (#6628)

* `getislocks` will now return the request `id` for each InstantSend Lock in results. (#6607)

* `coinjoin status` is a new RPC that reports the status message of all running mix sessions. `coinjoin start` will no longer
  report errors from mix sessions; users are recommended to query the status using `coinjoin status` instead. (#6594)

* The RPCs `masternode current` and `masternode winner` were deprecated in Dash Core v0.17 and are now removed. The `getpoolinfo` RPC was deprecated in Dash Core v0.15 and is now removed. (#6567)

* The `-deprecatedrpc=addresses` configuration option has been removed.  RPCs `gettxout`, `getrawtransaction`, `decoderawtransaction`, `decodescript`, `gettransaction verbose=true` and REST endpoints `/rest/tx`, `/rest/getutxos`, `/rest/block` no longer return the `addresses` and `reqSigs` fields, which were previously deprecated in 21.0. (#6569)

* The `getblock` RPC command now supports verbosity level 3, containing transaction inputs `prevout` information. The existing `/rest/block/` REST endpoint is modified to contain this information too. Every `vin` field will contain an additional `prevout` subfield. (#6577)

* A new RPC `newkeypool` has been added, which will flush (entirely clear and refill) the keypool. (#6577)

* The return value of the `pruneblockchain` method had an off-by-one bug and now returns the height of the last pruned block as documented. (#6784)

* The `listdescriptors` RPC now includes an optional coinjoin field to identify CoinJoin descriptors. (#6835)

### Extended address support

Dash Core v23 introduces support for extended masternode addresses, replacing legacy single-endpoint fields and
enabling more flexible network setups. The following RPC changes implement this functionality:

#### New and deprecated fields

* The keys `platformP2PPort` and `platformHTTPPort` have been deprecated for the following RPCs, `decoderawtransaction`,
  `decodepsbt`, `getblock`, `getrawtransaction`, `gettransaction`, `masternode status` (only the `dmnState` key),
  `protx diff`, `protx listdiff` and has been replaced with the key `addresses`.
  * The deprecated key is still available without additional runtime arguments, but is liable to be removed in future versions
    of Dash Core. (#6811)
* The key `service` has been deprecated for some RPCs (`decoderawtransaction`, `decodepsbt`, `getblock`, `getrawtransaction`,
  `gettransaction`, `masternode status` (only for the `dmnState` key), `protx diff`, `protx listdiff`) and has been replaced
  with the key `addresses`.
  * This deprecation also extends to the functionally identical key, `address` in `masternode list` (and its alias, `masternodelist`).
  * The deprecated key is still available without additional runtime arguments, but is liable to be removed in future versions
    of Dash Core.
  * This change does not affect `masternode status` (except for the `dmnState` key) as `service` does not represent a payload
    value but the external address advertised by the active masternode. (#6665)

#### Renamed input fields

* The input field `ipAndPort` has been renamed to `coreP2PAddrs`. In addition to accepting a string, `coreP2PAddrs` can
  now accept an array of strings, subject to validation rules.
* The input field `platformP2PPort` has been renamed to `platformP2PAddrs`. In addition to numeric inputs (i.e. ports),
  the field can now accept a string (i.e. an addr:port pair) and arrays of strings (i.e. multiple addr:port pairs),
  subject to validation rules.
* The input field `platformHTTPPort` has been renamed to `platformHTTPSAddrs`. In addition to numeric inputs (i.e. ports),
  the field can now accept a string (i.e. an addr:port pair) and arrays of strings (i.e. multiple addr:port pairs),
  subject to validation rules.

#### Reporting behavior

* When reporting on extended address payloads, `platformP2PPort` and `platformHTTPPort` will read the port value from
  `netInfo[PLATFORM_P2P][0]` and `netInfo[PLATFORM_HTTPS][0]` respectively as both fields are subsumed into `netInfo`.
  * If `netInfo` is blank (which is allowed by ProRegTx), `platformP2PPort` and `platformHTTPPort` will report `-1` to indicate
    that the port number cannot be determined.
  * `protx listdiff` will not report `platformP2PPort` or `platformHTTPPort` if the legacy fields were not updated (i.e.
    changes to `netInfo` will not translate into reporting). This is because `platformP2PPort` or `platformHTTPPort` have
    dedicated diff flags and post-consolidation, all changes are now affected by `netInfo`'s diff flag.

    To avoid the perception of changes to fields that are not serialized by extended address payloads, data from `netInfo` will
    not be translated for this RPC call. (#6666)
* The field `addresses` will now also report on platform P2P and platform HTTPS endpoints as `addresses['platform_p2p']`
  and `addresses['platform_https']` respectively.
  * On payloads before extended addresses, if a masternode update affects `platformP2PPort` and/or `platformHTTPPort`
    but does not affect `netInfo`, `protx listdiff` does not contain enough information to report on the masternode's
    address and will report the changed port paired with the dummy address `255.255.255.255`.

    This does not affect `protx listdiff` queries where `netInfo` was updated or diffs relating to masternodes that
    have upgraded to extended addresses.

#### Masternode rules

* If the masternode is eligible for extended addresses, `protx register{,_evo}` and `protx register_fund{,_evo}` will:
  * Continue allowing `coreP2PAddrs` to be left blank, as long as `platformP2PAddrs` and `platformHTTPSAddrs` are also left blank.
    * Attempting to populate any three address fields will make populating all fields mandatory.
  * Continue to allow specifying only the port number for `platformP2PAddrs` and `platformHTTPSAddrs`, pairing it with the address
    from the first `coreP2PAddrs` entry. This mirrors existing behavior.
    * This method of entry may not be available in future releases of Dash Core and operators are recommended to switch over to
      explicitly specifying (arrays of) addr:port strings for all address fields. (#6666)
  * No longer default to the core P2P port if a port is not specified in the addr:port pair. All ports must be specified explicitly.

Masternodes ineligible for extended addresses (i.e. all nodes before fork activation or legacy BLS nodes) must follow
  the legacy rules by continuing to:

* Default to the core P2P port if provided an address without a port.
* Specify `platformP2PAddrs` and `platformHTTPSAddrs` even if they wish to keep `coreP2PAddrs` blank.

## Updated settings

- BIP157 compact block filters are now automatically enabled for masternodes. This improves privacy for light clients
  connecting to masternodes and enables better support for pruned nodes. When a node is configured as a masternode
  (via `-masternodeblsprivkey`), both `-peerblockfilters` and `-blockfilterindex=basic` are automatically enabled.
  Note that this feature requires approximately 1GB+ of additional disk space for the block filter index.
  Regular nodes keep the previous defaults (disabled). Masternodes can still explicitly disable these features
  if needed by setting `-peerblockfilters=0` or `-blockfilterindex=0` (#6711).

- Setting `-maxconnections=0` will now disable `-dnsseed` and `-listen` (users may still set them to override) (#6649).

- Ports specified in `-port` and `-rpcport` options are now validated at startup. Values that previously worked and were considered valid can now result in errors. (#6634)

## Statistics

- IPv6 hosts are now supported by the StatsD client.
- `-statshost` now accepts URLs to allow specifying the protocol, host and port in one argument.
- Specifying invalid values will no longer result in the silent disablement of the StatsD client and will now cause errors at startup. (#6837)

- The arguments `-statsenabled`, `-statsns`, `-statshostname` have been removed. They were previously deprecated in v22.0 and will no longer be recognized on runtime. (#6505)

## Build System

GCC 11.1 or later, or Clang 16.0 or later, are now required to compile Dash Core. (#6389)

## Command-line Options

### Changes in Existing Command-line Options

- `-platform-user` removed in v23 (deprecated in v22 and never used by platform). (#6482)

## Wallet

- Wallet passphrases and mnemonic passphrases may now contain null characters. (#6780 #6792)

- `receivedby` RPCs now include coinbase transactions. Previously, the following wallet RPCs excluded coinbase transactions: `getreceivedbyaddress`, `getreceivedbylabel`, `listreceivedbyaddress`, `listreceivedbylabel`. The previous behaviour can be restored using `-deprecatedrpc=exclude_coinbase` (may be removed in a future release). A new option `include_immature_coinbase` (default=`false`) determines whether to account for immature coinbase transactions. (#6601)

### Mobile CoinJoin Compatibility

- Fixed an issue where CoinJoin funds mixed in the Dash Android wallet were invisible when importing the mnemonic into Dash Core. Descriptor Wallets now include an additional default descriptor for mobile CoinJoin funds, ensuring seamless wallet migration and complete fund visibility across different Dash wallet implementations.
- This is a breaking change that increases the default number of descriptors from 2 to 3 on mainnet (internal, external, mobile CoinJoin) for newly created descriptor wallets only - existing wallets are unaffected. (#6835)

## GUI changes

- A mnemonic verification dialog is now shown after creating a new HD wallet, requiring users to verify they have written down their recovery phrase (#6946).
- A new menu item "Show Recovery Phrase…" has been added to the Settings menu to view the recovery phrase for existing HD wallets (#6946).
- Added governance proposal voting functionality to the Qt interface. Users with masternode voting keys can now vote on governance proposals directly from the governance tab via right-click context menu. Added masternode count display to governance tab, showing how many masternodes the wallet can vote with. (#6690)
- Added a menu item to restore a wallet from a backup file. (#6648)

## RPC Wallet

- `unloadwallet` now fails if a rescan is in progress. (#6759)
- `gettransaction`, `listtransactions`, `listsinceblock` now return the `abandoned` field for all transactions. (#6780)

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh
- Konstantin Akimov
- Odysseas Gabrielides
- PastaPastaPasta
- thephez
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v22.1.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.3.md) released Jul/15/2025
- [v22.1.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.2.md) released Apr/15/2025
- [v22.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.1.md) released Feb/17/2025
- [v22.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.0.md) released Feb/10/2025
- [v22.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.0.0.md) released Dec/12/2024
- [v21.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.1.md) released Oct/22/2024
- [v21.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.0.md) released Aug/8/2024
- [v21.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.2.md) released Aug/1/2024
- [v21.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.0.md) released Jul/25/2024
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

[set-of-changes]: https://github.com/dashpay/dash/compare/v22.1.3...dashpay:v23.0.0