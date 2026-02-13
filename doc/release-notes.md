# Dash Core version v23.1.0

This is a new minor version release, bringing new features, important bugfixes, and significant performance improvements.
This release is highly recommended for all nodes. All Masternodes are required to upgrade.

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

## InstantSend performance improvements

This release includes a collection of changes that significantly improve InstantSend lock latency. When fully adopted
across the network, these changes are expected to reduce average InstantSend lock times by 25-50%, bringing average
lock confirmation times down to near 1 second. Key improvements include:

- **Quorum message prioritization**: Network message processing now separates quorum-related messages (DKG contributions,
  signing shares, recovered signatures, ChainLock signatures, InstantSend locks) into a dedicated priority queue. This
  ensures consensus-critical quorum operations are processed with lower latency than regular network traffic. (#6952)
- **Proactive recovered signature relay**: Recovered signatures are now proactively pushed to connected nodes rather than
  waiting for them to be requested. InstantSend quorums are now fully connected for recovered signature relay, reducing
  the number of network hops required for IS lock propagation. (#6967)
- **Multi-threaded signing shares processing**: The `CSigSharesManager` has been enhanced with a multi-threaded worker
  pool and dispatcher for parallel processing of signing shares, improving throughput during periods of high signing
  activity. (#7004)
- **InstantSend height caching**: Block heights are now cached in a dedicated LRU cache, significantly reducing
  contention on `cs_main` during InstantSend lock verification. (#6953)
- **Shared mutex conversions**: The `m_nodes_mutex` and `m_peer_mutex` have been converted from recursive mutexes to
  shared mutexes, allowing concurrent read access from multiple threads. This reduces lock contention for the most
  frequent network operations. (#6912, #6468)
- **Reduced redundant work**: Peers that have requested recovered signature relay are no longer sent redundant `ISDLOCK`
  inventory announcements. Redundant signature validation during signing share processing has been eliminated. (#6994,
  #6958)

## GUI refresh

The Masternode tab has been significantly overhauled with new status icons reflecting masternode ban state, type and
ban filters, context menus replacing address columns, elastic column widths, and a detailed masternode description
dialog. A dedicated masternode model now backs the list view. (#7116)

The Governance and Masternode tabs can now be shown or hidden at runtime through the Options dialog without requiring a
client restart. The proposal model has been split out for better separation of concerns. (#7112)

## Dust attack protection

A new GUI option allows users to enable automatic dust attack protection. When enabled, small incoming transactions from
external sources that appear to be dust attacks are automatically locked, excluding them from coin selection. Users can
configure the dust threshold in the Options dialog. (#7095)

## Descriptor wallets no longer experimental

Descriptor-based wallets have been promoted from experimental status to a fully supported wallet type. Users can now
create and use descriptor wallets without the experimental warning. (#7038)

## GUI settings migration

Configuration changes made in the Dash GUI (such as the pruning setting, proxy settings, UPNP preferences) are now
saved to `<datadir>/settings.json` rather than to the Qt settings backend (Windows registry or Unix desktop config
files), so these settings will now apply to `dashd` as well, instead of being ignored.

Settings from `dash.conf` are now displayed normally in the GUI settings dialog, instead of in a separate warning
message. These settings can now be edited because `settings.json` values take precedence over `dash.conf` values.
(#6833)

## Other notable changes

* Masternodes now trickle transactions to non-masternode peers (as regular nodes do) rather than sending immediately,
  reducing information leakage while maintaining fast masternode-to-masternode propagation. (#7045)
* The Send Coins dialog now warns users when sending to duplicate addresses in the same transaction. (#7015)
* External signer (hardware wallet) support has been added as an experimental feature, allowing wallets to delegate
  signing to HWI-compatible external devices. (#6019)
* Improved wallet encryption robustness and HD chain decryption error logging. (#6938, #6944, #6945, #6939)
* Peers that re-propagate stale quorum final commitments (`QFCOMMIT`) are now banned starting at protocol version
  70239. (#7079)
* Various race conditions in ChainLock processing have been fixed. (#6924, #6940)

## P2P and network changes

- `PROTO_VERSION` has been bumped to `70240` with the introduction of protocol version-based negotiation of BIP324 v2
  transport short IDs for Dash-specific message types. The `PLATFORMBAN` message has been added to the v2 P2P short ID
  mapping (short ID 168). When communicating with peers supporting version 70240+, this message uses 1-byte encoding
  instead of 13-byte encoding, reducing bandwidth. Compatible peers use compact encoding, while older v2 peers
  automatically fall back to long encoding. (#7082)

## Updated RPCs

- `quorum dkginfo` now requires that nodes run in either watch-only mode (`-watchquorums`) or as an active masternode,
  as regular nodes do not have insight into network DKG activity. (#7062)
- `quorum dkgstatus` no longer emits the return values `time`, `timeStr` and `session` on nodes that do not run in
  either watch-only or masternode mode. (#7062)
- The `getbalances`, `gettransaction` and `getwalletinfo` RPCs now return a `lastprocessedblock` JSON object containing
  the wallet's last processed block hash and height at the time the result was generated. (#6901)
- Fixed the BLS scheme selection in `protx revoke` and `protx update_service` to use the actual deployment state rather
  than hardcoded values. (#7096)
- Fixed `protx register` to include the `Prepare` action for wallet unlock checks. (#7069)
- Added a new `next_index` field in the response in `listdescriptors` to have the same format as `importdescriptors` (#6780)

Changes to wallet related RPCs can be found in the Wallet section below.

## Updated settings

- The `shutdownnotify` option is used to specify a command to execute synchronously before Dash Core has begun its
  shutdown sequence. (#6901)

## Build System

- Dash Core binaries now target Windows 10 and macOS 14 (Sonoma), replacing the previous targets of Windows 7 and
  macOS 11 (Big Sur). (#6927)
- The minimum supported Clang version has been bumped to Clang 19 for improved C++20 support and diagnostics. (#6995)

## Wallet

- CoinJoin denomination creation now respects the wallet's "avoid_reuse" setting. When the wallet has `avoid_reuse`
  enabled, change is sent to a fresh change address to avoid address/public key reuse. Otherwise, change goes back to
  the source address (legacy behavior). (#6870)
- CoinJoin masternode tracking (`vecMasternodesUsed`) is now shared across all loaded wallets instead of per-wallet,
  improving mixing efficiency. (#6875)

## GUI changes

- Masternode tab redesigned with new status icons, type/ban filters, context menus, elastic column widths, and
  detailed masternode description dialog. (#7116)
- Runtime show/hide of Governance and Masternode tabs through Options without restart. (#7112)
- Dust attack protection option added to Options dialog. (#7095)
- Duplicate recipient warning in Send Coins dialog. (#7015)
- Auto-validation of governance proposals as fields are filled in. (#6970)
- Wallet rescan option now available when multiple wallets are loaded (rescan remains one wallet at a time). (#7072)
- Improved CreateWalletDialog layout. (#7039)
- Dash-specific font infrastructure extracted to dedicated files with `FontInfo` and `FontRegistry` classes, supporting
  arbitrary fonts and dynamic font weight resolution. (#7068, #7120)
- Fixed precision loss in proposal generation. (#7134)
- Fixed crash when changing themes after mnemonic dialog was shown. (#7126)

See detailed [set-of-changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh
- Konstantin Akimov
- PastaPastaPasta
- thephez
- UdjinM6
- Vijay
- zxccxccxz

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v23.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.0.2.md) released Dec/4/2025
- [v23.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.0.0.md) released Nov/10/2025
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

[set-of-changes]: https://github.com/dashpay/dash/compare/v23.0.2...dashpay:v23.1.0
