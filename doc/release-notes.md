# Dash Core version v20.1.0

This is a new minor version release, including various new features, improvements, and bug fixes.

This release is optional but recommended for all nodes.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v19.2.0

Downgrading to a version older than v19.2.0 is not supported due to changes
in the evodb database. If you need to use an older version, you must either
reindex or re-sync the whole chain.

# Notable changes

## HD Wallets Enabled by Default

In this release, we are taking a significant step towards enhancing the Dash wallet's usability by enabling Hierarchical
Deterministic (HD) wallets by default. This change aligns the behavior of `dashd` and `dash-qt` with the previously
optional `-usehd=1` flag, making HD wallets the standard for all users.

While HD wallets are now enabled by default to improve user experience, Dash Core still allows for the creation of
non-HD wallets by using the `-usehd=0` flag. However, users should be aware that this option is intended for legacy
support and will be removed in future releases. Importantly, even with an HD wallet, users can still import non-HD
private keys, ensuring flexibility in managing their funds.

## Windows release signing

Windows releases are signed by a new certificate that replaces the expired one used previously.

## Removal of Non-Deterministic InstantSend

Legacy InstantSend was deprecated with the activation of Deterministic InstantSend, but we kept the underlying logic for some time to ensure a smooth transition. v20.1 finally removes all Legacy InstantSend logic and will no longer support sending and receiving islock p2p messages and inv p2p messages with id 30. Such messages will be seen as unknown by v20.1 nodes. The protocol version was incremented to 70231 to indicate a change in network protocol. Please note that the -llmqinstantsend and -llmqtestinstantsend command line options have also been removed as part of this change.

## RPC changes

- `protx info`: It's now possible to get historical data at a specific block by providing its blockhash.
- `gettxchainlocks`: will now return the status `mempool` indicating whether the transaction is in the mempool.
or not.
- Exposed transaction version numbers are now treated as unsigned 16-bit integers instead of signed 16-bit integers.
This matches their treatment in consensus logic. Versions greater than 3 continue to be non-standard (matching previous
behavior of smaller than 1 or greater than 3 being non-standard). Note that this includes the joinpsbt command, which
combines partially-signed transactions by selecting the highest version number.
- `getpeerinfo`: now returns a `connection_type` field. This indicates the type of connection established with
the peer. It will return one of six options. For more information, see the `getpeerinfo` help documentation.
- `getpeerinfo`: now has additional `last_block` and `last_transaction` fields that return the UNIX epoch time
of the last block and the last valid transaction received from each peer.
- `getblockchaininfo`: now returns a new `time` field that provides the chain tip time.
- `fundrawtransaction` and `walletcreatefundedpsbt`: when used with the `lockUnspents` argument now lock manually
selected coins, in addition to automatically selected coins. Note that locked coins are never used in automatic coin
selection, but can still be manually selected.
- `testmempoolaccept`: returns `vsize` and a `fee` object with the `base` fee if the transaction passes
validation. The `testmempoolaccept` RPC now accepts multiple transactions (still experimental at the moment, API may be
unstable).

## Added RPCs

- `submitchainlock`: allows the submission of a ChainLock signature. Note: This RPC is whitelisted for the Platform
RPC user.
- `getassetunlockstatuses`: allows fetching of Asset Unlock txs by their withdrawal index. The RPC accepts an array
of indexes and an optional block height. The possible outcomes per each index are: `chainlocked`, `mined`, `mempooled`,
`unknown`. Note: If a specific block height is passed on request, then only `chainlocked` and `unknown` outcomes are
possible. This RPC is whitelisted for the Platform RPC user.
- `quorum dkginfo`: returns information about DKGs: `active_dkgs` and `next_dkg`.
- `getrawtransactionmulti`: allows requesting a batch of up to 100 raw transactions per request.

## Docker Build System

Modified the Dockerfile to exclude the `dash-qt` graphical wallet interface from the set of binaries copied to
`/usr/local/bin` during the Docker image build process. This change streamlines the Docker image, making it more
suitable for server environments and headless applications, where the graphical user interface of `dash-qt` is not
required. The update enhances the Docker image's efficiency by reducing its size and minimizing unnecessary components.

## Command-line options

- The same ZeroMQ notification (e.g. `-zmqpubhashtx=address`) can now be specified multiple times to publish the same
notification to different ZeroMQ sockets.
- `-startupnotify=<cmd>`: Execute command on startup.


## Notification changes

- `-walletnotify` notifications are now sent for wallet transactions that are removed from the mempool because they
conflict with a new block. These notifications were sent previously, but have been broken since
the v18.1 release.
- The `startupnotify` option is used to specify a command to execute when Dash Core has finished with its startup
sequence.

## Build System

The `--enable-upnp-default` and `--enable-natpmp-default` options have been removed. If you want to use port mapping,
you can configure it using a `.conf` file, or by passing the relevant options at runtime.

## Wallet

- The wallet can create a transaction without change even when the keypool is empty. Previously it failed.
- New Discreet mode for hiding balances in Dash-Qt
- Added a "Close all wallet" menu item for Dash-Qt

## Backports from Bitcoin Core

This release introduces many updates from Bitcoin  v0.20-v25.0. Breaking changes are documented here; however, for
additional detail on what’s included in Bitcoin, please refer to their release notes.

# v20.1.0 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Alessandro Rezzi
- Kittywhiskers Van Gogh
- Konstantin Akimov
- Odysseas Gabrielides
- PastaPastaPasta
- thephez
- UdjinM6
- Vijay Manikpuri

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These release are considered obsolete. Old release notes can be found here:

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

[set-of-changes]: https://github.com/dashpay/dash/compare/v20.0.4...dashpay:v20.1.0
